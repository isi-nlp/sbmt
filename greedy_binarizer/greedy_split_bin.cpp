# include <xrsparse/xrs.hpp>
# include <sbmt/token.hpp>
# include <gusc/trie/basic_trie.hpp>
# include <gusc/trie/trie_algo.hpp>
# include <gusc/trie/traverse_trie.hpp>
# include <boost/shared_ptr.hpp>
# include <queue>

struct bin_tree;
typedef boost::shared_ptr<bin_tree> bin_tree_ptr;

typedef gusc::basic_trie<sbmt::indexed_token, bin_tree_ptr> virt_map_t;



struct bin_tree {
    bool used;
    bin_tree() : used(false) {}
    bin_tree_ptr left;
    bin_tree_ptr right;
    virt_map_t::state label;
};



bool lexical(bin_tree_ptr bin)
{
    return not bin->left;
}


bin_tree_ptr make_leaf(sbmt::indexed_token const& tok, virt_map_t& vmap)
{
    sbmt::indexed_token array[1] = { tok };
    bin_tree_ptr bin(new bin_tree());
    bin->label = vmap.insert(array, array + 1,bin).second;
    return vmap.value(bin->label);
}

template <class Range>
bin_tree_ptr join_trees( Range const& rng
                       , bin_tree_ptr left
                       , bin_tree_ptr right
                       , virt_map_t& vmap )
{
    using boost::begin;
    using boost::end;
    bin_tree_ptr bin(new bin_tree());
    bin->left = left;
    bin->right = right;
    bin->label = vmap.insert(begin(rng),end(rng),bin).second;
    bin->left->used = true;
    bin->right->used = true;
    assert(bin);
    return bin;
}

template <class Range>
boost::shared_ptr<bin_tree>
nt_bin(Range const& rng, virt_map_t& vmap)
{
    using boost::begin;
    using boost::end;
    using boost::make_iterator_range;
    using boost::distance;
    typedef typename boost::range_iterator<Range const>::type iterator;
    
    typedef gusc::trie_find_result<virt_map_t,iterator> trie_pos;
    
    trie_pos pos = trie_find(vmap,begin(rng),end(rng));
    if (pos.found) {
        assert(vmap.value(pos.state));
        return vmap.value(pos.state);
    }
    if (distance(rng) == 1) {
        return make_leaf(*begin(rng),vmap);
    }
    
    
    bool backup = false;
    //iterator backup_mdpt = begin(rng) + (distance(rng) / 2);
    iterator backup_mdpt = end(rng) - 1;
    iterator mdpt = end(rng) - 1;
    for(; mdpt != begin(rng); --mdpt) {
        trie_pos p1 = trie_find(vmap,begin(rng),mdpt);
        trie_pos p2 = trie_find(vmap,mdpt,end(rng));
        if (p1.found and p2.found) {
            return join_trees(rng,vmap.value(p1.state),vmap.value(p2.state),vmap);
        } else if ((p1.found or p2.found) and not backup) {
            backup = true;
            backup_mdpt = mdpt;
        }
    }
    
    bin_tree_ptr rt = 
           join_trees( rng
                     , nt_bin(make_iterator_range(begin(rng),backup_mdpt),vmap)
                     , nt_bin(make_iterator_range(backup_mdpt,end(rng)),vmap)
                     , vmap 
                     )
                     ;
    assert(rt);
    return rt;
}

template <class Range>
boost::shared_ptr<bin_tree>
bin_bin(Range const& rng, virt_map_t& vmap)
{
    typedef typename boost::range_value<Range const>::type inner_range;
    typedef typename boost::range_iterator<inner_range const>::type inner_iterator;
    
    using boost::begin;
    using boost::end;
    using boost::make_iterator_range;
    using boost::distance;
    typedef typename boost::range_iterator<Range const>::type iterator;
    
    typedef gusc::trie_find_result<virt_map_t,inner_iterator> trie_pos;
    
    //int x = begin(rng);
    //int y = begin(begin(rng));
    
    inner_range irng(begin(*begin(rng)),end(*(end(rng)-1)));
    
    trie_pos pos = trie_find(vmap,begin(irng),end(irng));
    if (pos.found) {
        assert(vmap.value(pos.state));
        return vmap.value(pos.state);
    }
    if (distance(rng) == 1) {
        return nt_bin(*begin(rng),vmap);
    }
    
    
    
    iterator backup_mdpt = end(rng) - 1;
    //iterator backup_mdpt = begin(rng) + (distance(rng) / 2);
    bool backup = false;
    iterator mdpt = end(rng) - 1;
    for(; mdpt != begin(rng); --mdpt) {
        trie_pos p1 = trie_find(vmap,begin(irng),begin(*mdpt));
        trie_pos p2 = trie_find(vmap,begin(*mdpt),end(irng));
        if (p1.found and p2.found) {
            return join_trees(irng,vmap.value(p1.state),vmap.value(p2.state),vmap);
        } else if ((p1.found or p2.found) and not backup) {
            backup = true;
            backup_mdpt = mdpt;
        }
    }
    
    bin_tree_ptr rt = 
           join_trees( irng
                     , bin_bin(make_iterator_range(begin(rng),backup_mdpt),vmap)
                     , bin_bin(make_iterator_range(backup_mdpt,end(rng)),vmap)
                     , vmap 
                     )
                     ;
    assert(rt);
    return rt;
}

template <class Range>
std::vector<Range> lex_nt_ranges(Range const& range) 
{
    std::vector<Range> out;
    typedef typename boost::range_iterator<Range const>::type iterator;
    iterator itr = boost::begin(range);
    while (itr != boost::end(range)) {
        iterator jtr = itr;
        if (is_lexical(*itr)) {
            ++jtr;
            out.push_back(Range(itr,jtr));
        } else {
            while (jtr != boost::end(range) and not is_lexical(*jtr)) {
                ++jtr;
            }
            out.push_back(Range(itr,jtr));
        }
        itr = jtr;
    }
    return out;
}

template <class Range>
std::deque<bin_tree_ptr> init_bin(Range const& rng, virt_map_t& vmap)
{
    std::deque<bin_tree_ptr> q;
    typedef typename boost::range_iterator<Range const>::type iterator;
    iterator itr = boost::begin(rng);
    iterator end = boost::end(rng);
    while (itr != end) {
        if (not is_lexical(*itr)) {
        
            iterator fin = itr; ++fin;
            while (fin != end and not is_lexical(*fin)) ++fin;
            bin_tree_ptr btp = nt_bin(boost::make_iterator_range(itr,fin),vmap);
            if (fin == end and itr == boost::begin(rng) and boost::distance(rng) > 1) {
                q.push_back(btp->left);
                q.push_back(btp->right);
            } else {
                q.push_back(btp);
            }
            itr = fin;
        } else {
            sbmt::indexed_token rng[1] = { *itr };
            gusc::trie_find_result<virt_map_t,sbmt::indexed_token*>
                pos = trie_find(vmap, rng, rng + 1);
            if (pos.found) q.push_back(vmap.value(pos.state));
            else q.push_back(make_leaf(*itr,vmap));
            ++itr;
        }
    }
    return q;
}

std::vector<sbmt::indexed_token>
coverage(virt_map_t const& vmap, bin_tree_ptr p1, bin_tree_ptr p2 = bin_tree_ptr())
{
    std::vector<sbmt::indexed_token> v;
    trie_path(vmap, p1->label,std::back_inserter(v));
    if (p2) {
        trie_path(vmap,p2->label,std::back_inserter(v));
    }
    return v;
}

std::vector<sbmt::indexed_token>
rule_rhs(rule_data const& rd, sbmt::in_memory_dictionary& dict)
{
    std::vector<sbmt::indexed_token> v;
    BOOST_FOREACH(rhs_node const& rn, rd.rhs) {
        if (rn.indexed) v.push_back(dict.tag(label(rd,rn)));
        else v.push_back(dict.foreign_word(label(rd,rn)));
    }
    return v;
}

struct print_lbl {
    bin_tree_ptr b;
    virt_map_t const* vmap;
    sbmt::in_memory_dictionary* dict;
    print_lbl(bin_tree_ptr b, virt_map_t const& vmap, sbmt::in_memory_dictionary& dict) 
      : b(b)
      , vmap(&vmap)
      , dict(&dict) {}
    static void prnt(std::ostream& os, sbmt::indexed_token tok, sbmt::in_memory_dictionary& dict)
    {
        if (is_lexical(tok)) os << '"' << dict.label(tok) << '"';
        else os << dict.label(tok);
    }
    friend std::ostream& operator<< (std::ostream& os, print_lbl const& rd)
    {
        std::vector<sbmt::indexed_token> r;
        trie_path(*rd.vmap,rd.b->label,std::back_inserter(r));
        if (r.size() == 1) {
            print_lbl::prnt(os,r[0],*rd.dict);
        } else {
            os << "V[_";
            BOOST_FOREACH(sbmt::indexed_token tok, r) {
                print_lbl::prnt(os,tok,*rd.dict);
                os << "_";
            }
            os << "]";
        }
        return os;
    }
};

struct print_rhs {
    bin_tree_ptr b;
    virt_map_t const* vmap;
    sbmt::in_memory_dictionary* dict;
    print_rhs(bin_tree_ptr b, virt_map_t const& vmap, sbmt::in_memory_dictionary& dict) 
      : b(b)
      , vmap(&vmap)
      , dict(&dict) {}
      
    friend std::ostream& operator<< (std::ostream& os, print_rhs const& rd)
    {
        os << print_lbl(rd.b->left,*rd.vmap,*rd.dict);
        if (rd.b->right) { 
           os << ' ' << print_lbl(rd.b->right,*rd.vmap,*rd.dict);
        }
        return os;
    }
};

template <class Range>
std::vector<bin_tree_ptr>
bin2(Range const& rng, virt_map_t& vmap)
{
    std::vector<bin_tree_ptr> ret;
    
    typedef typename boost::range_iterator<Range const>::type iterator;
    typedef boost::iterator_range<iterator> R;
    R r(boost::begin(rng),boost::end(rng));
    std::vector<R> splits = lex_nt_ranges(r);
    bin_tree_ptr bptr = bin_bin(splits,vmap);
    //bin_tree_ptr bptr = nt_bin(rng,vmap);
    if (bptr->left and bptr->right) {
        ret.push_back(bptr->left);
        bptr->left->used = true;
        ret.push_back(bptr->right);
        bptr->right->used = true;
    } else {
        ret.push_back(bptr);
    }
    return ret;
}

template <class Range>
std::vector<bin_tree_ptr>
bin(Range const& rng, virt_map_t& vmap)
{

    std::vector<bin_tree_ptr> v;
    std::deque<bin_tree_ptr> q = init_bin(rng, vmap);
    while (not q.empty()) {
        bin_tree_ptr p1 = q.front(); q.pop_front();
        if (q.empty()) {
            v.push_back(p1);
            return v;
        }
        bin_tree_ptr p2 = q.front(); q.pop_front();
        if (q.empty()) {
            v.push_back(p1);
            p1->used = true;
            v.push_back(p2);
            p2->used = true;
            return v;
        }
        std::vector<sbmt::indexed_token> v = coverage(vmap,p1,p2);
        typedef std::vector<sbmt::indexed_token>::iterator iterator;
        gusc::trie_find_result<virt_map_t,iterator>
            pos = trie_find(vmap,v.begin(),v.end());
        if (pos.found) q.push_front(vmap.value(pos.state));
        else q.push_front(join_trees(v,p1,p2,vmap));
    }
    throw std::logic_error("mustnt reach");
    return v;
}

struct print_virtuals_visitor {
    std::ostream* out;
    sbmt::in_memory_dictionary* dict;

    void at_state(virt_map_t const& vmap, virt_map_t::state s) 
    {
        bin_tree_ptr b = vmap.value(s);
        if (b and b->left and b->right and b->used) {
            *out << "V: " << print_lbl(b,vmap,*dict) << " -> " << print_rhs(b,vmap,*dict) << "\n";
        }
    }
    void begin_children(virt_map_t const& vmap, virt_map_t::state s) {}
    void end_children(virt_map_t const& vmap, virt_map_t::state s) {}
};

int main()
{
    std::ios::sync_with_stdio(false);
    std::string line;
    sbmt::in_memory_dictionary dict;
    virt_map_t vmap;
    std::cout << "BRF version 2\n";
    while (getline(std::cin,line)) {
        rule_data rd = parse_xrs(line);
        std::vector<sbmt::indexed_token> rhs = rule_rhs(rd,dict);
        std::vector<bin_tree_ptr> bv = bin2(rhs,vmap);
        std::cout << "X: "<< line << std::endl;
        std::cout << "V: "<< rd.lhs[0].label << " -> ";
        BOOST_FOREACH(bin_tree_ptr b, bv) {
            std::cout << print_lbl(b,vmap,dict) << ' ';
        }
        std::cout << "\n";
    }
    
    print_virtuals_visitor vis = {&std::cout, &dict};
    
    traverse_trie(vmap,vis);
    
}