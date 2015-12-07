namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class E>
bool ratio_predicate::keep_edge(edge_queue<E> const& pqueue, E const& e) const
{
    return e.score() >= threshold * pqueue.highest_score();
}

////////////////////////////////////////////////////////////////////////////////

template <class E>
bool ratio_predicate::pop_top(edge_queue<E> const& pqueue) const
{
    assert(pqueue.lowest_score() <= pqueue.highest_score());
    return pqueue.lowest_score() < threshold * pqueue.highest_score();
}

////////////////////////////////////////////////////////////////////////////////

template <class E>
bool histogram_predicate::keep_edge(edge_queue<E> const& pq, E const& e) const
{
    /// i would like the second half of this statement to be
    /// e.score() >= pqueue.lowest_score() but it is very important that
    /// after deciding to keep an edge, one doesnt then turn around and
    /// pop it off before adding new edges.
    ///
    /// 2007-12-11 -- i am going to write the statement as i want, 
    /// make it contractual that predicate_edge_filter always remove all or no
    /// edges of the same score, no matter what pop_top says.
    return pq.size() < top_n || e.score() >= pq.lowest_score();
}

////////////////////////////////////////////////////////////////////////////////

template <class E>
bool histogram_predicate::pop_top(edge_queue<E> const& pq) const
{
    assert(pq.lowest_score() <= pq.highest_score());
    return pq.size() > top_n;
}

////////////////////////////////////////////////////////////////////////////////

template <class E> boost::logic::tribool 
fuzzy_ratio_predicate::keep_edge(edge_queue<E> const& pq, E const& e) const
{
    using namespace boost::logic;
    
    if (e.score() >= threshold * pq.highest_score()) {
        //std::cout << "beam:true " << std::flush;
        return true;
    }
    else if (e.score() >= threshold * fuzz * pq.highest_score()) {
        //std::cout << "beam:indeterminate " << std::flush;
        return indeterminate;
    }
    else {
        //std::cout << "beam:false " << std::flush;
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class E>
bool fuzzy_ratio_predicate::pop_top(edge_queue<E> const& pq) const
{
    assert(pq.lowest_score() <= pq.highest_score());
    return pq.lowest_score() < threshold * pq.highest_score();
}

////////////////////////////////////////////////////////////////////////////////

template <class E> boost::logic::tribool
fuzzy_histogram_predicate::keep_edge(edge_queue<E> const& pq, E const& e) const
{
    using namespace boost::logic;
    /// i would like the second half of this statement to be
    /// e.score() >= pqueue.lowest_score() but it is very important that
    /// after deciding to keep an edge, one doesnt then turn around and
    /// pop it off before adding new edges.
    ///
    /// 2007-12-11 -- i am going to write the statement as i want, 
    /// make it contractual that predicate_edge_filter always remove all or no
    /// edges of the same score, no matter what pop_top says.
    if (pq.size() < top_n || e.score() >= pq.lowest_score()) {
        //std::cout << "hist:true " << std::flush;
        return true;
    }
    else if ( e.score() >= fuzz * pq.lowest_score() ) {
        //std::cout << "hist:indeterminate " << std::flush;
        return indeterminate;
    } 
    else {
        //std::cout << "hist:false "<< std::flush;
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class E>
bool fuzzy_histogram_predicate::pop_top(edge_queue<E> const& pq) const
{
    assert(pq.lowest_score() <= pq.highest_score());
    return pq.size() > top_n;
}

////////////////////////////////////////////////////////////////////////////////

template <class E> boost::logic::tribool 
poplimit_histogram_predicate::keep_edge(edge_queue<E> const& pq, E const& e) const
{
    using namespace boost::logic;
    boost::logic::tribool retval(false);
    
    ++num_keep_queries;
    if (num_keep_queries > poplimit || (pq.size() >= top_n && num_keep_queries > softlimit)) {
        retval = false;
    } else if (pq.size() < top_n || e.score() > pq.lowest_score()) {
        retval = true;
    } else {
        retval = indeterminate;
    }
    SBMT_PEDANTIC_MSG(
        filters
      , "poplimit-histogram-predicate::keep_edge:%s, poplimit:%s, queries:%s, "
        "queue-size:%s, top-n:%s, edge-score:%s, low-score:%s"
      , retval % 
        poplimit %
        num_keep_queries %
        pq.size() %
        top_n %
        e.score() %
        pq.lowest_score()
    );
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class E>
bool poplimit_histogram_predicate::pop_top(edge_queue<E> const& pq) const
{
    assert(pq.lowest_score() <= pq.highest_score());
    SBMT_PEDANTIC_MSG( 
        filters
      , "poplimit-histogram-predicate::pop_top:%s, queue-size:%s, top-n:%s"
      , (pq.size() > top_n) % 
        pq.size() %
        top_n
    );
    return pq.size() > top_n;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
