# include <collins/lm.hpp>
# include <gusc/trie/basic_trie.hpp>
# include <gusc/trie/fixed_trie.hpp>
# include <gusc/trie/trie_algo.hpp>
# include <gusc/iterator/ostream_iterator.hpp>
# include <boost/operators.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/interprocess/containers/flat_map.hpp>
# include <boost/interprocess/containers/string.hpp>
# include <boost/interprocess/containers/vector.hpp>
# include <boost/interprocess/allocators/allocator.hpp>
# include <boost/interprocess/managed_mapped_file.hpp>
# include <boost/regex.hpp>
# include <boost/algorithm/string.hpp>
# include <cstring>

//# define NO_ABSTRACTION 1

# define COLLINS_DEBUG_PRINT(x) x
# define COLLINS_DEBUG_PRINT_SCORE(x)
//SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(lexlog,"collins-lex-lm",sbmt::root_domain);
namespace ip = boost::interprocess;

namespace collins {

struct dictkey {
    ip::offset_ptr<const char> ptr;
    dictkey(const char* p = 0) : ptr(p) {}
    const char* get() const { return ptr.get(); }
};

typedef std::map< int, std::vector<int> > alignment;
typedef std::vector<std::string> source;

source read_source(std::string s)
{
    std::vector<std::string> v;
    boost::split(v,s,boost::is_any_of(" "));
    for (int x = 0; x != v.size(); ++x) v[x] = collins::at_replace(v[x]);
    return v;
}

alignment read_alignment(std::string s)
{
    alignment a;
    std::vector<std::string> v;
    boost::split(v,s,boost::is_any_of(" "));
    BOOST_FOREACH(std::string ss, v) {
        std::vector<std::string> vv;
        boost::split(vv,ss,boost::is_any_of("-"));
        a[boost::lexical_cast<int>(vv[0])].push_back(boost::lexical_cast<int>(vv[1]));
    }
    for (alignment::iterator itr = a.begin(); itr != a.end(); ++itr) {
        std::sort(itr->second.begin(),itr->second.end());
    }
    return a;
}

std::vector<std::string> affiliated_source(source const& src, alignment const& a, int d)
{
    std::vector<std::string> f(11);
    if (d < 0) {
        for (int c = 0; c != 11; ++c) f[c] = "NULL";
    } else {
        alignment::const_iterator pos = a.find(d);
        if (pos == a.end()) pos = a.upper_bound(d);
        if (pos == a.end()) {
            for (int dd = d; dd != -1; --dd) {
                pos = a.find(dd);
                if (pos != a.end()) break;
            }
        }
        if (pos == a.end()) throw std::logic_error("BRRK " + boost::lexical_cast<std::string>(d));
        int af = pos->second[pos->second.size()/2];
        for (int c = 5, a = af; c != 11; ++c, ++a) {
            if (a >= src.size()) f[c] = "</source_s>";
            else f[c] = src[a];
        }
        for (int c = 4, a = af -1; c != -1; --c, --a) {
            if (a < 0) f[c] = "<source_s>";
            else f[c] = src[a];
        }
    }
    return f;
}

struct trie_value : boost::equality_comparable<trie_value> {
    boost::uint32_t categories;
    boost::uint32_t count;
    trie_value(int count = 0, int categories = 0) : categories(categories), count(count) {}
    bool operator == (trie_value const& other) const
    {
        return other.categories == categories and other.count == count;
    }
};

std::ostream& operator << (std::ostream& out, trie_value const& c)
{
    out << '(' << c.count << ' ' << c.categories << ')';
    return out;
}

using boost::uint32_t;

struct trie_return {
    uint32_t sum;
    uint32_t categories;
    uint32_t count;
    trie_return(uint32_t n, uint32_t k, uint32_t c)
    : sum(n)
    , categories(k)
    , count(c) {}
    trie_return()
    : sum(0)
    , categories(0)
    , count(0) {}
};

void make_rule(treelib::Tree::iterator_base const& node, std::vector<strrule>& vv) 
{
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    
    if ((not node->getIsPreterminal()) and (itr != end)) {
        vv.push_back(collins::make_strrule(node));
        itr = node.begin();
        for (; itr != end; ++itr) make_rule(itr,vv);
    }
}

std::ostream& operator<< (std::ostream& out, trie_return const& tr)
{
    return out << '(' << tr.sum << ',' << tr.categories << ',' << tr.count << ')';
}

bool getline(std::istream& in, std::vector<std::string>& linevec)
{
    std::string line;
    if (getline(in,line)) {
        linevec.clear();
        std::stringstream sstr(line);
        copy(std::istream_iterator<std::string>(sstr),std::istream_iterator<std::string>(),std::back_inserter(linevec));
        return true;
    } else {
        return false;
    }
}

bool getline(std::istream& in, std::vector<std::vector<std::string> >& linevecvec)
{
    std::string line;
    if (getline(in,line)) {
        linevecvec.clear();
        std::vector<std::string> linevec;
        boost::split(linevec,line,boost::is_any_of("\t"));
        BOOST_FOREACH(std::string const& lv,linevec) {
            linevecvec.push_back(std::vector<std::string>());
            boost::split(linevecvec.back(),lv,boost::is_any_of(" "));
        }
        return true;
    } else {
        return false;
    }
}

typedef gusc::basic_trie<boost::uint32_t,trie_value> lm_trie;


typedef ip::managed_mapped_file::segment_manager segment_manager;
typedef ip::allocator<char,segment_manager> char_allocator;
typedef ip::basic_string<char,std::char_traits<char>,char_allocator> fstring;
typedef ip::allocator<fstring, segment_manager> string_allocator;

struct indirect_less {
    typedef bool return_value;
    return_value operator()(dictkey p1, dictkey p2) const
    {
        return ::strcmp(p1.get(),p2.get()) < 0;
    }
};

typedef ip::vector<dictkey,ip::allocator<dictkey,segment_manager> > 
        reverse_dictionary;

typedef ip::flat_map<
          dictkey
        , boost::uint32_t
        , indirect_less
        , ip::allocator<
            std::pair<dictkey,boost::uint32_t>
          , segment_manager
          >
        > dictionary;

typedef gusc::fixed_trie<
          boost::uint32_t
        , trie_value
        , char_allocator
        , gusc::create_plain
        > fixed_lm_trie;

template <class Iterator>
void insert(Iterator begin, Iterator given_end, Iterator end, lm_trie& counts, size_t count)
{
        // the insertions. first the conditional
        lm_trie::state s,p; bool inserted;
        
        boost::tie(inserted,s) = counts.insert(begin,given_end, trie_value(count));
        if (not inserted) counts.value(s).count += count;
        
        boost::tie(inserted,p) = counts.insert(begin,end, trie_value(count));
        if (not inserted) counts.value(p).count += count;
        else counts.value(s).categories += 1;
}

template <class Iterator>
void insert(Iterator given_beg, Iterator given_end, uint32_t sep, Iterator beg, Iterator end, lm_trie& counts, size_t count)
{
        // the insertions. first the conditional
        lm_trie::state s,p; bool inserted;
        trie_return v;
        boost::tie(inserted,s) = counts.insert(given_beg,given_end,trie_value(count));
        if (not inserted) counts.value(s).count += count;
        
        boost::tie(inserted,p) = counts.insert(s,&sep,&sep+1,trie_value(1));
        
        boost::tie(inserted,p) = counts.insert(p,beg,end,trie_value(count));
        if (not inserted) counts.value(p).count += count;
        else counts.value(s).categories += 1;
        v.categories = counts.value(s).categories;
        v.sum = counts.value(s).count;
        v.count = counts.value(p).count;
        COLLINS_DEBUG_PRINT(std::cerr << "insert: " << v << '\n');
}

/*
void dep_given_rule_head(treelib::Tree::iterator_base const& node)
{
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    int x = 1;
    int h = node->getHeadPosition();
    if (h != 0) for (;itr != end; ++itr,++x) if (x != h) {
        treelib::Tree::sibling_iterator jtr = node.begin();
        std::cout << node->getLabel() << ' ' << node->getHeadPosition();
        for (; jtr != end; ++jtr) std::cout << ' ' << jtr->getLabel();
        std::cout << ' ' << x << ' ' << at_replace(node->getHeadword());
        std::cout << '\t' << at_replace(itr->getHeadword());
        std::cout << '\t' << 1 << '\n';
    }
    itr = node.begin();
    for (; itr != end; ++itr) dep_given_rule_head(itr);
}
*/

void make_dictionary(std::istream& in, ip::managed_mapped_file& mfile)
{
    char_allocator calloc(mfile.get_segment_manager());
    std::set<dictkey,indirect_less> dset;
    std::vector<std::string> v;
    
    std::string tab = "~\t~";
    fstring* fstr = mfile.construct<fstring>(ip::anonymous_instance)(tab.begin(),tab.end(),calloc);
    dset.insert(dictkey(fstr->c_str()));
    
    for (int x = 0; x != 10; ++x) {
        std::string num = " " + boost::lexical_cast<std::string>(x);
        fstr = mfile.construct<fstring>(ip::anonymous_instance)(num.begin(),num.end(),calloc);
    }
    
    std::string STOP = "STOP";
    fstr = mfile.construct<fstring>(ip::anonymous_instance)(STOP.begin(),STOP.end(),calloc);
    dset.insert(dictkey(fstr->c_str()));
    
    while (getline(in,v)) {
        fstr = mfile.construct<fstring>(ip::anonymous_instance)(v[0].begin(),v[0].end(),calloc);
        dset.insert(dictkey(fstr->c_str()));
    }
    
    dictionary* dict = mfile.construct<dictionary>("dictionary")(indirect_less(),calloc);
    reverse_dictionary* revdict = mfile.construct<reverse_dictionary>("reverse-dictionary")(calloc);
    revdict->reserve(dset.size());
    dict->reserve(dset.size());
    BOOST_FOREACH(dictkey ss, dset) {
        revdict->push_back(ss);
    }
    uint32_t xx = 0;
    BOOST_FOREACH(dictkey ss, dset) {
        dict->insert(std::make_pair(ss,xx));
        ++xx;
    }
}

size_t at(dictionary const& dict, std::string const& lbl)
{
    return dict.at(dictkey(lbl.c_str()));
}

std::string word(uint32_t x, reverse_dictionary const& revdict)
{
    if (x < revdict.size()) return revdict.at(x).get();
    else return "@UNKNOWN@";
}

trie_return get_counts(fixed_lm_trie const& counts, uint32_t const* beg, uint32_t const* mid, uint32_t const* end)
{
    //std::cerr << "get_counts:";
    //for (uint32_t const* itr = mid; itr != end; ++itr) std::cerr << ' ' << *itr;
    //std::cerr << " |";
    //for (uint32_t const* itr = beg; itr != mid; ++itr) std::cerr << ' ' << *itr;
    
    trie_return retval;
    gusc::trie_find_result<fixed_lm_trie,uint32_t const*> res = trie_find(counts,counts.start(),beg,mid);
    if (res.found) {
        //std::cerr << " cond=" << counts.value(res.state);
        retval.sum = counts.value(res.state).count;
        retval.categories = counts.value(res.state).categories;
        res = trie_find(counts,res.state,mid,end);
        if (res.found) {
            //std::cerr << "count=" << counts.value(res.state);
            retval.count = counts.value(res.state).count;
        }
    }
    //std::cerr << '\n';
    
    return retval;
}

using namespace std;

void make_deptag_given_rule_headtag_headword( std::istream& in, lm_trie& counts, dictionary& dict )
{
    vector<string> v;
    while (getline(in,v)) {
        vector<boost::uint32_t> tv;
        tv.push_back(at(dict,v.at(0)));
        tv.push_back(boost::lexical_cast<size_t>(v.at(1)));
        vector<string>::iterator pos = v.begin() + 2;
        vector<string>::iterator end = v.end() - 1;
        for (; pos != end; ++pos) tv.push_back(at(dict,*pos));
        size_t count = boost::lexical_cast<size_t>(*pos++);
        assert (pos == v.end());
        insert(tv.begin(), tv.end() - 1, tv.end(), counts, count);
    }    
}

idxrule str2idxrule(strrule const& r, dictionary& dict)
{
    idxrule res;
    res.hpos = r.hpos;
    res.sz = r.sz;
    for (size_t x = 0; x != res.sz; ++x) {
        res.r[x].head = at(dict,r.r[x].head);
        res.r[x].tag = at(dict,r.r[x].tag);
        res.r[x].label = at(dict,r.r[x].label);
    }
    return res;
}


// for recovering cfg scores. do not use to populate model!
struct gen_rule_given_root {
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t N=32;
        uint32_t vec[N];
        uint32_t* vvec[6];
        uint32_t* vecitr = vec;
        uint32_t** vvecitr = vvec;
        *vvecitr++ = vecitr;
        *vecitr++ = rule.lhs().label;
        *vvecitr++ = vecitr;
        *vecitr++ = rule.hpos;
        BOOST_FOREACH(idxrule::variable const& v, rule.rhs()) {
            *vecitr++ = v.label;
        }
        *vvecitr++ = vecitr;
    
        op(std::make_pair(vvec,vvecitr));
    }
};

struct gen_rule_given_root_headtag_headword {
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t N=32;
        uint32_t vec[N];
        uint32_t* vvec[6];
        uint32_t* vecitr = vec;
        uint32_t** vvecitr = vvec;
        *vvecitr++ = vecitr;
        *vecitr++ = rule.lhs().label;
        *vvecitr++ = vecitr;
        *vecitr++ = rule.lhs().tag;
        *vvecitr++ = vecitr;
        *vecitr++ = rule.lhs().head;
        *vvecitr++ = vecitr;
        *vecitr++ = rule.hpos;
        BOOST_FOREACH(idxrule::variable const& v, rule.rhs()) {
            *vecitr++ = v.label;
        }
        *vvecitr++ = vecitr;
    
        op(std::make_pair(vvec,vvecitr));
    }
};

// for amr
struct gen_label_given_concept {
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t lbl = f->size();
        uint32_t vec[16];
        uint32_t* vvec[8];
        uint32_t* vecitr;
        uint32_t** vvecitr;
        if (rule.sz == 4) {
            // identify the label
            for (size_t x = 1; x != 4; ++x) {
                if (word(rule.r[x].head,*f)[0] == ':') {
                    lbl = rule.r[x].head;
                    break;
                }
            }
            
            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            // abuse of stop as holder for unary backoff
            *vecitr++ = STOP;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().head;
            *vvecitr++ = vecitr;
            
            *vecitr++ = lbl;
            *vvecitr++ = vecitr;
    
            op(std::make_pair(vvec,vvecitr));
        }
        //generating the STOP probability, though we are actually doing it at the beginning
        char STOPCOND = word(rule.r[rule.hpos].label,*f)[0];
        if (STOPCOND == 'S') {
            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            // abuse of stop as holder for unary backoff
            *vecitr++ = STOP;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.r[rule.hpos].head;
            *vvecitr++ = vecitr;
            
            *vecitr++ = STOP;
            *vvecitr++ = vecitr;
            
            op(std::make_pair(vvec,vvecitr));
        }
    }
    gen_label_given_concept(reverse_dictionary const& f, uint32_t TOP, uint32_t STOP)
    : f(&f)
    , TOP(TOP)
    , STOP(STOP) {}
    reverse_dictionary const* f;
    uint32_t TOP;
    uint32_t STOP;
};

// for amr
struct gen_label_given_abstraction_concept {
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t lbl = f->size();
        uint32_t vec[16];
        uint32_t* vvec[8];
        uint32_t* vecitr;
        uint32_t** vvecitr;
        if (rule.sz == 4) {
            // identify the label
            for (size_t x = 1; x != 4; ++x) {
                if (word(rule.r[x].head,*f)[0] == ':') {
                    lbl = rule.r[x].head;
                    break;
                }
            }
            
            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().tag;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().head;
            *vvecitr++ = vecitr;
            
            *vecitr++ = lbl;
            *vvecitr++ = vecitr;
    
            op(std::make_pair(vvec,vvecitr));
        }
        //generating the STOP probability, though we are actually doing it at the beginning
        char STOPCOND = word(rule.r[rule.hpos].label,*f)[0];
        if (STOPCOND == 'S') {
            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.r[rule.hpos].tag;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.r[rule.hpos].head;
            *vvecitr++ = vecitr;
            
            *vecitr++ = STOP;
            *vvecitr++ = vecitr;
            
            op(std::make_pair(vvec,vvecitr));
        }
    }
    gen_label_given_abstraction_concept(reverse_dictionary const& f, uint32_t TOP, uint32_t STOP)
    : f(&f)
    , TOP(TOP)
    , STOP(STOP) {}
    reverse_dictionary const* f;
    uint32_t TOP;
    uint32_t STOP;
};

struct gen_depabstraction_given_label_abstraction_concept {
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t lbl = f->size();
        int32_t dpos = -1;
        uint32_t vec[16];
        uint32_t* vvec[8];
        uint32_t* vecitr = vec;
        uint32_t** vvecitr = vvec;
        
        if (rule.lhs().label == TOP) {
            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            *vecitr++ = TOP;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().tag;
            *vvecitr++ = vecitr;
            
            op(std::make_pair(vvec,vvecitr));
        }
        
        if (rule.sz == 4) {
            // identify the label and location of dep
            for (size_t x = 1; x != 4; ++x) {
                if (word(rule.r[x].head,*f)[0] == ':') {
                    lbl = rule.r[x].head;
                } else if (x != rule.hpos) {
                    dpos = x;
                }
            }
            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            *vecitr++ = lbl;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().tag;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().head;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.r[dpos].tag;
            *vvecitr++ = vecitr;
        
            op(std::make_pair(vvec,vvecitr));
        }
    }
    gen_depabstraction_given_label_abstraction_concept(reverse_dictionary const& f, uint32_t TOP, uint32_t STOP)
    : f(&f)
    , TOP(TOP)
    , STOP(STOP) {}
    reverse_dictionary const* f;
    uint32_t TOP;
    uint32_t STOP;
};

struct gen_depconcept_given_depabstraction_labelabstraction_concept {
    gen_depconcept_given_depabstraction_labelabstraction_concept(reverse_dictionary const& f, uint32_t TOP, uint32_t STOP)
    : f(&f)
    , TOP(TOP)
    , STOP(STOP) {}
    reverse_dictionary const* f;
    uint32_t TOP;
    uint32_t STOP;
    
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t lbl = f->size();
        int32_t dpos = -1;
        uint32_t vec[16];
        uint32_t* vvec[8];
        uint32_t* vecitr = vec;
        uint32_t** vvecitr = vvec;
        
        if (rule.lhs().label == TOP) {            
            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().tag;
            *vvecitr++ = vecitr;
            
            *vecitr++ = TOP;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().head;
            *vvecitr++ = vecitr;
            
            op(std::make_pair(vvec,vvecitr));
        }
        
        if (rule.sz == 4) {
            // identify the label and location of dep
            for (size_t x = 1; x != 4; ++x) {
                if (word(rule.r[x].head,*f)[0] == ':') {
                    lbl = rule.r[x].head;
                } else if (x != rule.hpos) {
                    dpos = x;
                }
            }

            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            // labels are closed set? no unary backoff required?
            *vecitr++ = rule.r[dpos].tag;
            *vvecitr++ = vecitr;
            
            *vecitr++ = lbl;
            *vecitr++ = rule.lhs().tag;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().head;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.r[dpos].head;
            *vvecitr++ = vecitr;
            
            op(std::make_pair(vvec,vvecitr));
        }
    }
};

struct gen_depconcept_given_label_concept {
    gen_depconcept_given_label_concept(reverse_dictionary const& f, uint32_t TOP, uint32_t STOP)
    : f(&f)
    , TOP(TOP)
    , STOP(STOP) {}
    reverse_dictionary const* f;
    uint32_t TOP;
    uint32_t STOP;
    
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t lbl = f->size();
        int32_t dpos = -1;
        uint32_t vec[16];
        uint32_t* vvec[8];
        uint32_t* vecitr = vec;
        uint32_t** vvecitr = vvec;
        
        if (rule.lhs().label == TOP) {            
            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            // abuse of stop as holder for unary backoff
            *vecitr++ = STOP;
            *vvecitr++ = vecitr;
            
            *vecitr++ = TOP;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().head;
            *vvecitr++ = vecitr;
            
            op(std::make_pair(vvec,vvecitr));
        }
        
        if (rule.sz == 4) {
            // identify the label and location of dep
            for (size_t x = 1; x != 4; ++x) {
                if (word(rule.r[x].head,*f)[0] == ':') {
                    lbl = rule.r[x].head;
                } else if (x != rule.hpos) {
                    dpos = x;
                }
            }

            vecitr = vec;
            vvecitr = vvec;
            *vvecitr++ = vecitr;
            
            // labels are closed set? no unary backoff required?
            
            *vecitr++ = lbl;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.lhs().head;
            *vvecitr++ = vecitr;
            
            *vecitr++ = rule.r[dpos].head;
            *vvecitr++ = vecitr;
            
            op(std::make_pair(vvec,vvecitr));
        }
    }
};

struct gen_depword_given_deptag {
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t N=32;
        uint32_t vec[N];
        uint32_t* vvec[6];
    
        if (rule.hpos > 0) {
            size_t x = 1;
            BOOST_FOREACH(idxrule::variable const& v, rule.rhs()) {
                if (x != rule.hpos) {
                    uint32_t* vecitr = vec;
                    uint32_t** vvecitr = vvec;
                    *vvecitr++ = vecitr;
                    *vecitr++ = v.tag;
                    *vvecitr++ = vecitr;
                    *vecitr++ = v.head;
                    *vvecitr++ = vecitr;
                    op(std::make_pair(vvec,vvecitr));
                }
                ++x;
            }
        }
    }
};

struct gen_depword_given_deptag_var_ruleheadtag_headword {
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t N=32;
        uint32_t vec[N];
        uint32_t* vvec[6];
    
        if (rule.hpos > 0) {
            size_t x = 1;
            BOOST_FOREACH(idxrule::variable const& v, rule.rhs()) {
                if (x != rule.hpos) {
                    uint32_t* vecitr = vec;
                    uint32_t** vvecitr = vvec;
                    *vvecitr++ = vecitr;
                    *vecitr++ = v.tag;
                    *vvecitr++ = vecitr;
                    *vecitr++ = rule.rhs(x).label;
                    *vvecitr++ = vecitr;
                    *vecitr++ = rule.lhs().label;
                    *vecitr++ = rule.hpos;
                    BOOST_FOREACH(idxrule::variable const& cv, rule.rhs()) {
                        *vecitr++ = cv.label;
                    }
                    *vecitr++ = rule.lhs().tag;
                    *vvecitr++ = vecitr;
                    *vecitr++ = rule.lhs().head;
                    *vvecitr++ = vecitr;
                    *vecitr++ = v.head;
                    *vvecitr++ = vecitr;
                
                    op(std::make_pair(vvec,vvecitr));
                }
                ++x;
            }
        }
    }
};

struct gen_deptag_given_var_rule_headtag_headword {
    template <class Operator>
    void operator()(idxrule const& rule, Operator& op)
    {
        uint32_t N=32;
        uint32_t vec[N];
        uint32_t* vvec[6];
    
        if (rule.hpos > 0) {
            size_t x = 1;
            BOOST_FOREACH(idxrule::variable const& v, rule.rhs()) {
                if (x != rule.hpos) {
                    uint32_t* vecitr = vec;
                    uint32_t** vvecitr = vvec;
                    *vvecitr++ = vecitr;
                    *vecitr++ = rule.rhs(x).label;
                    *vvecitr++ = vecitr;
                    *vecitr++ = rule.lhs().label;
                    *vecitr++ = rule.hpos;
                    BOOST_FOREACH(idxrule::variable const& cv, rule.rhs()) {
                        *vecitr++ = cv.label;
                    }
                    *vvecitr++ = vecitr;
                    *vecitr++ = rule.lhs().tag;
                    *vvecitr++ = vecitr;
                    *vecitr++ = rule.lhs().head;
                    *vvecitr++ = vecitr;
                    *vecitr++ = v.tag;
                    *vvecitr++ = vecitr;
                
                    op(std::make_pair(vvec,vvecitr));
                }
                ++x;
            }
        }
    }
};

template <class VecVec>
void add_generic(VecVec const& vv, lm_trie& counts, reverse_dictionary const& dict, uint32_t sep, uint32_t count)
{
    typedef typename boost::range_iterator<VecVec>::type VecVecItr;
    VecVecItr vvbeg = boost::begin(vv), vvend = boost::end(vv);
    VecVecItr beg = vvend - 2, end = vvend - 1;
    VecVecItr given_beg = vvbeg, given_end = vvbeg + 1;
    for (; given_end != end; ++given_end) {
        COLLINS_DEBUG_PRINT(
            std::cerr << "insert ";
            BOOST_FOREACH(uint32_t tok, std::make_pair(*beg,*end)) std::cerr << word(tok,dict) << ' ';
            std::cerr << "given ";
            BOOST_FOREACH(uint32_t tok, std::make_pair(*given_beg,*given_end)) std::cerr << word(tok,dict) << ' ';
        );
        insert(*given_beg,*given_end,sep,*beg,*end,counts,count);
    }
}

template <class VecVec>
std::vector<std::string> 
print_generic(VecVec const& vv, lm_trie& counts, reverse_dictionary const& dict, uint32_t sep)
{
    typedef typename boost::range_iterator<VecVec>::type VecVecItr;
    VecVecItr vvbeg = boost::begin(vv), vvend = boost::end(vv);
    VecVecItr beg = vvend - 2, end = vvend - 1;
    VecVecItr given_beg = vvbeg, given_end = vvbeg + 1;
    for (; given_end != end; ++given_end);
    --given_end;
    std::vector<std::string> ret;
    BOOST_FOREACH(uint32_t tok, std::make_pair(*given_beg,*given_end)) ret.push_back(word(tok,dict));// << '\t';
    BOOST_FOREACH(uint32_t tok, std::make_pair(*beg,*end)) ret.push_back(word(tok,dict));// << '\n';
    return ret;
}

struct add_generic_op {
    add_generic_op(lm_trie& counts, reverse_dictionary const& dict, uint32_t sep)
    : counts(&counts)
    , dict(&dict)
    , sep(sep) {}
    lm_trie* counts; 
    reverse_dictionary const* dict;
    uint32_t sep;
    template <class VecVec>
    void operator()(VecVec const& vv)
    {
        add_generic(vv,*counts,*dict,sep,1);
    }
};

struct print_generic_op {
    print_generic_op(lm_trie& counts, reverse_dictionary const& dict, uint32_t sep)
    : counts(&counts)
    , dict(&dict)
    , sep(sep) {}
    lm_trie* counts; 
    reverse_dictionary const* dict;
    std::vector<std::vector<std::string> > retval;
    uint32_t sep;
    template <class VecVec>
    void operator()(VecVec const& vv)
    {
        retval.push_back(print_generic(vv,*counts,*dict,sep));
    }
};

template <class Func>
void ruleinfo_to_model(Func func, std::istream& in, lm_trie& counts, dictionary& dict, reverse_dictionary const& rdict)
{
    uint32_t sep = at(dict,"~\t~");
    add_generic_op op(counts,rdict,sep);
    std::string line;
    std::stringstream sstr;
    while (getline(in,line)) {
        sstr.str(line);
        sstr.clear();
        strrule srule;
        sstr >> srule;
        idxrule rule = str2idxrule(srule,dict);
        COLLINS_DEBUG_PRINT(std::cerr << srule << '\n');
        func(rule,op);
    }
}

void count_preterm(treelib::Tree::iterator_base const& node, int& x) 
{
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    if (node->getIsPreterminal()) ++x;
    else if ((not node->getIsPreterminal()) and (itr != end)) {
        for (; itr != end; ++itr) {
            count_preterm(itr,x);
        }
    }
}

int count_preterm(treelib::Tree::iterator_base const& node)
{
    int x = 0;
    count_preterm(node,x);
    return x;
}

//int amr_dep()

template <class Func>
int print_model_recurse( Func func
                       , dictionary& dict
                       , print_generic_op op
                       , int sentid
                       , treelib::Tree::iterator_base const& node
                       , source const& src
                       , alignment const& a
                       , double wt
                       , int& x
                       , bool use_dep )
{
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    
    if (node->getIsPreterminal()) {
        return x++;
    }
    else if ((not node->getIsPreterminal()) and (itr != end)) {
        strrule srule = make_strrule(node);
        idxrule irule = str2idxrule(srule,dict);
        
        int pos = -1;
        if (srule.sz == 4 and use_dep) for (size_t xx = 1; xx != 4; ++xx) {
            if (srule.r[xx].head[0] != ':' and xx != srule.hpos) {
                pos = xx;
            }
        }
        else pos = irule.hpos;
        
        std::vector<int> headidx;
        op.retval.clear();
        func(irule,op);
        for (; itr != end; ++itr) headidx.push_back(print_model_recurse(func,dict,op,sentid,itr,src,a,wt,x,use_dep));
        BOOST_FOREACH(std::vector<std::string> const& vs, op.retval) {
            std::cerr << srule <<" pos=" << pos << " headidx[pos-1]=" << headidx[pos-1] << '\n';
            std::vector<std::string> f = affiliated_source(src,a,headidx[pos - 1]);
            std::copy(f.begin(),f.end(), gusc::ostream_iterator(std::cout," "));
            std::cout << '\t';
            std::copy(vs.begin(),vs.end(),gusc::ostream_iterator(std::cout,"\t"));
            std::cout << '\n';
        }
        return headidx[irule.hpos - 1];
    } else {
        throw std::logic_error("WAHTH");
    }
}

template <class Func>
void print_model(Func func, std::istream& in, lm_trie& counts, dictionary& dict, reverse_dictionary const& rdict, bool use_dep)
{
    uint32_t sep = at(dict,"~\t~");
    print_generic_op op(counts,rdict,sep);
    std::string line;
    std::stringstream sstr;
    size_t linecount = 0;
    while (getline(in,line)) {
        ++linecount;
        std::vector<std::string> v;
        boost::split(v,line,boost::is_any_of("\t"));
        if (v.size() < 4 or v[0] == "0" or v[0] == "" or v[1] == "" or v[2] == "" or v[3] == "") {
            std::cout << linecount << '\n';
            continue;
        }
        source src = read_source(v[1]);
        alignment a = read_alignment(v[2]);
        double wt = boost::lexical_cast<double>(v[3]);
        if (src.size() == 0 or a.size() == 0 ) {
            std::cout << linecount << '\n';
            continue;
        }
        treelib::Tree tree;
        tree.read(v[0]);
        int pt = count_preterm(tree.begin());
        if (pt < a.rbegin()->first) {
            std::cout << linecount << '\n';
            continue;
        }
        int x = 0;
        print_model_recurse(func,dict,op,linecount,tree.begin(),src,a,wt,x,use_dep);
    }
}

trie_return 
get_counts( fixed_lm_trie const& counts
          , uint32_t const* given_begin, uint32_t const* given_end
          , uint32_t const sep
          , uint32_t const* begin, uint32_t const* end )
{
    //std::cerr << "get_counts:";
    //for (uint32_t const* itr = mid; itr != end; ++itr) std::cerr << ' ' << *itr;
    //std::cerr << " |";
    //for (uint32_t const* itr = beg; itr != mid; ++itr) std::cerr << ' ' << *itr;
    
    trie_return retval;
    gusc::trie_find_result<fixed_lm_trie,uint32_t const*> res = trie_find(counts,counts.start(),given_begin,given_end);
    if (res.found) {
        
        //std::cerr << " cond=" << counts.value(res.state);
        retval.sum = counts.value(res.state).count;
        retval.categories = counts.value(res.state).categories;
        res = trie_find(counts,res.state,&sep,&sep+1);
        res = trie_find(counts,res.state,begin,end);
        if (res.found) {
            //std::cerr << "count=" << counts.value(res.state);
            retval.count = counts.value(res.state).count;
        } 
    }
    COLLINS_DEBUG_PRINT_SCORE(std::cerr << "get_counts: " << retval << "\n");
    //std::cerr << '\n';
    return retval;
}

double wb_backoff(trie_return b, double off, double wb_rate)
{
    if (b.count > 0) return double(b.count) / double(b.sum + wb_rate * b.categories);
    else if (b.sum > 0) return double(b.categories) / double(b.sum + wb_rate * b.categories);
    //else return 0.0;
    else return off;
}

double wb_backoff(trie_return b, trie_return bb, double wb_rate)
{
    if (b.count > 0) return double(b.count) / double(b.sum + wb_rate * b.categories);
    else if (b.sum > 0) {
        return (double(b.categories) / double(b.sum + wb_rate * b.categories)) * wb_backoff(bb,1.0,wb_rate) ;
    }
    //else return 0.0;
    else return 1.0;
}

double wb_smooth(trie_return v, double backoff, double wb_rate)
{
    //std::cerr << "wb: v=" << v << " backoff=" << backoff << "\n";
    double w = 0;
    double p1 = 0;
    if (v.sum > 0) {
        w  = double(v.sum) / double(v.sum + wb_rate * v.categories);
        p1 = double(v.count) / double(v.sum);
    }

    return w * p1 + (1 - w) * backoff;
}

double wb_smooth(trie_return v, trie_return b, double wb_rate)
{
    //std::cerr << "wb: v=" << v << " b=" << b << "\n";
    double w = 0;
    double p1 = 0;
    if (v.sum > 0) {
        w  = double(v.sum) / double(v.sum + wb_rate * v.categories);
        p1 = double(v.count) / double(v.sum);
    }

    return w * p1 + (1 - w) * wb_backoff(b, 1.0, wb_rate);
}

double wb_smooth(trie_return v, trie_return b, trie_return bb, double wb_rate, double wbb_rate)
{
    //std::cerr << "wb: v=" << v << " b=" << b << "\n";
    double w = 0;
    double p1 = 0;
    if (v.sum > 0) {
        w  = double(v.sum) / double(v.sum + wb_rate * v.categories);
        p1 = double(v.count) / double(v.sum);
    }
    return w * p1 + (1 - w) * wb_backoff(b,bb,wb_rate);

    //return w * p1 + (1 - w) * wb_smooth(b, bb, wbb_rate);
}

template <class VecVec>
double score_generic(VecVec const& vv, fixed_lm_trie const& counts, double off, double wbrate, uint32_t sep, reverse_dictionary const& dict)
{
    typedef typename boost::range_iterator<VecVec const>::type VecVecItr;
    VecVecItr vvbeg = boost::begin(vv), vvend = boost::end(vv);
    VecVecItr beg = vvend - 2, end = vvend - 1;
    VecVecItr given_beg = vvbeg, given_end = vvbeg + 1;
    
    COLLINS_DEBUG_PRINT_SCORE(
      std::cerr << "insert ";
      BOOST_FOREACH(uint32_t tok, std::make_pair(*beg,*end)) {
          std::cerr << word(tok,dict) << ' ';
      }
      //std::copy(*beg,*end,std::ostream_iterator<uint32_t>(std::cerr, " "));
      std::cerr << "given ";
      BOOST_FOREACH(uint32_t tok,std::make_pair(*given_beg,*given_end)) {
          std::cerr << word(tok,dict) << ' ';
      }
      //std::copy(*given_beg,*given_end,std::ostream_iterator<uint32_t>(std::cerr, " "));
    );
    double scr = wb_backoff(get_counts(counts,*given_beg,*given_end,sep,*beg,*end),off,wbrate);
    ++given_end;
    for (; given_end != end; ++given_end) {
        COLLINS_DEBUG_PRINT_SCORE(
          std::cerr << "insert ";
          BOOST_FOREACH(uint32_t tok, std::make_pair(*beg,*end)) {
              std::cerr << word(tok,dict) << ' ';
          }
          //std::copy(*beg,*end,std::ostream_iterator<uint32_t>(std::cerr, " "));
          std::cerr << "given ";
          BOOST_FOREACH(uint32_t tok, std::make_pair(*given_beg,*given_end)) {
              std::cerr << word(tok,dict) << ' ';
          }
          //std::copy(*given_beg,*given_end,std::ostream_iterator<uint32_t>(std::cerr, " "));
        );
        scr = wb_smooth(get_counts(counts,*given_beg,*given_end,sep,*beg,*end),scr,wbrate);
    }
    return scr;
}

struct score_generic_op {
    score_generic_op(fixed_lm_trie const& counts, double wbrate, uint32_t sep, double& accum, reverse_dictionary const& dict)
    : counts(&counts)
    , wbrate(wbrate)
    , sep(sep)
    , accum(&accum)
    , dict(&dict) { *(this->accum) = 1.0; }
    fixed_lm_trie const* counts;
    double wbrate;
    uint32_t sep;
    double* accum;
    reverse_dictionary const* dict;
    template <class VecVec>
    void operator()(VecVec const& vv)
    {
        *accum *= score_generic(vv,*counts,0.0001,wbrate,sep,*dict);
    }
};

template <class Func>
double score_ruleinfo(Func f, idxrule const& rule, fixed_lm_trie const& counts, double wbrate, uint32_t sep, reverse_dictionary const& dict)
{
    double ret;
    score_generic_op op(counts,wbrate,sep,ret,dict);
    f(rule,op);
    return ret;
}

void make_deptag_given_rule_headtag( std::istream& in, lm_trie& counts, dictionary& dict )
{
    vector<string> v;
    while (getline(in,v)) {
        vector<boost::uint32_t> tv;
        tv.push_back(at(dict,v.at(0)));
        tv.push_back(boost::lexical_cast<size_t>(v.at(1)));
        vector<string>::iterator pos = v.begin() + 2;
        vector<string>::iterator end = v.end() - 1;
        for (; pos != end; ++pos) tv.push_back(at(dict,*pos));
        size_t count = boost::lexical_cast<size_t>(*pos++);
        assert (pos == v.end());
        insert(tv.begin(), tv.end() - 1, tv.end(), counts, count);
    }    
}

void make_dep_given_rule_head( std::istream& in, lm_trie& counts, dictionary& dict )
{
    using namespace std;
    vector<string> v;
    while (getline(in,v)) {
        vector<boost::uint32_t> tv;
        tv.push_back(at(dict,v.at(0)));
        tv.push_back(boost::lexical_cast<size_t>(v.at(1)));
        vector<string>::iterator pos = v.begin() + 2;
        vector<string>::iterator end = v.end() - 4;
        for (; pos != end; ++pos) tv.push_back(at(dict,*pos));
        tv.push_back(boost::lexical_cast<size_t>(*pos++));
        tv.push_back(at(dict,*pos++)); 
        tv.push_back(at(dict,*pos++)); 
        size_t count = boost::lexical_cast<size_t>(*pos++);
        assert (pos == v.end());
        insert(tv.begin(), tv.end() - 1, tv.end(), counts, count);
    }
}

trie_return get_dep_given_rule_head(fixed_lm_trie const& counts, uint32_t const* beg, uint32_t const* end)
{
    return get_counts(counts,beg,end-1,end);
}

void make_dep_given_rule( std::istream& in, lm_trie& counts, dictionary& dict )
{
    using namespace std;
    vector<string> v;
    while (getline(in,v)) {
        vector<boost::uint32_t> tv;
        tv.push_back(at(dict,v.at(0)));
        tv.push_back(boost::lexical_cast<size_t>(v.at(1)));
        vector<string>::iterator pos = v.begin() + 2;
        vector<string>::iterator end = v.end() - 3;
        for (; pos != end; ++pos) tv.push_back(at(dict,*pos));
        tv.push_back(boost::lexical_cast<size_t>(*pos++));
        tv.push_back(at(dict,*pos++)); 
        size_t count = boost::lexical_cast<size_t>(*pos++);
        assert (pos == v.end());
        
        insert(tv.begin(), tv.end() - 1, tv.end(), counts, count);
    }
}

void make_head_given_root(std::istream& in, lm_trie& counts, dictionary& dict)
{
    using namespace std;
    vector<string> v;
    while (getline(in,v)) {
        vector<boost::uint32_t> tv;
        tv.push_back(at(dict,v.at(0)));
        tv.push_back(at(dict,v.at(1)));
        size_t count = boost::lexical_cast<size_t>(v.at(2));
        insert(tv.begin(),tv.end() - 1, tv.end(),counts,count);
    }
}

trie_return get_dep_given_rule(fixed_lm_trie const& counts, uint32_t const* beg, uint32_t const* end)
{
    return get_counts(counts,beg,end-1,end);
}

void make_rule_given_root_head(std::istream& in, lm_trie& counts, dictionary& dict)
{
    using namespace std;
    vector<string> v;
    while (getline(in,v)) {
        vector<boost::uint32_t> tv;
        size_t count = boost::lexical_cast<size_t>(v.back());
        tv.push_back(at(dict,v.at(0)));
        tv.push_back(at(dict,v.at(1)));
        size_t hpos = boost::lexical_cast<size_t>(v.at(2));
        tv.push_back(hpos);
        if (hpos == 0) tv.push_back(at(dict,v.at(3)));
        else {
            vector<string>::iterator pos = v.begin() + 3;
            vector<string>::iterator end = v.end() - 1;
            for (; pos != end; ++pos) {
                tv.push_back(at(dict,*pos));
            }
        }
        insert(tv.begin(),tv.begin() + 2, tv.end(), counts, count);
    }
}

trie_return get_rule_given_root_head(fixed_lm_trie const& counts, uint32_t const* beg, uint32_t const* end)
{
    return get_counts(counts,beg,beg+2,end);
}

void make_rule_given_root_headtag(std::istream& in, lm_trie& counts, dictionary& dict)
{
    using namespace std;
    vector<string> v;
    while (getline(in,v)) {
        vector<boost::uint32_t> tv;
        size_t count = boost::lexical_cast<size_t>(v.back());
        tv.push_back(at(dict,v.at(0)));
        tv.push_back(at(dict,v.at(1)));
        size_t hpos = boost::lexical_cast<size_t>(v.at(2));
        tv.push_back(hpos);
        if (hpos == 0) tv.push_back(at(dict,v.at(3)));
        else {
            vector<string>::iterator pos = v.begin() + 3;
            vector<string>::iterator end = v.end() - 1;
            for (; pos != end; ++pos) {
                tv.push_back(at(dict,*pos));
            }
        }
        insert(tv.begin(),tv.begin() + 2, tv.end(), counts, count);
    }
}

trie_return get_rule_given_root_headtag(fixed_lm_trie const& counts, uint32_t const* beg, uint32_t const* end)
{
    return get_counts(counts,beg,beg+2,end);
}

void make_rule_given_root_headtag_headword(std::istream& in, lm_trie& counts, dictionary& dict)
{
    using namespace std;
    vector<string> v;
    while (getline(in,v)) {
        vector<boost::uint32_t> tv;
        size_t count = boost::lexical_cast<size_t>(v.back());
        tv.push_back(at(dict,v.at(0)));
        tv.push_back(at(dict,v.at(1)));
        tv.push_back(at(dict,v.at(2)));
        size_t hpos = boost::lexical_cast<size_t>(v.at(3));
        tv.push_back(hpos);
        if (hpos == 0) tv.push_back(at(dict,v.at(4)));
        else {
            vector<string>::iterator pos = v.begin() + 4;
            vector<string>::iterator end = v.end() - 1;
            for (; pos != end; ++pos) {
                tv.push_back(at(dict,*pos));
            }
        }
        insert(tv.begin(),tv.begin() + 3, tv.end(), counts, count);
    }
}

trie_return get_rule_given_root_headtag_headword(fixed_lm_trie const& counts, uint32_t const* beg, uint32_t const* end)
{
    return get_counts(counts,beg,beg+3,end);
}

void make_rule_given_root(std::istream& in,lm_trie& counts, dictionary& dict)
{
    using namespace std;
    vector<string> v;
    while (getline(in,v)) {
        vector<boost::uint32_t> tv;
        size_t count = boost::lexical_cast<size_t>(v.back());
        tv.push_back(at(dict,v.at(0)));
        size_t hpos = boost::lexical_cast<size_t>(v.at(1));
        tv.push_back(hpos);
        if (hpos == 0) tv.push_back(at(dict,v.at(2)));
        else {
            vector<string>::iterator pos = v.begin() + 2;
            vector<string>::iterator end = v.end() - 1;
            for (; pos != end; ++pos) {
                tv.push_back(at(dict,*pos));
            }
        }
        insert(tv.begin(),tv.begin() + 1, tv.end(), counts, count);
    }
}

trie_return get_rule_given_root(fixed_lm_trie const& counts, uint32_t const* beg, uint32_t const* end)
{
    return get_counts(counts,beg,beg+1,end);
}

trie_return get_head_given_root(fixed_lm_trie const& counts, uint32_t const* beg, uint32_t const* end)
{
    return get_counts(counts,beg,beg+1,end);
}

void add_to_file(lm_trie const& lm, ip::managed_mapped_file& mfile, output_type type)
{
    char_allocator calloc(mfile.get_segment_manager());
    std::string typestr = boost::lexical_cast<std::string>(type);
    std::cerr << "adding " << typestr.c_str() << " to file\n";
    mfile.construct<fixed_lm_trie>(typestr.c_str())(lm,lm.start(),calloc);
    std::cerr << "done\n";
}

void make_section(std::istream& in, std::string filename, output_type type)
{
    if (type == output_type::dictionary) {
        ip::managed_mapped_file mfile(ip::create_only,filename.c_str(),5UL*1024UL*1024UL*1024UL);
        make_dictionary(in,mfile);
    } else if (type == output_type::complete) {
        ip::managed_mapped_file::shrink_to_fit(filename.c_str());
    } else {
        bool add = true;
        lm_trie counts;
        ip::managed_mapped_file mfile(ip::open_only,filename.c_str());
        dictionary* dict = mfile.find<dictionary>("dictionary").first;
        reverse_dictionary* revdict = mfile.find<reverse_dictionary>("reverse-dictionary").first;
        uint32_t TOP = dict->find(dictkey("TOP"))->second;
        uint32_t STOP = dict->find(dictkey("STOP"))->second;
        #ifdef NO_ABSTRACTION
        if (type == output_type::rulemodel) ruleinfo_to_model(gen_label_given_concept(*revdict,TOP,STOP),in,counts,*dict,*revdict);
        else if (type == output_type::deptagmodel) {}
        else if (type == output_type::depwordmodel) ruleinfo_to_model(gen_depconcept_given_label_concept(*revdict,TOP,STOP),in,counts,*dict,*revdict);
        #else
        
        if (type == output_type::rulemodel) ruleinfo_to_model(gen_label_given_abstraction_concept(*revdict,TOP,STOP),in,counts,*dict,*revdict);
        else if (type == output_type::deptagmodel) ruleinfo_to_model(gen_depabstraction_given_label_abstraction_concept(*revdict,TOP,STOP),in,counts,*dict,*revdict);
        else if (type == output_type::depwordmodel) ruleinfo_to_model(gen_depconcept_given_depabstraction_labelabstraction_concept(*revdict,TOP,STOP),in,counts,*dict,*revdict);
        else if (type == output_type::print_rulemodel) {
            print_model(gen_label_given_abstraction_concept(*revdict,TOP,STOP),in,counts,*dict,*revdict,false);
            add = false;
        }
        else if (type == output_type::print_deptagmodel) {
            print_model(gen_depabstraction_given_label_abstraction_concept(*revdict,TOP,STOP),in,counts,*dict,*revdict,true);
            add = false;
        }
        else if (type == output_type::print_depwordmodel) {
            print_model(gen_depconcept_given_depabstraction_labelabstraction_concept(*revdict,TOP,STOP),in,counts,*dict,*revdict,true);
            add = false;
        }
        #endif
        else throw std::runtime_error("unsupported model");
        if (add) add_to_file(counts,mfile,type);
    }
}

struct lm_file {
private:
    fixed_lm_trie const* section(ip::managed_mapped_file& m, output_type type)
    {
        return m.find<fixed_lm_trie>(boost::lexical_cast<std::string>(type).c_str()).first;
    }
public:
    lm_file(std::string filename, double wb_rule_rate, double wb_tag_rate, double wb_word_rate, double wb_cfg_rate)
    : mfile(ip::open_only,filename.c_str())
    , rule_model(section(mfile,output_type::rulemodel))
    , deptag_model(section(mfile,output_type::deptagmodel))
    , depword_model(section(mfile,output_type::depwordmodel))
    , dict(mfile.find<dictionary>("dictionary").first)
    , revdict(mfile.find<reverse_dictionary>("reverse-dictionary").first)
    , top(dict->find(dictkey("TOP"))->second)
    , stop(dict->find(dictkey("STOP"))->second)
    , sep(dict->find(dictkey("~\t~"))->second)
    , wb_rule_rate(wb_rule_rate)
    , wb_tag_rate(wb_tag_rate)
    , wb_word_rate(wb_word_rate)
    , wb_cfg_rate(wb_cfg_rate)
    {}
    ip::managed_mapped_file mfile;
    fixed_lm_trie const* rule_model;
    fixed_lm_trie const* deptag_model;
    fixed_lm_trie const* depword_model;
    dictionary const* dict;
    reverse_dictionary const* revdict;
    uint32_t top;
    uint32_t stop;
    uint32_t sep;
    double wb_rule_rate;
    double wb_tag_rate;
    double wb_word_rate;
    double wb_cfg_rate;
};

std::ostream& operator << (std::ostream& out, trie_return& v)
{
    out << '(' << v.sum << ' ' << v.categories << ' ' << v.count << ')';
    return out;
}


double model::logprob_amr(idxrule const& r) const
{
    return 0;
}

double model::logprob_cfg(idxrule const& r) const
{
    return 0;
}

#ifdef NO_ABSTRACTION

double model::logprob_rule(idxrule const& r) const
{
    COLLINS_DEBUG_PRINT_SCORE(std::cerr << "logprob_rule:\n");
    return -std::log10( score_ruleinfo(
                          gen_label_given_concept(*file->revdict,file->top,file->stop)
                        , r
                        , *file->rule_model
                        , file->wb_rule_rate
                        , file->sep
                        , *file->revdict ) );
}

double model::logprob_tag(idxrule const& r) const
{
    return 0;
}

double model::logprob_word(idxrule const& r) const
{
    COLLINS_DEBUG_PRINT_SCORE(std::cerr << "logprob_word:\n");
    return -std::log10( score_ruleinfo(
                          gen_depconcept_given_label_concept(*file->revdict,file->top,file->stop)
                        , r
                        , *file->depword_model
                        , file->wb_word_rate
                        , file->sep
                        , *file->revdict ) );
}
#else
double model::logprob_rule(idxrule const& r) const
{
    COLLINS_DEBUG_PRINT_SCORE(std::cerr << "logprob_rule:\n");
    return -std::log10( score_ruleinfo(
                          gen_label_given_abstraction_concept(*file->revdict,file->top,file->stop)
                        , r
                        , *file->rule_model
                        , file->wb_rule_rate
                        , file->sep
                        , *file->revdict ) );
}

double model::logprob_tag(idxrule const& r) const
{
    COLLINS_DEBUG_PRINT_SCORE(std::cerr << "logprob_tag:\n");
    return -std::log10( score_ruleinfo(
                          gen_depabstraction_given_label_abstraction_concept(*file->revdict,file->top,file->stop)
                        , r
                        , *file->deptag_model
                        , file->wb_tag_rate
                        , file->sep
                        , *file->revdict ) );
}

double model::logprob_word(idxrule const& r) const
{
    COLLINS_DEBUG_PRINT_SCORE(std::cerr << "logprob_word:\n");
    return -std::log10( score_ruleinfo(
                          gen_depconcept_given_depabstraction_labelabstraction_concept(*file->revdict,file->top,file->stop)
                        , r
                        , *file->depword_model
                        , file->wb_word_rate
                        , file->sep
                        , *file->revdict ) );
}
#endif

model::model(std::string const& filename, double wb_rate)
: file(new lm_file(filename,wb_rate,wb_rate,wb_rate,wb_rate)) {}

model::model(std::string const& filename, double wb_rule_rate, double wb_tag_rate, double wb_word_rate, double wb_cfg_rate)
: file(new lm_file(filename,wb_rule_rate,wb_tag_rate,wb_word_rate,wb_cfg_rate)) {}

idxrule str2idxrule(strrule const& r, model const& lm)
{
    idxrule res;
    res.hpos = r.hpos;
    res.sz = r.sz;
    for (size_t x = 0; x != res.sz; ++x) {
        res.r[x].head = lm.word(r.r[x].head);
        res.r[x].tag = lm.word(r.r[x].tag);
        res.r[x].label = lm.word(r.r[x].label);
    }
    return res;
}

strrule idx2strrule(idxrule const& r, model const& lm)
{
    strrule res;
    res.hpos = r.hpos;
    res.sz = r.sz;
    for (size_t x = 0; x != res.sz; ++x) {
        res.r[x].head = lm.word(r.r[x].head);
        res.r[x].tag = lm.word(r.r[x].tag);
        res.r[x].label = lm.word(r.r[x].label);
    }
    return res;
}

namespace {
boost::regex digits("[0-9]+");
std::string atr("@");
}

std::string at_replace(std::string str)
{
  return regex_replace(str, digits, atr);
}




std::string model::word(uint32_t x) const
{
    return collins::word(x,*(file->revdict));
}

uint32_t model::word(std::string const& w) const
{
    dictionary::const_iterator ptr = file->dict->find(dictkey(w.c_str()));
    if (ptr == file->dict->end()) return file->dict->size();
    else return ptr->second;
}

strrule make_strrule(treelib::Tree::iterator_base const& node)
{
    treelib::Tree::sibling_iterator itr = node.begin(), end = node.end();
    std::vector<std::string> rhs;
    std::string lhs = node->getLabel();
    uint32_t hpos = node->getHeadPosition();
    for (; itr != end; ++itr) {
        std::string hword;
        std::string htag;
        if (itr->getIsTerminal()) {
            hword = itr->getLabel();
            htag = itr->getLabel();
        } else if (itr->getIsPreterminal()) {
            hword = itr.begin()->getLabel();
            htag = itr->getLabel();
        } else {
            hword = itr->getHeadword();
            htag = itr->getHeadPOS();
        }
        rhs.push_back(itr->getIsTerminal() ? at_replace(itr->getLabel()) : itr->getLabel());
        rhs.push_back(htag);
        rhs.push_back(at_replace(hword));
    }
    return strrule(lhs,hpos,rhs.begin(),rhs.end());
}

size_t bool2idx( sbmt::fat_syntax_rule const& r
               , sbmt::fat_syntax_rule::tree_node const& n
               , std::vector<bool> const& headdep
               , std::vector<int>& headpos )
{
    size_t x = &n - r.lhs_begin();
    if (n.children_begin() != n.children_end()) {
        if (n.children_begin()->lexical()) headpos[x] = 0;
        else {
            size_t h = 0;
            size_t nc = 0;
            BOOST_FOREACH(sbmt::fat_syntax_rule::tree_node const& cn, n.children()) {
                ++nc;
                size_t pos = bool2idx(r,cn,headdep,headpos);
                if (headdep[pos]) h = nc;
            }
            if (h == 0) h = nc;
            headpos[x] = h;
        }
    }
    return x;
}

std::vector<int> hpos_array(sbmt::fat_syntax_rule const& r, std::string const& s)
{
    std::vector<bool> v(r.lhs_size(),false);
    std::string::const_iterator itr = s.begin(), end = s.end();
    sbmt::fat_syntax_rule::lhs_preorder_iterator litr = r.lhs_begin();
    size_t x = 0;
    v[x] = true;
    ++litr;
    ++x;
    itr = next(itr, end);
    while (litr != r.lhs_end() and itr != end) {
        if (not litr->lexical()) {
            if (*itr == 'H') v[x] = true;
            else v[x] = false;
            //std::cerr << *itr;
            itr = next(itr,end);
        } else {
            //std::cerr << 'w';
        }
        ++x;
        ++litr;
    }
    //std::cerr << '\n';
    std::vector<int> ret(r.lhs_size(),0);
    bool2idx(r,*r.lhs_root(),v,ret);
    return ret;
}

int token_label::model_index()
{
    static const int di_ = ios_base::xalloc();
    return di_;
}

model const* get_model(std::ios_base& ios)
{
    return static_cast<model const*>(
               ios.pword(token_label::model_index())
           );
}

std::ostream& operator<< (std::ostream& os, token_label const& l)
{
    os.pword(token_label::model_index()) = (void*)l.m;
    return os;
}

token_label::token_label(model const& m) : m(&m) {}
token_label::token_label(model const* m) : m(m) {}

} // namespace collins

