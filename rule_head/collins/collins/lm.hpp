# include <boost/enum.hpp>
# include <iosfwd>
# include <string>
# include <cstring> // bug: boost/enum.hpp needs strcmp
# include <stdexcept>
# include <boost/shared_ptr.hpp>
# include <treelib/Tree.h>
# include <sbmt/grammar/syntax_rule.hpp>
# include <boost/foreach.hpp>


namespace collins {
std::string at_replace(std::string str);

BOOST_ENUM_VALUES(
    output_type
  , const char*
  , (dictionary)("dictionary")
    (rulemodel)("rulemodel")
    (deptagmodel)("deptagmodel")
    (depwordmodel)("depwordmodel")
    (print_rulemodel)("print_rulemodel")
    (print_deptagmodel)("print_deptagmodel")
    (print_depwordmodel)("print_depwordmodel")
    (amrlm)("amrlm")
    (ruleheadroot)("ruleheadroot")
    (ruleroot)("ruleroot")
    (deprulehead)("deprulehead")
    (deprule)("deprule")
    (headroot)("headroot")
    (ruleroottagword)("ruleroottagword")
    (ruleroottag)("ruleroottag")
    (tagruletagword)("tagruletagword")
    (tagruletag)("tagruletag")
    (tagrule)("tagrule")
    (tagvar)("tagvar")
    (wordtagruletagword)("wordtagruletagword")
    (wordtagruletag)("wordtagruletag")
    (wordtagvar)("wordtagvar")
    (wordtag)("wordtag")
    (ruleinfo)("ruleinfo")
    (neural)("neural")
    (amr_neural)("amr_neural")
    (complete)("complete")
);

void make_section(std::istream&, std::string filename, output_type type);

template <class Token, int RHS=3>
struct rule {
    struct variable {
        Token label;
        Token tag;
        Token head;
        variable() {}
        variable(Token label, Token tag, Token head)
        : label(label)
        , tag(tag)
        , head(head) {}
    };
    uint32_t hpos;
    variable r[RHS+1];
    uint32_t sz;
    
    rule() : hpos(0), sz(0) {}

    template <class Iterator>
    rule(Token lhs, uint32_t hpos, Iterator itr, Iterator end)
    : hpos(hpos)
    , sz(0)
    {
        if (itr == end) throw std::runtime_error("bad input for collins::rule");
        Token wd, hd, tg; 
        r[sz++] = variable(lhs,lhs,lhs);
        while (itr != end) {
            wd = *itr++;
            if (itr == end) throw std::runtime_error("bad input for collins::rule");
            tg = *itr++;
            if (itr == end) throw std::runtime_error("bad input for collins::rule");
            hd = *itr++;
            r[sz++] = variable(wd,tg,hd);
        }
        if (hpos > sz) throw std::runtime_error("bad input for collins::rule");
        if (hpos > 0) {
            r[0].head = r[hpos].head;
            r[0].tag = r[hpos].tag;
        } else {
            r[0].head = r[1].head;
            r[0].tag = r[1].tag;
        }
    }
    
    rule(rule const& other)
    : hpos(other.hpos)
    , sz(other.sz)
    {
        for (size_t x = 0; x != sz; ++x) r[x] = other.r[x];
    }
    
    rule& operator=(rule const& other) 
    {
        if (this != &other) {
            hpos = other.hpos;
            sz = other.sz;
            for (size_t x = 0; x != sz; ++x) r[x] = other.r[x];
        }
        return *this;
    }
    
    variable const& lhs() const { return r[0]; }
    variable const& rhs(size_t x) const { return r[x]; }
    std::pair<variable const*, variable const*> 
    rhs() const { return std::make_pair(&r[0] + 1, &r[0] + sz); }
};

typedef rule<uint32_t> idxrule;
typedef rule<std::string> strrule;

struct lm_file;

struct model {
    model(std::string const& filename, double wb_rate = 1.0);
    model(std::string const& filename, double wb_rh_rate, double wb_dr_rate, double wb_hr_rate, double wb_cf_rate);
    double logprob_rule(idxrule const& r) const;
    double logprob_tag(idxrule const& r) const;
    double logprob_word(idxrule const& r) const;
    double logprob_cfg(idxrule const& r) const;
    double logprob_amr(idxrule const& r) const;
    double logprob(idxrule const& r) const { return logprob_rule(r) + logprob_tag(r) + logprob_word(r); }
    uint32_t word(std::string const& w) const;
    std::string word(uint32_t w) const;
private:
    boost::shared_ptr<lm_file> file;
};

model const* get_model(std::ios_base&);

class token_label;
std::ostream& operator << (std::ostream&, token_label const&);

////////////////////////////////////////////////////////////////////////////////
///
/// ties a dictionary to an iostream for using standard iostream functions with
/// indexed_tokens
///
////////////////////////////////////////////////////////////////////////////////
class token_label {
public:
    explicit token_label(model const& dict);
    explicit token_label(model const* dict);
    struct saver : public boost::io::ios_pword_saver {
        saver(std::ios_base&);
    };
private:
    model const* m;
    static int model_index();

    friend model const* get_model(std::ios_base&);
    friend std::ostream& operator << (std::ostream&, token_label const&);
};

template <class Token>
struct map_token {
    map_token(model const* m) : m(m) {}
    Token operator()(Token const& t) const { return t;}
private:
    model const* m;
};

template <>
struct map_token<uint32_t> {
    map_token(model const* m) : m(m) {}
    std::string operator()(uint32_t const& t) const 
    { 
        if (m) return m->word(t);
        else return boost::lexical_cast<std::string>(t);
    } 
private:
    model const* m;
};

template <class Token>
std::istream& operator >> (std::istream& in, rule<Token>& r)
{
    in >> r.r[0].label >> r.hpos;
    int x = 1;
    while (in >> r.r[x].label >> r.r[x].tag >> r.r[x].head) ++x;
    r.sz = x;
    if (r.hpos == 0) {
        r.r[0].head = r.r[1].head;
        r.r[0].tag = r.r[1].tag;
    }
    else {
        r.r[0].head = r.r[r.hpos].head;
        r.r[0].tag  = r.r[r.hpos].tag;
    }
    return in;
}

template <class Token>
std::ostream& operator << (std::ostream& out, rule<Token> const& r)
{
    map_token<Token> mp(get_model(out));
    out << mp(r.r[0].label) << ' ' << r.hpos;
    for (size_t x = 1; x != r.sz; ++x) {
        out << ' ' << mp(r.r[x].label) << ' ' << mp(r.r[x].tag) << ' ' << mp(r.r[x].head);
    }
    return out;
}

typedef token_label::saver token_format_saver;

strrule make_strrule(treelib::Tree::iterator_base const& node);

idxrule str2idxrule(strrule const& r, model const& lm);
strrule idx2strrule(idxrule const& r, model const& lm);

inline
std::string::const_iterator
next( std::string::const_iterator pos, std::string::const_iterator end )
{
    ++pos;
    while ( pos != end and
            *pos != 'H' and
            //*pos != '(' and
            //*pos != ')' and
            *pos != 'D'
          ) { ++pos; }
    return pos;
}

template <class T, class A, class C, class O>
size_t bool2idx( sbmt::syntax_rule<T,A,C,O> const& r
               , typename sbmt::syntax_rule<T,A,C,O>::tree_node const& n
               , std::vector<bool> const& headdep
               , std::vector<int>& headpos )
{
    size_t x = &n - r.lhs_begin();
    if (n.children_begin() != n.children_end()) {
        if (n.children_begin()->lexical()) headpos[x] = 0;
        else {
            size_t h = 0;
            size_t nc = 0;
            typedef typename sbmt::syntax_rule<T,A,C,O>::tree_node tree_node;
            BOOST_FOREACH(tree_node const& cn, n.children()) {
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

template <class T, class A, class C, class O>
std::vector<int> hpos_array(sbmt::syntax_rule<T,A,C,O> const& r, std::string const& s)
{
    std::vector<bool> v(r.lhs_size(),false);
    std::string::const_iterator itr = s.begin(), end = s.end();
    typename sbmt::syntax_rule<T,A,C,O>::lhs_preorder_iterator litr = r.lhs_begin();
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

} // namespace collins

