#include <sbmt/search/cube.hpp>
#include <sbmt/hash/hash_map.hpp>
#include <sbmt/hash/hash_set.hpp>
#include <boost/range/iterator_range.hpp>

namespace sbmt {

namespace detail {

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
struct pre_post_application
{
    typedef ET edge_type;
    typedef typename sorted_cube< typename rule_applications<GT>::iterator
                                , typename CT::edge_iterator
                                , typename CT::edge_iterator >::item
            cube_item_type;
    cube_item_type pre;
    edge_type      post;
    
    pre_post_application( cube_item_type const& pre
                        , edge_type const& post )
    : pre(pre), post(post) {}
};

template <class ET, class GT, class CT>
struct post_score_compare
{
    typedef bool value_type;
    
    bool operator()( pre_post_application<ET,GT,CT> const& p1
                   , pre_post_application<ET,GT,CT> const& p2 ) const
    {
        return p1.post.score() < p2.post.score();
    }
};

template <class CT, class QT, class IT, class ET, class GT>
void
push_pre_post( QT& pq
             , IT& items_seen
             , GT& gram
             , typename pre_post_application<ET,GT,CT>::cube_item_type const& pre
             , concrete_edge_factory<ET,GT>& ef )
{
    #if SBMT_CUBE_HEAP_REDUNDANT_PATHS
    if (items_seen.find(pre) == items_seen.end()) {
    #endif
    
        pq.push(pre_post_application<ET,GT,CT>(
                   pre
                 , ef.create_edge(gram, pre.x(), pre.y(), pre.z())()
               )
        );
    #if SBMT_CUBE_HEAP_REDUNDANT_PATHS
        items_seen.insert(pre);
    }
    #endif
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
class insert_triple_to_heap
{
public:
    typedef typename GT::rule_type rule_type;
    typedef edge_equivalence<ET>                   edge_equiv_type;
    bool operator()( rule_type const& r
                   , edge_equiv_type const& e1
                   , edge_equiv_type const& e2 )
    {
        rule_edge_triple<ET,GT> triple(gram,r,e1,e2);
        best_score = std::max(triple.score(),best_score);
        if (triple.score() < stop_thresh * best_score) return false;
        else {
            pq.push(triple);
            return true;
        }
    }
    
    insert_triple_to_heap( GT& gram
                         , score_t& best_score
                         , score_t const& stop_thresh
                         , std::priority_queue< rule_edge_triple<ET,GT> >& pq )
    : gram(gram)
    , best_score(best_score)
    , stop_thresh(stop_thresh)
    , pq(pq) {}
    
private:
    GT& gram;
    score_t& best_score;
    score_t  stop_thresh;
    std::priority_queue< rule_edge_triple<ET,GT> >& pq;
};

} // namespace sbmt::detail

    
////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
template <class FilterF>
cube_heap_span_filter<ET,GT,CT>::cube_heap_span_filter( 
                                    FilterF f
                                  , span_t const& target_span
                                  , GT& gram
                                  , concrete_edge_factory<ET,GT>& ef
                                  , CT& chart
                                  , cube_heap_factory<ET,GT,CT>* parent)
: base_t(target_span,gram,ef,chart)
, fuzzy_filter(f)
, finalized(false)
, parent(parent)
{}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void
cube_heap_span_filter<ET,GT,CT>::apply_rules( rule_range const& rr
                                            , edge_range const& er1
                                            , edge_range const& er2 )
{
    boost::iterator_range<rule_iterator> 
        rrr(rr.template get<0>(),rr.template get<1>());
    if ( (boost::begin(rrr) != boost::end(rrr)) and
         (boost::begin(er1) != boost::end(er1)) and
         (boost::begin(er2) != boost::end(er2))
       ) {
        sorted_cube_list.push_back(sorted_cube_t(rrr,er1,er2));
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
struct greater_rule_estimate
{
    bool operator()( std::pair<typename GT::rule_type,score_t> const& r1
                   , std::pair<typename GT::rule_type,score_t> const& r2 ) const
    { return r1.second > r2.second; }
    
    cube_heap_span_filter<ET,GT,CT>* chs;
    
    greater_rule_estimate(cube_heap_span_filter<ET,GT,CT>* chs)
    : chs(chs) {}
};

template <class ET, class GT, class CT>
void cube_heap_span_filter<ET,GT,CT>::finalize()
{
    if (finalized) return;
  //  std::cout << "cube heap finalize..." << std::flush;    
    using namespace detail;
    using namespace detail::cube;
    using namespace std;
    using namespace boost::logic;
    
    io::logging_stream& log = registry_log(cube_heap_domain);
    
    typedef stlext::hash_set< typename sorted_cube_t::item
                            , member_hash<typename sorted_cube_t::item> >
            item_set_t;
    
    item_set_t items_seen;
            
    
    /// a queue of pre and post processed edge information, sorted so the
    /// highest scoring post-processed edge is on top.
    typedef std::priority_queue<
                pre_post_application<ET,GT,CT>
              , std::vector< pre_post_application<ET,GT,CT> >
              , post_score_compare<ET,GT,CT>
            > priority_queue_t;
    priority_queue_t pq;
    
    typename sorted_cube_list_t::iterator itr = sorted_cube_list.begin();
    typename sorted_cube_list_t::iterator end = sorted_cube_list.end();
    std::size_t cube_heap_size = 0;
    {io::log_time_report fill(log,io::lvl_verbose,"cube-heap initialized: ");
    for (; itr != end; ++itr) {
        if (io::logging_at_level(log,io::lvl_verbose)) {
            // careful, this is not a constant function...
            cube_heap_size += 1;
        }
        /// the priority queue is being initialized with the edges produced
        /// by the top-corners of each cube.
        if (!itr->empty()) {
            typename sorted_cube_t::item pre = itr->corner();
            push_pre_post<CT>(pq,items_seen,base_t::gram,pre,base_t::ef);
        }
    }}
    
    std::size_t edges_popped = 0;
    {io::log_time_report explore(log, io::lvl_verbose, "time exploring cube-heap: ");
    while(!pq.empty()) {
        edge_type const& e = pq.top().post;
        
        /// we stop the cube search if the top of the heap fails the beam/histogram criteria
        /// with a fuzz to weaken the hypothesis.
        tribool add_edge = fuzzy_filter.insert(base_t::epool,e);
        
        if (add_edge == false) {
            //std::cout << "++++ false: best: "<< fuzzy_filter.highest_score();
            //std::cout << "worst: "<< fuzzy_filter.lowest_score();
            //std::cout << "e: " << e.score();
            //std::cout << "cube-heap:false" << std::endl;
            break;
        } else if (indeterminate(add_edge)) {
            //std::cout << "cube-heap:indeterminate" << std::endl;
        } else {
            //std::cout << "cube-heap:true" << std::endl;
        }
        
        typename sorted_cube_t::item pp = pq.top().pre;
        pq.pop();
        ++edges_popped;
        
        for (int xyz = 1; xyz <= 3; ++xyz) {
            if (pp.has_successor(xyz)) {
                typename sorted_cube_t::item next = pp.successor(xyz);
                push_pre_post<CT>(pq,items_seen,base_t::gram,next,base_t::ef);
            }
        }
    }}
    {
        io::log_time_report fnl(log,io::lvl_verbose, "time finalizing cube-heap inner filter: ");
        fuzzy_filter.finalize();
    }
    SBMT_VERBOSE_STREAM_TO(
        log
      , base_t::target_span << ": " << edges_popped 
        << " edges popped exploring "<< cube_heap_size << " cubes."
    );

}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool cube_heap_span_filter<ET,GT,CT>::is_finalized() const
{
    return fuzzy_filter.is_finalized();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool cube_heap_span_filter<ET,GT,CT>::empty() const
{
    return fuzzy_filter.empty();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void cube_heap_span_filter<ET,GT,CT>::pop()
{
    fuzzy_filter.pop();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
typename cube_heap_span_filter<ET,GT,CT>::edge_equiv_type const& 
cube_heap_span_filter<ET,GT,CT>::top() const
{
    return fuzzy_filter.top();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt


