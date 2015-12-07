//#include <graehl/shared/exact_cast.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
/*
template <class ET, class IP, class SP>
template <class Sentence,class Grammar>
void basic_chart<ET,IP,SP>::
from_sentence(Sentence const& sentence,ECS &ecs,Grammar & gram)
{
    clear();
    span_index_t s=0;
    (void)graehl::exact_static_cast<span_index_t>(sentence.size());
    for (typename Sentence::iterator i=sentence.begin(),e=sentence.end();
         i!=e;++i,++s)
        insert_edge(ecs,ecs.create_edge(gram,i->label(),span_t(s,s+1)));    
}
*/


////////////////////////////////////////////////////////////////////////////////

template <class ET, class IP, class SP>
std::pair<typename basic_chart<ET,IP,SP>::edge_iterator,bool>
basic_chart<ET,IP,SP>::find(edge_type const& e) const
{
    typename span_container_t::const_iterator pos = 
        span_container.find(e.span());
    assert (pos != span_container.end());
    return pos->second.find(e);
}
        
////////////////////////////////////////////////////////////////////////////////

template <class ET, class IP, class SP>
typename basic_chart<ET,IP,SP>::edge_range
basic_chart<ET,IP,SP>::edges(indexed_token t, span_t s) const
{
    typename span_container_t::const_iterator pos = 
        span_container.find(s);
    if (pos == span_container.end()) {
        return edge_range();
    }
    else return pos->second.edges(t);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class IP, class SP>
typename basic_chart<ET,IP,SP>::cell_range
basic_chart<ET,IP,SP>::cells(span_t s) const
{
    typename span_container_t::const_iterator pos = 
        span_container.find(s);
    if (pos == span_container.end())
        return cell_range();
    else return pos->second.cells();
}

template <class ET, class IP, class SP>
basic_chart<ET,IP,SP>::basic_chart(std::size_t max_len)
: max_span_rt(max_len)
{
    // pre-allocating dummy containers for the chart allows us to multi-thread
    // the cky loop without locking, since each thread will be modifying a
    // different container in the chart, and the table containing the containers
    // will never change.
    span_t target_span(0,max_len);
    for(std::size_t x = 1; x < max_len; ++x) {
        shift_generator sg(target_span,x);
        shift_generator::iterator itr = sg.begin();
        for (; itr != sg.end(); ++itr) {
            insert_into_map(span_container, *itr, span_type());
        }
    }
    insert_into_map(span_container, target_span, span_type());
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class IP, class SP>
typename basic_chart<ET,IP,SP>::span_storage_reference
basic_chart<ET,IP,SP>::span_storage(span_t s)
{
    typename span_container_t::const_iterator pos = 
        span_container.find(s);
    if (pos == span_container.end()) {
        assert(false);    
    }
    return const_cast<span_type&>(pos->second);
}

template <class ET, class IP, class SP>
void
basic_chart<ET,IP,SP>::clear(span_t s)
{
    typename span_container_t::iterator pos = 
        span_container.find(s);
    if (pos == span_container.end()) {
        assert(false);    
    }
    //span_container.erase(pos);
    pos->second.clear();
}

template <class ET, class IP, class SP>
std::pair<typename basic_chart<ET,IP,SP>::cell_iterator,bool>
basic_chart<ET,IP,SP>::cell(span_t s, indexed_token t)
{
    typename span_container_t::const_iterator pos = 
        span_container.find(s);
    if (pos == span_container.end()) {
        assert(false);    
    }
    return const_cast<span_type&>(pos->second).cell(t);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class IP, class SP>
void 
basic_chart<ET,IP,SP>::insert_edge( edge_equivalence_pool<edge_type>& epool
                                  , edge_type const& e )
{
    span_t s = e.span();
    typename span_container_t::const_iterator pos = 
        span_container.find(s);
    if (pos == span_container.end()) {
        assert(false);
        insert_into_map(span_container, s, span_type(epool,e));
        max_span_rt = std::max(max_span_rt,s.right());
    } else {
        // this is evil, but seems to be caused by a deficiency in hash_map
        // one should be able to modify the value of a map in-place
        // (as opposed to modifying the key, which would break the mapping)
        // but for some reason, hash_map::iterator returns a const reference
        // to the key _and_ value.
        //
        // i suspect that hash_map::iterator and hash_map::const_iterator are 
        // typedefs to each-other in the gcc implementation!
        const_cast<span_type&>(pos->second).insert_edge(epool,e);
    }
}

template <class ET, class IP, class SP>
void basic_chart<ET,IP,SP>::insert_edge(edge_equiv_type const& eq)
{
    span_t s = eq.span();
    typename span_container_t::const_iterator pos = 
        span_container.find(s);
    if (pos == span_container.end()) {
        assert(false);
        insert_into_map(span_container, s, span_type(eq));
        max_span_rt = std::max(max_span_rt,s.right());
    } else {
        const_cast<span_type&>(pos->second).insert_edge(eq);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class IP, class SP>
void basic_chart<ET,IP,SP>::reset(std::size_t max_len)
{
    span_container.clear();
    max_span_rt = max_len;
    span_t target_span(0,max_len);
    for(std::size_t x = 1; x < max_len; ++x) {
        shift_generator sg(target_span,x);
        shift_generator::iterator itr = sg.begin();
        for (; itr != sg.end(); ++itr) {
            insert_into_map(span_container, *itr, span_type());
        }
    }
    insert_into_map(span_container, target_span, span_type());
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class IP, class SP>
void print( std::ostream& out
          , basic_chart<ET,IP,SP> const& chart
          , indexed_token_factory const& tf )
{
    span_index_t len = 1;
    typedef basic_chart<ET,IP,SP> chart_t;
    typedef std::multiset< typename chart_t::edge_equiv_type
                         , greater_edge_score<ET> > score_sort_t;
                         
    for (; len <= chart.max_span_rt; ++len) {
        
        shift_generator sg(span_t(0,chart.max_span_rt), len);
        shift_generator::iterator sitr = sg.begin();
        for (; sitr != sg.end(); ++sitr) {
            score_sort_t sorted_edges;
            out << *sitr << ":";
            typename chart_t::cell_range cell_r = chart.cells(*sitr);
            typename chart_t::cell_iterator cell_itr = cell_r.begin();
            for (; cell_itr != cell_r.end(); ++cell_itr) {
                typename chart_t::edge_range edge_r = cell_itr->edges();
                typename chart_t::edge_iterator edge_itr = edge_r.begin();
                    
                for( ; edge_itr != edge_r.end(); ++edge_itr) {
                    sorted_edges.insert(*edge_itr);
                }
            }
            out << "# edges: " << sorted_edges.size();
            typename score_sort_t::iterator itr = sorted_edges.begin();
            typename score_sort_t::iterator end = sorted_edges.end();
            if (itr != end) {
                out << " ### " << print(itr->representative().root(),tf);
                //out << " ### " << print(sorted_edges.rbegin()->representative(),tf);
                ++itr;
            }
            for (; itr != end; ++itr) 
                out << " ### " << print(itr->representative().root(),tf);
            
            out << std::endl;
        }
        out << std::endl;
    }
}

} // namespace sbmt
