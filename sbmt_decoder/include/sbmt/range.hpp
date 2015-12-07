#ifndef   SBMT_RANGE_HPP
#define   SBMT_RANGE_HPP

#include <cstddef>

#define DEBUG_VALGRIND

#ifdef DEBUG_VALGRIND
#include <graehl/shared/podcpy.hpp>
#endif

#include <iterator>
#include <boost/iterator/iterator_facade.hpp>

namespace sbmt {

template <class Range>
bool empty_range(Range const& range)
{
    return range.begin() == range.end();
}

////////////////////////////////////////////////////////////////////////////////
///
///  the purpose of a range concept is the following:  sometimes, you cant store
///  everything in a uniform collection.  sometimes, when a range of items is
///  requested, you actually have to pull the appropriate items from a file or
///  database, and return a temporary collection whose items can be accessed via
///  iterator.
///
///  other times, all the items _are_ in a permanent collection, and a subset 
///  can be accessed by providing a pair of iterators to the items.  A range 
///  interface allows both ideas.
///
///  when given a range, you should assume the range is light-weight.  all 
///  heavy work to provide the items occurred before the range was passed to 
///  you.  the range might be as simple as a pair of iterators, or it 
///  might hold a smart pointer to a collection of elements.  you can access
///  iterators into the range via begin() and end() methods.  you should assume
///  all iterators from the range are invalid when the range type goes out of
///  scope.
///
////////////////////////////////////////////////////////////////////////////////
template <class ItrT> 
class itr_pair_range
{
public:
    typedef ItrT iterator;
    typedef ItrT const_iterator;
    
    itr_pair_range(ItrT begin_, ItrT end_)
    : m_begin(begin_)
    , m_end(end_) {}
    
    itr_pair_range() {
#ifdef DEBUG_VALGRIND
        graehl::podzero(m_begin);
#endif
        m_end=m_begin;
    }
    
    
    template<class IT>
    explicit itr_pair_range(std::pair<IT,IT> const& p)
    : m_begin(p.first)
    , m_end(p.second) {}

    iterator begin() const { return m_begin; }
    iterator end()   const { return m_end; }

    operator bool() const 
    {
        return m_begin!=m_end;
    }
    void operator ++()
    {
        ++m_begin;
    }
    typedef typename std::iterator_traits<iterator>::value_type value_type;
    value_type const& operator *() const 
    {
        return *m_begin;
    }

    
private:
    ItrT m_begin;
    ItrT m_end;
};

template <class ItrT> 
itr_pair_range<ItrT> make_itr_pair_range(ItrT begin,ItrT end) 
{
    return itr_pair_range<ItrT>(begin,end);
}

template <class Cont> 
itr_pair_range<typename Cont::iterator> make_itr_pair_range(Cont &c)
{
    return
        itr_pair_range<typename Cont::iterator> 
        (c.begin(),c.end());
}

template <class Cont> 
itr_pair_range<typename Cont::iterator> make_itr_pair_range(Cont const& c)
{
    return
        itr_pair_range<typename Cont::iterator> 
        (c.begin(),c.end());
}

template <class ItrT> 
void make_empty(itr_pair_range<ItrT> &r)
{
    r.begin=ItrT(); // note: even for void *, should actually init to 0
    r.end=r.begin();
}

/// A reference wrapper for containers, satisfying the range concept
template <class Cont>
class cont_range
{
 public:
    typedef Cont container_type;
    typedef cont_range<Cont> self_type;
    typedef typename container_type::iterator iterator;
    iterator begin() const { return p->begin(); }
    iterator end() const { return p->end(); }
    cont_range(container_type &c) : p(&c) {}
    cont_range(self_type const& o) : p(o.p) {}
 private:
    container_type *p;
};


template <class Cont>
cont_range<Cont> make_cont_range(Cont &cont) 
{
    return cont_range<Cont>(cont);
}

/// A const reference wrapper for containers, satisfying the range concept
/// (identical to above but const_iterator, const container *)
template <class Cont>
class const_cont_range
{
 public:
    typedef Cont container;
    typedef typename container::const_iterator iterator;
    iterator begin() const { return p->begin(); }
    iterator end() const { return p->end(); }
    explicit const_cont_range(const container &c) : p(&c) {}
 private:
    const container *p;
};
   
template <class Cont>
cont_range<Cont> make_const_cont_range(const Cont &cont) 
{
    return const_cont_range<Cont>(cont);
}

template <class Range>
std::size_t range_size(Range const& range)
{
    std::size_t s=0;
    for (typename Range::iterator i=range.begin(),e=range.end();i!=e;++i)
        ++s;
    return s;
}


template <class I1,class I2=I1>
struct concat_iterator : public
boost::iterator_facade<concat_iterator<I1,I2>,typename std::iterator_traits<I1>::value_type,boost::forward_traversal_tag>
{
    typedef concat_iterator<I1,I2> self_type;
    typedef typename std::iterator_traits<I1>::value_type value_type;
    I1 i1,e1;
    I2 i2,e2;
    void increment() 
    {
        if (i1!=e1)
            ++i1;
        else
            ++i2;
    }
    bool equal(self_type const& o) 
    {
        return i2==o.i2;
    }
    value_type &dereference() const
    {
        if (i1!=e1)
            return const_cast<value_type&>(*i1);
        else
            return const_cast<value_type&>(*i2);
    }
    concat_iterator(I2 i2) : i2(i2) {} // for init end.
    concat_iterator(I1 i1,I1 e1,I2 i2,I2 e2) : i1(i1),e1(e1),i2(i2),e2(e2) {}
    template <class C1,class C2>
    concat_iterator(C1 const&a,C1 const&b)  : i1(a.begin()),e1(a.end()),i2(b.begin()),e2(b.end()) {}    
};

template <class R1,class R2>
class concat_ranges 
{
    typedef typename R1::iterator I1;
    typedef typename R2::iterator I2;
 public:
    typedef concat_iterator<I1,I2> iterator;
    iterator begin() const 
    { return i; }
    iterator end() const
    { return e; }
    concat_ranges(R1 const&r1,R2 const&r2) : i(r1,r2),e(r2.end()) {}
 private:
    iterator i,e;
};

    

}

#endif // SBMT_RANGE_HPP
