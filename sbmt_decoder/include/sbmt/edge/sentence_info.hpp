# ifndef   SBMT_EDGE_SENTENCE_INFO_HPP
# define   SBMT_EDGE_SENTENCE_INFO_HPP

# include <sbmt/edge/info_base.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/edge/constituent.hpp>
# include <sbmt/sentence.hpp>
# include <sbmt/grammar/lm_string.hpp>
# include <sbmt/logmath.hpp>

# include <boost/iterator/iterator_facade.hpp>
# include <boost/range.hpp>

namespace sbmt {

template <class ET>
inline bool edge_scoreable(ET const& e)
{
    return e.root().type() == foreign_token or
           e.get_info().scoreable();
}

template <class ET>
inline bool edges_scoreable(ET const& e1, ET const& e2)
{
    return edge_scoreable(e1) and edge_scoreable(e2);
}

template <class ET, class OutT>
OutT scoreable_children(ET const& e, OutT out)
{
    if (e.root().type() == foreign_token) return out;
    else if (e.get_info().scoreable()) {
        *out = &e;
        ++out;
    }
    else {
        if (e.has_first_child()) {
            out = scoreable_children(e.first_child(),out);
        } 
        if (e.has_second_child()) {
            out = scoreable_children(e.second_child(),out);
        }
    }
    return out;
}

template <class ET>
std::vector< ET const* > scoreable_children(ET const& e)
{
    std::vector<ET const* > vec;
    vec.reserve(10);
    scoreable_children(e, std::back_inserter(vec));
    return vec;
}

template <class ET>
std::vector< ET const* > 
scoreable_children(ET const& e1, ET const& e2)
{
    std::vector<ET const* > vec;
    vec.reserve(10);
    scoreable_children(e1,std::back_inserter(vec));
    scoreable_children(e2,std::back_inserter(vec));
    return vec;
} 

////////////////////////////////////////////////////////////////////////////////
///
///  span-strings are like lm strings except for the following:
///  # lexical items are replaced with spans
///  # two spans may not be adjacent to each other
///  # spans must have non-zero length
///
///  the strings "x0 [2,5] x1", "x1 x0", "[1,3]" "x1 [0,2]" are all span-strings
///  span-strings are used in place of lm-strings for force-decoding
///
////////////////////////////////////////////////////////////////////////////////
class span_string {
public:
    struct token {
        typedef boost::variant<span_t,unsigned int> variant_t;
        variant_t var;
    
        span_t const& get_span() const;
        unsigned int  get_index() const;
        
        bool is_span() const { return var.which() == 0; }
        bool is_index() const { return var.which() == 1; }
        
        token() {};
        token(span_t const& s);
        token(unsigned int x);
        bool operator==(token const& other)  const 
        { return var == other.var; }
        bool operator==(unsigned int other)  const 
        { return var == variant_t(other); }
        bool operator==(span_t const& other) const 
        { return var == variant_t(other); }
        bool operator!=(token const& other)  const 
        { return !(var == other.var); }
        bool operator!=(span_t const& other) const 
        { return !(var == variant_t(other)); }
        bool operator!=(unsigned int other)  const 
        { return !(var == variant_t(other)); }
    };
    

    typedef std::vector<token>::const_iterator
            iterator;
    typedef iterator const_iterator;

    span_string(){}
    
    span_string(std::string const& str);

    token const& at(std::size_t idx) const 
    { 
       return vec.at(idx); 
    }

    token const& operator[](std::size_t idx) const 
    { 
       return vec[idx];
    }
    
    void pop_back();
    void pop_back(span_t const& s);
    void push_back(token const& t);
    void push_front(token const& t);
    token const& front() const { return vec.front(); }
    token const& back() const { return vec.back(); }

    std::size_t size() const { return vec.size(); }

    iterator begin() const { return vec.begin(); }
    iterator end() const { return vec.end(); }

    bool operator==(span_string const& o) const { return vec == o.vec; }
    bool operator!=(span_string const& o) const { return !(*this == o); }

private:
    std::vector<token> vec;

    template <class T>
    friend void swap(span_string&, span_string&);
};

////////////////////////////////////////////////////////////////////////////////
///
///  represents a family of spans:
///  [n,m], (?,m] , [n,?), (?,?), where "?" refers to an open-ended span.
///  
///  used to represent the family of spans acceptable to a span_string
///
////////////////////////////////////////////////////////////////////////////////
class span_sig
{
public:
    enum state { open_none  = 0
               , open_left  = 1
               , open_right = 2
               , open_both  = 3 };
    
    bool match(span_t const& s) const
    {
        switch (st) {
            case open_both:  return s.left() >= lt and s.right() <= rt; break;
            case open_left:  return s.left() >= lt and s.right() == rt; break;
            case open_right: return s.left() == lt and s.right() <= rt; break;
            case open_none:  return s.left() == lt and s.right() == rt; break;
        }
        return false;
    }
    
    bool operator()(span_t const& s) const { return match(s); }
    
    bool operator==(span_sig const& o) const
    {
        return st == o.st and lt == o.lt and rt == o.rt;
    }
    
    bool operator!=(span_sig const& o) const
    {
        return !(*this == o);
    }
    
    span_sig(span_index_t lt, span_index_t rt, state st) 
    : st(st), lt(lt), rt(rt) {}

private:    
    state  st;
    span_index_t lt, rt;
    friend std::ostream& operator<<(std::ostream&, span_sig const&);
};


std::ostream& operator<<(std::ostream& out, span_sig const& s);

////////////////////////////////////////////////////////////////////////////////
///
///  determines signature of a given variable in a span_string.
///  eg:  signature("x0 [4,7] x1 [10,11]", 0) == (?,4]
///       signature("x0 [4,7] x1 [10,11]", 1) == [7,10]
///
////////////////////////////////////////////////////////////////////////////////
span_sig signature(span_string const& sstr, unsigned int index);

/// if span_string is a binary indexed string, returns the gap between the 
/// two indices.  if its unary, the second return value is 0.
/// if the span_string is order reversing, then the second result is -1
/// otherwise, its positive 1
std::pair<int,int> gap(span_string const& sstr);

template <class C, class T>
std::basic_ostream<C,T>&
operator << (std::basic_ostream<C,T>& out, span_string::token const& stok)
{
    return out << stok.var;
}

template <class C, class T>
std::basic_ostream<C,T>&
operator << (std::basic_ostream<C,T>& out, span_string const& sstr)
{
    span_string::iterator itr = sstr.begin(),
                          end = sstr.end();
    if (itr == end) return out;
    out << *itr; ++itr;
    for (;itr != end; ++itr) out << " " << *itr;
    return out;
}

////////////////////////////////////////////////////////////////////////////////
///
///  sentence_info is primarily for forced-decoding, to be used with the 
///  force_sentence span_filter
///
////////////////////////////////////////////////////////////////////////////////
class sentence_info : public info_base<sentence_info>
{
    typedef sentence_info self_type;
    std::pair<span_t,bool> s;
    //bool scoreable_;
public:

    bool equal_to(sentence_info const& other) const 
    { 
        return s.first == other.s.first and
               s.second == other.s.second; 
    }
    
    std::size_t hash_value() const
    { boost::hash<span_t> hasher; return hasher(s.first); }
    void set_null() { s = std::make_pair(span_t(0,0),true); }
    sentence_info() 
    : s(span_t(0,0),true) {}
    sentence_info(std::pair<span_t,bool> const& s)
    : s(s) {}
    sentence_info& operator=(std::pair<span_t,bool> const& s)
    { this->s = s; return *this; }
    span_t const& espan() const { return s.first; }
    bool valid() const { return s.second; }
    //bool is_scoreable() const { return scoreable_; }
    
    //friend sentence_info unscoreable_sentence_info();
};

//sentence_info unscoreable_sentence_info();

struct espan {
    typedef span_t const& result_type;
    template <class ET>
    span_t const& operator()(ET const* e) const
    {
        return e->info().espan();
    }
};


struct constit2espan {
    typedef span_t result_type;
    template <class Info>
    span_t operator()(constituent<Info> const& c) const
    {
        return c.info()->espan();
    }
};

template <class TF>
void print(std::ostream& out, sentence_info const& i, TF& tf) 
{
    out << '[' << i.espan() << ',' << i.valid() << ']';
}

////////////////////////////////////////////////////////////////////////////////

indexed_sentence 
join_sentence( indexed_lm_string const& lmstr
             , indexed_sentence const& str );

indexed_sentence 
join_sentence( indexed_lm_string const& lmstr
             , indexed_sentence const& str1 
             , indexed_sentence const& str2 );
             
template <class ItrT> std::pair<span_t,bool>
join_span_range(span_string const& sstr, ItrT fbegin, ItrT fend);

std::pair<span_t,bool> 
join_spans(span_string const& sstr, span_t const& s1, span_t const& s2);
                    
std::pair<span_t,bool> 
join_spans(span_string const& sstr, span_t const& s);
             
////////////////////////////////////////////////////////////////////////////////
             
class lazy_join_sentence_iterator
: public boost::iterator_facade<
            lazy_join_sentence_iterator
          , indexed_token const
          , boost::forward_traversal_tag
         >   
{
    enum state { sent0_active = 0, sent1_active = 1, lm_active = 2 };
    
    typedef indexed_sentence::iterator                 sentence_itr_t;
    typedef std::pair<sentence_itr_t,sentence_itr_t>   sentence_itr_pair_t;
    typedef indexed_lm_string::iterator                lm_string_itr_t;
    typedef std::pair<lm_string_itr_t,lm_string_itr_t> lm_string_itr_pair_t;
    
    lm_string_itr_pair_t lm_str;
    sentence_itr_pair_t  sent0;
    sentence_itr_pair_t  sent1;
    state                s;
    
    indexed_token const& dereference() const;
    void increment();
    bool equal(lazy_join_sentence_iterator const&) const;
    void init();
    
    friend class boost::iterator_core_access;
public:
    lazy_join_sentence_iterator( indexed_lm_string const&
                               , indexed_sentence const&
                               , indexed_sentence const& 
                               , bool start = true );
    lazy_join_sentence_iterator( indexed_lm_string const&
                               , indexed_sentence const&
                               , bool start = true );
};

////////////////////////////////////////////////////////////////////////////////

std::pair<lazy_join_sentence_iterator, lazy_join_sentence_iterator>
lazy_join_sentence( indexed_lm_string const& lmstr
                  , indexed_sentence const& str );

std::pair<lazy_join_sentence_iterator, lazy_join_sentence_iterator>
lazy_join_sentence( indexed_lm_string const& lmstr
                  , indexed_sentence const& str1
                  , indexed_sentence const& str2 );
/*

*/

/*
template <class IT>
std::pair<lazy_join_sentence_iterator, lazy_join_sentence_iterator>
lazy_join_sentence( indexed_lm_string const& lmstr
                  , edge<IT> const& e1
                  , edge<IT> const& e2 )
{
    if (e1.root().type() == foreign_token) {
        return lazy_join_sentence( 
                   lmstr
                 , e2.template cast_info<sentence_info>().sentence()
               );          
    } else if (e2.root().type() == foreign_token) {
        return lazy_join_sentence( 
                   lmstr
                 , e1.template cast_info<sentence_info>().sentence()
               );
    } else {
        return lazy_join_sentence(
                   lmstr
                 , e1.template cast_info<sentence_info>().sentence()
                 , e2.template cast_info<sentence_info>().sentence()
               );
    }
}
*/

////////////////////////////////////////////////////////////////////////////////

class sentence_info_factory
{
public:
    typedef sentence_info info_type;
    typedef edge<info_type> edge_type;
    typedef edge_equivalence<edge_type> edge_equiv_type;

    /*
    template <class IT, class Sent, class Grammar>
    score_t print_details( std::ostream& o
                         , edge_equivalence< edge<IT> > deriv
                         , Sent const& sent
                         , const Grammar&g
                         , bool more=false) 
    {
        //FIXME? any details?
        
        return as_one();
    }
    */

    //void reset() {}
    /*
    template <class GT> void
    create_info( info_type& n
               , score_t& i
               , score_t& h
               , GT& g
               , indexed_syntax_rule const& r ) {}

    template <class GT, class IT> void
    create_info( info_type &n
               , score_t &inside
               , score_t &heuristic
               , GT &gram
               , typename GT::rule_type r
               , edge<IT> const& e1
               , edge<IT> const& e2 )
    {
        inside = 1.0;
        heuristic = 1.0;
        if (not gram.template rule_property<bool>(r,lm_scoreable_id)) {
            n = unscoreable_sentence_info();
        } else if (edges_scoreable(e1,e2)) {
            edge_type const* vec[2];
            edge_type const** vec_end = scoreable_children(e1,vec);
            vec_end = scoreable_children(e2,vec_end);
            boost::transform_iterator<espan, edge_type const**> 
                itr(vec,espan()), end(vec_end,espan());
            n = join_span_range(gram.rule_span_string(r),itr,end);
        } else {
            typedef std::vector<edge_type const*> vec_t;
            vec_t vec = scoreable_children(e1,e2); //nvro
            typedef boost::transform_iterator<espan, typename vec_t::iterator>
                    transform_itr_t;
            transform_itr_t itr(vec.begin(),espan()),
                            end(vec.end(),espan());
            n = join_span_range(gram.rule_span_string(r),itr,end);
            //std::cerr << "scoreable children of " << print(e1,gram.dict())
            //          << " and " << print(e2,gram.dict()) << " :";
            //for (typename vec_t::iterator i = vec.begin(); i != vec.end(); ++i) {
            //    std::cerr << " " << print(**i,gram.dict());
            //}
            //std::cerr << std::endl;
        }
    }*/
       /*                  
    template <class GT, class IT> void
    create_info( info_type &n, score_t &inside
               , score_t &heuristic
               , GT &gram
               , typename GT::rule_type r
               , edge<IT> const& e )
    {
        inside = 1.0;
        heuristic = 1.0;
        if (not gram.lm_scoreable(r)) {
            n = unscoreable_sentence_info();
        } else if (edge_scoreable(e)) {
            edge_type const* vec[1];
            edge_type const** vec_end = scoreable_children(e,vec);
            boost::transform_iterator<espan, edge_type const**> 
                    itr(vec,espan()), end(vec_end,espan());
            n = join_span_range(gram.rule_span_string(r),itr,end);
       } else {
            typedef std::vector<edge_type const*> vec_t;
            vec_t vec = scoreable_children(e);
            typedef boost::transform_iterator<espan, typename vec_t::iterator>
                    transform_itr_t;
            transform_itr_t itr(vec.begin(),espan()),
                            end(vec.end(),espan());
            n = join_span_range(gram.rule_span_string(r),itr,end);
            //std::cerr << "scoreable children of " << print(e,gram.dict()) << " :";
            //for (typename vec_t::iterator i = vec.begin(); i != vec.end(); ++i) {
            //    std::cerr << " " << print(**i,gram.dict());
            //}
            //std::cerr << std::endl;
        } 

    } */
    
    //void create_info(info_type& n) const { n.set_null(); }
    
    typedef boost::tuple<info_type,score_t,score_t> result_type;
    typedef gusc::single_value_generator<result_type> result_generator;
    
    template <class Grammar, class ConstituentIterator>
    result_generator
    create_info( Grammar& grammar
               , typename Grammar::rule_type rule
               , boost::iterator_range<ConstituentIterator> constituents )
    {
        typedef boost::transform_iterator<constit2espan,ConstituentIterator>
                transform_itr_t;
        transform_itr_t itr(boost::begin(constituents),constit2espan()),
                        end(boost::end(constituents),constit2espan());
        info_type n = join_span_range(grammar.rule_span_string(rule),itr,end);
        return result_type(n,1.0,1.0);
    }
    
    std::vector<std::string> component_score_names() const 
    { 
        return std::vector<std::string>(); 
    }
    
    template < class Grammar
             , class ConstituentIterator
             , class ScoreOutputIterator >
    boost::tuple<ScoreOutputIterator,ScoreOutputIterator>
    component_scores( Grammar const& grammar
                    , typename Grammar::rule_type rule
                    , boost::iterator_range<ConstituentIterator> constituents
                    , info_type const& resultant
                    , ScoreOutputIterator scores_out
                    , ScoreOutputIterator heur_out
                    ) const { return boost::make_tuple(scores_out,heur_out); }
    
    template <class Grammar>
    bool scoreable_rule(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        return grammar.template rule_property<bool>(rule,lm_scoreable_id);
    }
    
    template <class GT>
    score_t rule_heuristic(GT& gram, typename GT::rule_type r) const
    {
        return 1.0;
    }
private:
    size_t lm_scoreable_id;
    size_t lm_string_id;
};


////////////////////////////////////////////////////////////////////////////////


} // namespace sbmt

# include <sbmt/edge/impl/sentence_info.ipp>

#endif // SBMT_EDGE_SENTENCE_INFO_HPP
