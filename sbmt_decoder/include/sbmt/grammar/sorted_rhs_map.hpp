# if ! defined(SBMT__GRAMMAR__SORTED_RHS_MAP_HPP)
# define       SBMT__GRAMMAR__SORTED_RHS_MAP_HPP

# include <sbmt/grammar/grammar.hpp>
# include <sbmt/edge/edge.hpp>

# include <vector>
# include <utility>
# include <iostream>

# include <boost/multi_index_container.hpp>
# include <boost/multi_index/hashed_index.hpp>
# include <boost/iterator/iterator_adaptor.hpp>
# include <sbmt/hash/concrete_iterator.hpp>
# include <graehl/shared/map_from_set.hpp>

namespace sbmt {

namespace sig {

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
struct uniform_access {
    typedef typename Grammar::rule_type rule_type;
    
    uniform_access(Grammar const& g) : gram(&g) {}
      
    size_t rhs_size(rule_type const& r) const 
    { 
        return gram->rule_rhs_size(r);
    }
    
    template <class RHS>
    size_t rhs_size(RHS const& r) const 
    { 
        return r.size();
    }
    
    indexed_token rhs(rule_type const& r, size_t idx) const 
    {
        return gram->rule_rhs(r,idx);
    }
    
    template <class RHS>
    indexed_token rhs(RHS const& r, size_t idx) const 
    {
        return r[idx];
    }
private:
    Grammar const* gram;
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
struct first : private uniform_access<Grammar>{
    typedef uniform_access<Grammar> base_t;
    typedef indexed_token result_type;
    
    first(Grammar const& gram) : base_t(gram) {}
    
    template <class RHS>
    indexed_token operator()(RHS const& r) const
    {
        return base_t::rhs(r,0);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
struct equal : private uniform_access<Grammar> {
    typedef uniform_access<Grammar> base_t;
    
    equal(Grammar const& gram) : base_t(gram) {}
    
    template <class R1, class R2>
    bool operator()(R1 const& r1, R2 const& r2) const 
    {
        if (base_t::rhs_size(r1) != base_t::rhs_size(r2)) return false;
        else for (size_t x = 0; x != base_t::rhs_size(r1); ++x) {
            if (base_t::rhs(r1,x) != base_t::rhs(r2,x)) return false;
        }
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
struct hash : private uniform_access<Grammar> {
    typedef uniform_access<Grammar> base_t;
    
    hash(Grammar const& gram) : base_t(gram) {}
    
    typedef size_t result_type;
    
    template <class R>
    size_t operator()(R const& r) const
    {
        size_t sz = base_t::rhs_size(r);
        size_t retval = (sz << 2) | (sz >> (sizeof(sz) - 2));
        for(size_t x = 0; x != sz; ++x) {
            boost::hash_combine(retval,base_t::rhs(r,x));
        }
        return retval;
    }
};

} // namespace sig



template <class Grammar>
class sorted_rules_for_rhs {
public:
    typedef typename Grammar::rule_type rule_t;
 private:
    Grammar const& gram;
 public:        
    
    class iterator;

    typedef std::pair<iterator,iterator> rule_range_t;
    
private:       
    typedef std::pair<rule_t,score_t> rule_score_t;
    typedef std::vector<rule_score_t> vec_t;
    typedef std::pair<rule_t,vec_t> key_t;    // note: Michael used to have the very clever key-less (key is any member of the vec_t, i.e. the first).  but now we want to allow empty lists of rules corresponding to a rhs that had all its rules filtered.  anyway, it's faster to cache the key.  i'd almost recommend caching a fixed-type array copy of the rhs rather than making the hash/equal go through the grammar methods, but this code isn't that performance-critical
    
    // sort all the vectors...
    // sorting according to greater score...
    struct sort_op {
        bool operator()(rule_score_t const& p1, rule_score_t const& p2) const
        {
            return p1.second > p2.second;
        }
    };

    
    struct key {
        typedef rule_t result_type;
        rule_t const& operator()(key_t const& k) const
        {
            return k.first;
        }
    };
    
    typedef boost::multi_index_container<
        key_t
      , boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<
                key
              , sig::hash<Grammar>
              , sig::equal<Grammar>
            >
        >
    > rhs_map_t;
    rhs_map_t rhs_map;

    typedef graehl::map_from_set<rhs_map_t> map;
    
public:

    template <class RHS>
    vec_t & get_vec(RHS const& r,bool &is_new) 
    {
        /*
        std::pair<typename rhs_map_t::iterator,bool> i=rhs_map.insert(std::make_pair(r,vec_t()));
        is_new=i.second;
        return const_cast<vec_t &>(i.first->second);
        */
        return map::get(rhs_map,r,is_new);
    }
    
    template <class RHS>
    vec_t & get_vec(RHS const& r) 
    {
        /*
        return const_cast<vec_t &>(rhs_map.insert(std::make_pair(r,vec_t())).first->second);
        */
        return map::get(rhs_map,r);
/*
  bool is_new_dummy;
  return get_vec(r,is_new_dummy);
*/
    }

    
    rule_range_t 
    rules_from_vec(vec_t const& vec) const
    {            
        return rule_range_t(iterator(vec.begin()),iterator(vec.end()));
    }
    
    template <class RHS>
    rule_range_t 
    rules(RHS const& rhs) const
    {
/*
        typename rhs_map_t::iterator pos = rhs_map.find(rhs);
        if (pos != rhs_map.end())
            return rules_from_vec(pos->second);
     */
        vec_t const* p=map::get_ptr(rhs_map,rhs);
        if (p)
            return rules_from_vec(*p);
        else {
            iterator uninit;         
            return rule_range_t(uninit,uninit);             // to indicate empty.  //FIXME: uninit.  sentinel empty vec_t available?
        }
    }

    
    
    template <class ET>
    void insert(rule_t const& r,concrete_edge_factory<ET,Grammar> const& ef)
    {
        get_vec(r).push_back(std::make_pair(r,ef.rule_heuristic(gram,r)));
    }
    
    class iterator 
      : public boost::iterator_adaptor<
            iterator
          , typename vec_t::const_iterator
          , rule_t const
        > {
    public:
        iterator() {}
    private:
        friend class sorted_rules_for_rhs<Grammar>;
        iterator(typename vec_t::const_iterator p) 
          : iterator::iterator_adaptor_(p) {}
        friend class boost::iterator_core_access;
        rule_t const& dereference() const { return this->base()->first; }
    };
    
    // does not fill map!
    sorted_rules_for_rhs( Grammar const& g) :
        gram(g)
        ,rhs_map(
            boost::make_tuple(
                boost::make_tuple(
                    0
                    , key()
                    , sig::hash<Grammar>(g)
                    , sig::equal<Grammar>(g)
                    )
                )
            )
    {}

    void sort(vec_t &v) const
    {
        std::sort(v.begin(),v.end(),sort_op());
    }
    
    void sort() 
    {
        typename rhs_map_t::iterator itr = rhs_map.begin(), end = rhs_map.end();
        for (; itr != end; ++itr)  {
            sort(const_cast<vec_t&>(itr->second));
        }
    }

    

    // Filter f, rule_t r: score_t f(r).  exclude when is_zero, otherwise sort by combined rule score * f(r)
    template <class RuleIt,class Filter,class ET>
    void add_rules(vec_t &ret,RuleIt i,RuleIt end,Filter const& f
        ,concrete_edge_factory<ET,Grammar> const& ef) const 
    {
//        ret.clear();
        for (;i!=end;++i) { // replace w/ remove_copy_if(i,end,back_inserter(ret),not(accept)) ??
            rule_t const& r=*i;
            score_t s=f(r);
            if (!s.is_zero())
                ret.push_back(std::make_pair(r,s*ef.rule_heuristic(gram,r)));
        }
        sort(ret);
    }

    /// huge assumption: any range [i,end) with the same rule rhs ... will always be the same list.  we don't / can't verify this.  expect same list to be presented many times with different first_constituent (split point) in span_filter.  but only filter/sort once.  i,end: forward (not input) iterator
    template <class RuleIt,class Filter,class ET>
    rule_range_t
    lazy_filtered_rules(RuleIt i,RuleIt end,Filter const& f
                        ,concrete_edge_factory<ET,Grammar> const& ef)
    {
        assert(i!=end);
        bool is_new;
        vec_t &ret=get_vec(*i,is_new);
        if (is_new)
            add_rules(ret,i,end,f,ef);
        return rules_from_vec(ret);
    }
};

template <class Grammar>
class sorted_rhs_map {
public:
    
    typedef typename Grammar::rule_type rule_t;

    typedef sorted_rules_for_rhs<Grammar> sorted_t;
    
    typedef typename sorted_t::iterator iterator;
    typedef typename sorted_t::rule_range_t rule_range_t;

    template <class RHS>
    rule_range_t rules(RHS const& r) const
    {
        return rhs.rules(r);
    }
    
    
    template <class RHS>
    rule_range_t toplevel_rules(RHS const& r) const
    {
        return top_rhs.rules(r);
    }

    template <class ET>
    sorted_rhs_map( Grammar const& g
                  , concrete_edge_factory<ET,Grammar> const& ef )
        : rhs(g),top_rhs(g)
    {
        typename Grammar::rule_range rr = g.all_rules();
        typename Grammar::rule_iterator ritr = rr.begin(), rend = rr.end();
    
        // insert rules into appropriate vectors...
    
        for (; ritr != rend; ++ritr) {
            if (g.rule_lhs(*ritr).type() == top_token)
                top_rhs.insert(*ritr,ef);
            else
                rhs.insert(*ritr,ef);
        }

        rhs.sort();
        top_rhs.sort();        
    }
    
    
 private:
    sorted_t rhs,top_rhs;
};


////////////////////////////////////////////////////////////////////////////////

// caches questions of the form:
// given a token x, give me representative rules that have an rhs with signature
// starting with x, one-per-signature
template <class Grammar>
class signature_index_map {
public:
    typedef typename Grammar::rule_type rule_type;
private:
    typedef boost::multi_index_container<
        rule_type
      , boost::multi_index::indexed_by<
            boost::multi_index::hashed_non_unique< sig::first<Grammar> >
          , boost::multi_index::hashed_unique<
                boost::multi_index::identity<rule_type>
              , sig::hash<Grammar>
              , sig::equal<Grammar>
            >
        >
    > sig_map_t;
    
    sig_map_t sig_map;
    sig_map_t top_sig_map;
public:
    typedef typename sig_map_t::iterator iterator;
    signature_index_map(Grammar const& g);
    std::pair<iterator,iterator> reps(indexed_token const& t) const;
    std::pair<iterator,iterator> toplevel_reps(indexed_token const& t) const;
};

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
std::pair< typename signature_index_map<Grammar>::iterator
         , typename signature_index_map<Grammar>::iterator
         >
signature_index_map<Grammar>::reps(indexed_token const& tok) const
{
    return sig_map.equal_range(tok);
}

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
std::pair< typename signature_index_map<Grammar>::iterator
         , typename signature_index_map<Grammar>::iterator
         >
signature_index_map<Grammar>::toplevel_reps(indexed_token const& tok) const
{
    return top_sig_map.equal_range(tok);
}

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
signature_index_map<Grammar>::signature_index_map(Grammar const& gram)
: sig_map(
      boost::make_tuple(
          boost::make_tuple(
              0
            , sig::first<Grammar>(gram)
            , boost::hash<indexed_token>()
            , std::equal_to<indexed_token>()
          )
        , boost::make_tuple(
              0
            , boost::multi_index::identity<rule_type>()
            , sig::hash<Grammar>(gram)
            , sig::equal<Grammar>(gram)
          )
      )
  )
, top_sig_map(
      boost::make_tuple(
          boost::make_tuple(
              0
            , sig::first<Grammar>(gram)
            , boost::hash<indexed_token>()
            , std::equal_to<indexed_token>()
          )
        , boost::make_tuple(
              0
            , boost::multi_index::identity<rule_type>()
            , sig::hash<Grammar>(gram)
            , sig::equal<Grammar>(gram)
          )
      )
  )
{
    typename Grammar::rule_range rr = gram.all_rules();
    typename Grammar::rule_iterator ritr = rr.begin(), rend = rr.end();
    for (; ritr != rend; ++ritr) {
        if (gram.rule_rhs_size(*ritr) == 2) {
            if (gram.rule_lhs(*ritr).type() != top_token) sig_map.insert(*ritr); 
            else top_sig_map.insert(*ritr);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////


} // namespace sbmt

# endif //     SBMT__GRAMMAR__SORTED_RHS_MAP_HPP
