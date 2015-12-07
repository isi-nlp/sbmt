#include <graehl/shared/null_deleter.hpp>
#include <sbmt/logmath.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <sbmt/token/token.hpp>
#include <sbmt/edge/edge_equivalence.hpp>
#include <limits>
#include <vector>
#include <algorithm>
#include <boost/detail/atomic_count.hpp>
#include <graehl/shared/intrusive_refcount.hpp>
#include <graehl/shared/epsilon.hpp>
// have compiled with all 4 combinations; light testings says they're OK

// advantage: sort doesn't invalidate insert_edge heap pruning
//#define EDGE_EQUIV_REVERSE_SORT

// advantage: not too much.  was just bughunting and using this to compare
//#define EQUIV_ITER_CONSTANT_END

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  sort so that higher scored edges appear before lower scored edges in a
///  sorted container.
///
////////////////////////////////////////////////////////////////////////////////
template <class ET>
struct edge_score_compare
{
    bool operator()(ET const& e1, ET const& e2) const
    {
        return e1.score() > e2.score();
    }
};



/*
template <class ET>
void intrusive_ptr_add_ref(edge_equivalence_impl<ET>* ptr);

template <class ET>
void intrusive_ptr_release(edge_equivalence_impl<ET>* ptr);
*/

////////////////////////////////////////////////////////////////////////////////
///
/// motivation behind this implementation is this:
/// equivalent edges have a lot in common, and little that differs.
/// they have a common root, common info, common heuristic score portion.
/// potentially they have different children, different inside scores, and
/// different syntax rules that created them.
///
/// rather than redundantly store all the common info, we only store one full-
/// fledged edge.  only the differing data is stored multiply, in a structure
/// called alt_edge
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT>
class edge_equivalence_impl : public graehl::intrusive_refcount<edge_equivalence_impl<EdgeT> >
{
	static std::size_t max_equivalents;
public:
	static void        max_edges_per_equivalence(std::size_t x) 
	{ 
	    max_equivalents = x == 0 ? std::numeric_limits<unsigned>::max() : x; 
	}
    static std::size_t max_edges_per_equivalence() { return max_equivalents; }
    typedef EdgeT edge_type;
    typedef edge_equivalence_impl<EdgeT> self_type;
	typedef boost::intrusive_ptr<self_type> self_ptr_type;
    typedef graehl::intrusive_refcount<self_type> refcount;

    //typedef edge_equivalence<EdgeT>  edge_equiv_type;
    typedef typename edge_type::info_type info_type;
    ~edge_equivalence_impl() { --num_equivs; }

    unsigned edge_equiv_max() const
    {
        return (m_rep.root().type() == top_token)
            ? std::numeric_limits<unsigned>::max()
            : max_edges_per_equivalence();
    }
    struct alt_edge {
        self_ptr_type child[2];
        score_t         inside;
        grammar_rule_id rule_id;

        alt_edge( edge_type const & e )
        : inside(e.inside_score())
        , rule_id(e.rule_id())
        {
            child[0] = e.has_first_child() ? e.first_children().get_shared()
                                           : self_ptr_type();
            child[1] = e.has_second_child() ? e.second_children().get_shared()
                                            : self_ptr_type();
        }
        alt_edge() {} // for vector::resize
    };


    /// do not use this to sort.  instead, always sort such that worst-on-top property is maintained.  iterator gives best->worst order when you sort by worst first now.
    struct better_score
    {
        typedef bool result_type;
        bool operator()(alt_edge const& e1, alt_edge const& e2) const
        {
            return e1.inside > e2.inside;
        }
    };

    struct worse_score
    {
        typedef bool result_type;
        bool operator()(alt_edge const& e1, alt_edge const& e2) const
        {
            return e1.inside < e2.inside;
        }
    };


    typedef std::vector<alt_edge> alt_container_t;
    typedef typename alt_container_t::const_iterator alt_iter;

    alt_container_t m_alts;
    edge_type       m_rep;
//    boost::detail::atomic_count m_count; // intrusive reference count.
    static boost::detail::atomic_count num_equivs;
    bool alts_full() const
    {
        return m_alts.size()>=edge_equiv_max()-1;
    }
public:

    static std::size_t equivalence_count() { return num_equivs; }

    class iterator
    : public boost::iterator_facade <
                  iterator
                , edge_type const
                , boost::forward_traversal_tag
             >
    {
        typedef alt_iter iterator_t;
        edge_type  current;
        iterator_t next;
        iterator_t end; // so we can "dereference" on increment into current (do nothing if ++next==end)
        bool at_end; // so we visit singleton equiv's one edge (next==end at start)

        edge_type const& dereference() const { assert(!at_end); return current; }

#ifdef EDGE_EQUIV_REVERSE_SORT
# define EDGE_EQUIV_BEGIN end
# define EDGE_EQUIV_END begin
#else
# define EDGE_EQUIV_BEGIN begin
# define EDGE_EQUIV_END end
#endif

        bool equal(iterator const& o) const
        {
#ifndef EQUIV_ITER_CONSTANT_END
            if (!at_end && !o.at_end)
                return next==o.next;
            return at_end && o.at_end;
#else
            return (next == o.next) and (at_end == o.at_end);   // check of at_end lets us visit representative even when no alts
#endif
        }

        void increment()
        {
            if (next != end) {
#ifdef EDGE_EQUIV_REVERSE_SORT
                --next;
#endif
                current.adjust( edge_equivalence<EdgeT>(next->child[0])
                              , edge_equivalence<EdgeT>(next->child[1])
                              , next->inside
                              , next->rule_id);
#ifndef EDGE_EQUIV_REVERSE_SORT
                ++next;
#endif
            } else
                at_end = true;
        }

        template <class iter>
        /// the "begin" constructor, used by edges():
        iterator( edge_type const& e
                     , iter next
                     , iter end )
        : current(e)
#ifdef EDGE_EQUIV_REVERSE_SORT
        , next(end)
        , end(next)
#else
        , next(next)
        , end(end)
#endif
        , at_end(false) {}

        template <class vect>
        /// the "begin" constructor, used by edges():
        iterator( edge_type const& e
                       , vect const& alts)
        : current(e)
#ifdef EDGE_EQUIV_REVERSE_SORT
        , next(alts.end())
        , end(alts.begin())
#else
        , next(alts.begin())
        , end(alts.end())
#endif
        , at_end(false) {}

        /// the "end" constructor, used by edges():
#ifndef EQUIV_ITER_CONSTANT_END
        template <class vect>
        iterator(vect const& alts) : at_end(true) {}
#else
        template <class vect>
        iterator(vect const& alts) :
#ifdef EDGE_EQUIV_REVERSE_SORT
            next(alts.begin()),
#else
            next(alts.end()),
#endif
            at_end(true) {}
#endif
        friend class edge_equivalence_impl<EdgeT>;
        friend class boost::iterator_core_access;
    };

    typedef iterator const_iterator;

    edge_type const& representative() const { return m_rep; }
    edge_type & representative() { return m_rep; }

    edge_type operator[](unsigned i) const
    {
        if (i==0)
            return representative();
        --i;
        edge_type e=representative();
        alt_edge const& a=m_alts[i];
        e.adjust(edge_equivalence<EdgeT>(a.child[0]),edge_equivalence<EdgeT>(a.child[1]),a.inside,a.rule_id);
        return e;
    }

    bool empty() const { return m_rep.is_null(); }
    span_t const& span() const { return m_rep.span(); }

    std::size_t size() const { return empty() ? 0 : m_alts.size() + 1; }

    edge_equivalence_impl()
    //    : m_count(0)
    {
        m_rep.set_null();
        ++num_equivs;
    }

//    std::size_t usage_count() const { return m_count; }

    edge_equivalence_impl(edge_type const& e)
    : m_rep(e)
                         //    , m_count(0)
    {
        ++num_equivs;
    }

    /// note: you may not merge or insert any more after this, unless EDGE_EQUIV_REVERSE_SORT defined (reverse sort is still worst-heap)
    void prune_inside(score_t worse_than)
    {
#ifdef EDGE_EQUIV_REVERSE_SORT
        while(!m_alts.empty() && m_alts.front().inside < worse_than)
            pop_worst();
#else
        sort_alts();
        for (alt_iter i=m_alts.begin(),e=m_alts.end();i!=e;++i)
            if (i->inside < worse_than) {
                m_alts.resize(i-m_alts.begin());
                return;
            }
#endif
    }

    void pop_worst()
    {
        std::pop_heap(m_alts.begin(),m_alts.end(),worse_score());
        m_alts.pop_back();
    }

    void insert_edge(edge_type const& e)
    {
        using namespace std;
        worse_score worse;

        assert(empty() or m_rep == e);
        assert(empty() or hash_value(m_rep) == hash_value(e));
//        assert(empty() or graehl::very_close( m_rep.heuristic_score().log()
//                                            , e.heuristic_score().log()
//                                            , 1e-3 ) );


        if(empty()) {
            m_rep = e;
            return;
        }

        if (e.inside_score() > m_rep.inside_score()) { // improves on old best
            if (edge_equiv_max()>1) {
                if (alts_full()) {
                    std::pop_heap(m_alts.begin(),m_alts.end(),worse);
                    m_alts.back() = alt_edge(m_rep);
                } else {
                    m_alts.push_back(alt_edge(m_rep));
                }
            }

//            std::push_heap(m_alts.begin(),m_alts.end(),worse); // redundant: old best was better than whole heap, so at end (bottom of balanced binary tree) already satisfies
            m_rep = e;
            return;
        } // else
// worse than old best - goes somewhere in alts (unless full and worse than all)
        if (edge_equiv_max()==1)
            return;

        if (alts_full()) {
            if (m_alts[0].inside < e.inside_score()) { // better than old worst //NOTE: this is why max-equivalents must be 2 or more
                std::pop_heap(m_alts.begin(),m_alts.end(),worse);
                m_alts.back() = alt_edge(e);
            } else {
                return;  // (discard new e as it's worse than everything and no room to keep)
            }
        } else {
            m_alts.push_back(alt_edge(e));
        }
        std::push_heap(m_alts.begin(),m_alts.end(),worse); // necessary: we have no idea what the rank of e is among alts
    }
    
    iterator begin() const
    {
        if (empty()) return iterator(m_alts);
        else return iterator(m_rep,m_alts);
    }
    
    iterator end() const
    {
        return iterator(m_alts);
    }

    /// note: you may not merge or insert any more after this, unless EDGE_EQUIV_REVERSE_SORT defined (reverse sort is still worst-heap)
    void sort_alts()
    {
        std::sort( m_alts.begin()
                 , m_alts.end()
#ifdef EDGE_EQUIV_REVERSE_SORT
                 , worse_score()
#else
                   , better_score()
#endif
            );
    }


    void sort()
    {
        sort_alts();

    }

    void swap_alts(edge_equivalence_impl& other)
    {
        m_alts.swap(other.m_alts);
    }

    edge_equivalence_impl(bool dummy_static_equiv)
    //    : m_count(1)
    {
        m_rep.set_null();
    }

};

////////////////////////////////////////////////////////////////////////////////

template <class ET>
edge_equivalence<ET>::edge_equivalence(impl_shared_ptr impl)
: impl(impl) {}

template <class ET>
edge_equivalence<ET>::edge_equivalence(ET const& e)
: impl(new edge_equivalence_impl<ET>(e))
{
//    assert(impl->usage_count() == 1);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
typename edge_equivalence<ET>::edge_type &
edge_equivalence<ET>::representative() const
{
    assert(!empty());
    return impl->representative();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
span_t edge_equivalence<ET>::span() const
{
    assert(bool(impl));
    return impl->span();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
typename edge_equivalence<ET>::iterator
edge_equivalence<ET>::end() const
{
    assert(bool(impl));
    return impl->end();
}

template <class ET>
typename edge_equivalence<ET>::iterator
edge_equivalence<ET>::begin() const
{
    assert(bool(impl));
    return impl->begin();
}


////////////////////////////////////////////////////////////////////////////////

template <class ET>
std::size_t
edge_equivalence<ET>::size() const
{
    return !impl ? 0 : impl->size();
}
////////////////////////////////////////////////////////////////////////////////

template <class ET>
bool edge_equivalence<ET>::empty() const
{
    return !impl or impl->empty();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
void edge_equivalence<ET>::merge(edge_equivalence<ET> const& other)
{
    assert(bool(impl));
    if (!other.empty()) {
        iterator itr = other.impl->begin(), end_ = other.impl->end();
        for (; itr != end_; ++itr)
            impl->insert_edge(*itr);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
void edge_equivalence<ET>::insert_edge(edge_type const& e)
{
    assert(bool(impl));
    return impl->insert_edge(e);
}

template <class ET>
std::size_t edge_equivalence_impl<ET>::max_equivalents=DEFAULT_MAX_EQUIVALENTS;

template <class ET>
void edge_equivalence<ET>::max_edges_per_equivalence(std::size_t m)
{
    edge_equivalence_impl<ET>::max_edges_per_equivalence(m);
}

template <class ET>
std::size_t edge_equivalence<ET>::max_edges_per_equivalence()
{
    return edge_equivalence_impl<ET>::max_edges_per_equivalence();
}

template <class ET>
boost::detail::atomic_count edge_equivalence_impl<ET>::num_equivs(0);

template <class ET>
typename edge_equivalence<ET>::impl_type
edge_equivalence<ET>::none_impl;

template <class ET>
typename edge_equivalence<ET>::impl_type
edge_equivalence<ET>::pending_impl;

template <class ET>
typename edge_equivalence<ET>::impl_shared_ptr
edge_equivalence<ET>::none_impl_ptr(new edge_equivalence_impl<ET>());

template <class ET>
typename edge_equivalence<ET>::impl_shared_ptr
edge_equivalence<ET>::pending_impl_ptr(new edge_equivalence_impl<ET>());

template <class ET>
edge_equivalence<ET> edge_equivalence<ET>::NONE()
{
//    if (!none_impl_ptr) none_impl_ptr = new edge_equivalence_impl<ET>();
    return edge_equivalence<ET>(none_impl_ptr);
}

template <class ET>
edge_equivalence<ET> edge_equivalence<ET>::PENDING()
{
//    if (!pending_impl_ptr) pending_impl_ptr = new edge_equivalence_impl<ET>();
    return edge_equivalence<ET>(pending_impl_ptr);
}

template <class E>
std::size_t edge_equivalence<E>::hash_value() const
{
	return boost::hash_value(this->get_raw());
}

template <class E>
bool edge_equivalence<E>::operator==(self_type const& equiv) const
{
	return impl == equiv.impl;
}

template <class E>
bool edge_equivalence<E>::operator!=(self_type const& equiv) const
{
	return !(*this == equiv);
}

template <class E>
std::size_t hash_value(edge_equivalence<E> const& eq)
{
	return eq.hash_value();
}

template <class E>
typename edge_equivalence<E>::impl_raw_ptr
edge_equivalence<E>::get_raw() const
{
	return impl.get();
}

template <class E>
typename edge_equivalence<E>::impl_shared_ptr
edge_equivalence<E>::get_shared() const
{
	return impl;
}

template <class E>
edge_equivalence<E>::operator bool() const
{
    return bool(impl);
}

template <class E>
typename edge_equivalence<E>::edge_type
edge_equivalence<E>::operator[](unsigned i) const
{
	return (*impl)[i];
}

template <class E>
void
edge_equivalence<E>::sort()
{
	impl->sort();
}

template <class E>
void edge_equivalence<E>::prune_inside(score_t worse_than) const
{
	return impl->prune_inside(worse_than);
}

template <class E>
std::size_t edge_equivalence<E>::equivalence_count()
{
	return edge_equivalence_impl<E>::equivalence_count();
}

template <class E>
edge_equivalence<E>::edge_equivalence()
  : impl()
{}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
edge_equivalence_pool<ET>::edge_equivalence_pool(std::size_t max_allowed)
: max_allowed(max_allowed){}

template <class ET>
void edge_equivalence_pool<ET>::max_equivalences(std::size_t m)
{
    max_allowed=m;
}

template <class ET>
std::size_t edge_equivalence_pool<ET>::max_equivalences() const
{
    return max_allowed;
}

template <class ET>
typename edge_equivalence_pool<ET>::edge_equiv_type
edge_equivalence_pool<ET>::create(edge_type const& e)
{
    boost::intrusive_ptr<equiv_impl_t> ptr(create_p(e));
    return edge_equiv_type(ptr);
}

template <class ET>
typename edge_equivalence_pool<ET>::equiv_impl_t*
edge_equivalence_pool<ET>::create_p(edge_type const& a)
{
    //if (++stat.n_edge_equiv > max_allowed)
    //    throw edge_pool_bad_alloc( "reached preset limit on number "
   //                                "of edge equivalences" );
//    equiv_impl_t* pe = pool->construct(a);
//    if (!pe)
//        throw edge_pool_bad_alloc("memory exhausted combining edge");

      return new equiv_impl_t(a);
//    return pe;
}

template <class ET>
typename edge_equivalence_pool<ET>::edge_equiv_type
edge_equivalence_pool<ET>::transfer( edge_equiv_type eq
                                   , edge_equivalence_pool& other )
{
//    edge_equiv_type new_eq = create(eq.representative());
//    new_eq.get()->swap_alts(*eq.get());
//    return new_eq;
    return eq;
}

template <class ET>
void edge_equivalence_pool<ET>::swap(edge_equivalence_pool& other)
{
    std::swap(max_allowed,other.max_allowed);
    //std::swap(stat,other.stat);
 //   pool.swap(other.pool);
}

template <class ET>
void edge_equivalence_pool<ET>::reset()
{
   //stat.n_edge_equiv = 0;
   // pool.reset(new boost::object_pool<equiv_impl_t>());
}


}// namespace sbmt
