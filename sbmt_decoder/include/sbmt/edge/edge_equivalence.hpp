#ifndef   SBMT_EDGE_EDGE_EQUIVALENCE_HPP
#define   SBMT_EDGE_EDGE_EQUIVALENCE_HPP

#include <graehl/shared/intrusive_refcount.hpp>
#include <boost/limits.hpp>

#include <sbmt/span.hpp>
#include <sbmt/range.hpp>
#include <sbmt/logmath.hpp>
#include <sbmt/edge/edge_fwd.hpp>

#include <set>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/operators.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/atomic_count.hpp>

#define DEFAULT_MAX_EQUIVALENTS 4


namespace sbmt {


/// \addtogroup edges
///\{

struct edge_equivalence_cycle : public std::runtime_error
{
    edge_equivalence_cycle()
        : std::runtime_error("all the edges in an equivalence are "
                             "self-referencing unary cycles; erroneous pruning "
                             "must have occurred") {}
};


////////////////////////////////////////////////////////////////////////////////
///
///  All edges in an edge_equivalence are, well, equivalent, with respect to the
///  search.  In particular, if a1, a2 are edges in one equiv class, and
///  b1, b2 are edges in another, then the set of rules which combine any pair
///  (ai,bj) are equivalent, and the additional cost added is the same.
///  Equivalent => edge_infos are the same, and heuristic cost is the same.
///
///  close examination of this object reveals it just defers all behaviour to
///  a pointer of type edge_equivalence_impl.
///  why the encapsulation?  why not just
///  deal with a pointer to the impl directly?  because they are created in a
///  pool by the edge_factory, and I want it explicit that no one can destroy
///  the contents in them or create them other than the edge_factory.
///
///  these are lightweight value/handle objects, referring to storage owned by
///  edge_factories.  cheap copy, inlined operation, etc.
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT>
class edge_equivalence
{
 public:
    typedef edge_equivalence<EdgeT> self_type;
    typedef edge_equivalence_impl<EdgeT> impl_type;
    typedef impl_type* impl_raw_ptr;
    typedef boost::intrusive_ptr<impl_type> impl_shared_ptr;
    typedef EdgeT    edge_type;
    typedef typename impl_type::iterator iterator;
    typedef typename impl_type::const_iterator const_iterator;

    /// Every equivalence is nonempty and has a
    /// highest scoring edge called the representative
    edge_type & representative() const;

    edge_type operator[](unsigned i) const;

    /// Although in parsing, edges are only built when they cover a span, and
    /// only potentially tested for equivalence against other edges produced to
    /// cover the same span, we remember that span in edge_equivalence so that
    /// forest processing algorithms can know the split points taken e.g. for
    /// forest printing or multipass per-span viterbi outside cost calculation.
    span_t           span() const;

    /// merges two edge_equivalence objects.
    /// attempting to merge non-equivalent edges results in an error
    void             merge(edge_equivalence<EdgeT> const& other);

    /// inserting an edge that is not equivalent to the rest of the edges
    /// in this edge_equivalence results in an error.
    void             insert_edge(edge_type const& e);

    /// The first edge of the range is the representative, but no guarantees
    /// about the order of the rest.
    iterator       begin() const;
    iterator       end() const;

    /// Gives edges in best first (sorted) order of inside cost
    /// (heuristic cost must be constant by definition of equivalence).
    /// Invalidates previous edge_range for this object (may require sorting).
    /// The first edge in the range is guaranteed not to be a unary self-loop
    /// (this may mean breaking cost monotonicity for negative-cost self-loops)
    void sort();

    /// prune(representative().inside_score()) -> only 1st best (and ties) are kept.  not conceptually const; this removes (some) alts
    void prune_inside(score_t worse_than) const;


    std::size_t size() const;

    std::size_t hash_value() const;

    /// for dfs_forest and lazy_kbest
    bool operator==(const self_type &equiv) const;

    bool operator!=(const self_type &equiv) const;

    /// for lazy kbest
    static self_type NONE();
    /// for lazy kbest
    static self_type PENDING();

    impl_raw_ptr get_raw() const;
    impl_shared_ptr get_shared() const;

    operator bool() const;

    static std::size_t equivalence_count();

    /// for edge_factory AND now reusable_object_pool -> dfs_forest
    explicit edge_equivalence(impl_shared_ptr p);
    // valid edge_equivalences should only be created by edge_factory.
    explicit edge_equivalence(edge_type const& e);

    edge_equivalence();

    static void        max_edges_per_equivalence(std::size_t x);
    static std::size_t max_edges_per_equivalence();

 private:

//    template <class ECS> friend class derivation_builder; // was for NONE/PENDING
    template <class IF> friend class edge_factory;
    //friend class edge<typename EdgeT::info_type>;
    friend class edge_equivalence_impl<EdgeT>;

    // these two are private, because empty/null equivalences should never
    // be encountered by user; only by internal edge/edge_factory methods
    bool empty() const;

    impl_shared_ptr impl;

    static impl_type none_impl;
    static impl_type pending_impl;
    static impl_shared_ptr none_impl_ptr;
    static impl_shared_ptr pending_impl_ptr;
};

////////////////////////////////////////////////////////////////////////////////

class edge_pool_bad_alloc : public std::bad_alloc
{
 public:
    edge_pool_bad_alloc(char const* const msg) throw() : msg(msg) {}
    virtual char const* what() const throw()
    { return msg; }
 private:
    char const* const msg;
};

////////////////////////////////////////////////////////////////////////////////

class edge_counter;

class edge_stats : public boost::additive<edge_stats>
{
    typedef edge_stats self_type;
    std::size_t n_edge;
    std::size_t n_edge_equiv;

public:
    std::size_t edges() const { return n_edge; }
    std::size_t edge_equivalences() const { return n_edge_equiv; }

    edge_stats& operator -= (edge_stats const& o)
    {
        n_edge -= o.n_edge;
        n_edge_equiv -= o.n_edge_equiv;
        return *this;
    }

    edge_stats& operator += (edge_stats const& o)
    {
        n_edge += o.n_edge;
        n_edge_equiv += o.n_edge_equiv;
        return *this;
    }

    edge_stats()
      : n_edge(0), n_edge_equiv(0) {}
    edge_stats(std::size_t ne, std::size_t neq)
      : n_edge(ne), n_edge_equiv(neq) {}

    void print(std::ostream &o) const
    {
        o << n_edge << " edges"
          <<" and "<< n_edge_equiv << " equivalences created";
    }
    //edge_stats(long e, long eq) : n_edge(e), n_edge_equiv(eq) {}
    edge_stats(edge_counter const& e);
};

class edge_counter {
public:
    void increment_edges() { ++n_edge; }
    void decrement_edges() { --n_edge; }
    void increment_edge_equivalences() { ++n_edge_equiv; }
    void decrement_edge_equivalences() { --n_edge_equiv; }
    long edges() const { return n_edge; }
    long edge_equivalences() const { return n_edge_equiv; }
private:
    boost::detail::atomic_count n_edge;
    boost::detail::atomic_count n_edge_equiv;
};

inline edge_stats::edge_stats(edge_counter const& ec)
  : n_edge(ec.edges()), n_edge_equiv(ec.edge_equivalences()) {}

inline std::ostream& operator << (std::ostream &o, edge_stats const& me)
{
    me.print(o);
    return o;
}

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT>
class edge_equivalence_pool
{
 public:
    typedef EdgeT                       edge_type;
    typedef edge_equivalence<edge_type> edge_equiv_type;

    edge_equivalence_pool( std::size_t max =
                           std::numeric_limits<std::size_t>::max() );

    edge_equiv_type create(edge_type const& e);

    void        max_equivalences(std::size_t x);
    std::size_t max_equivalences() const;

    edge_stats stats() { return edge_stats(0,edge_equiv_type::equivalence_count()); }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// releases eq from its edge_equivalence_pool other, and assigns ownership
    /// to this pool.
    /// returns the transferred eq.
    ///
    ////////////////////////////////////////////////////////////////////////////
    edge_equiv_type transfer(edge_equiv_type eq, edge_equivalence_pool& other);

    void swap(edge_equivalence_pool& other);
    void reset();

 private:
    typedef edge_equivalence_impl<edge_type> equiv_impl_t;
    //typedef boost::scoped_ptr<boost::object_pool<equiv_impl_t> > pool_ptr;

    std::size_t max_allowed;
    //pool_ptr    pool;
   // edge_stats  stat;

    // helper to ensure that we throw an exception if memory is
    // exhausted. the object pool we are using does not.  it returns null.
    equiv_impl_t* create_p(edge_type const& a);
};

template <class ET>
void swap(edge_equivalence_pool<ET>& p1, edge_equivalence_pool<ET>& p2)
{
    p1.swap(p2);
}

///\}


} // namespace sbmt


#include <sbmt/edge/impl/edge_equivalence.ipp>

#endif // SBMT_EDGE_EDGE_EQUIVALENCE_HPP
