# if ! defined(GUSC__GENERATOR__LAZY_SEQUENCE_HPP)
# define       GUSC__GENERATOR__LAZY_SEQUENCE_HPP

# include <boost/shared_ptr.hpp>
# include <boost/iterator/iterator_facade.hpp>
# include <boost/utility/result_of.hpp>
# include <boost/integer_traits.hpp>
# include <vector>
# include <iostream>

namespace gusc {

template <class Generator> class lazy_sequence_iterator;

////////////////////////////////////////////////////////////////////////////////
///
///  lazy_sequence backs a generator by a random access container, to memoize
///  the values in the generator on demand
///  
////////////////////////////////////////////////////////////////////////////////
template <class Generator>
class lazy_sequence {
public:
    typedef typename boost::result_of<Generator()>::type value_type;
    typedef value_type const& reference;
    typedef reference const_reference;
    typedef lazy_sequence_iterator<Generator> iterator;
    typedef iterator const_iterator;
private:
    typedef std::vector<value_type> container;
    typedef typename std::vector<value_type>::iterator raw_iterator;
public:
    iterator begin();
    iterator end();
    reference operator[](size_t n)
    {
        instantiate(n);
        return c.at(n);
    }

    lazy_sequence(Generator const& g) : g(g) {}
    lazy_sequence() {}
    
private:
    Generator g;
    container c;
    void instantiate(ptrdiff_t n = boost::integer_traits<ptrdiff_t>::const_max)
    {
        if (n < 0 or c.size() > size_t(n) or (not g)) return;
        
        size_t curr_size = c.size();
        while (curr_size <= size_t(n) and bool(g)) {
            value_type ag = g();
            //std::cerr << "("<<this<<")(" << &c[0] << ").push_back(" << ag << ")\n";
            c.push_back(ag);
            curr_size = c.size();
        }
        //std::cerr << "\ninstantiated-vector: ("<<this<<")(" << &c[0] << ") ";
        //copy(c.begin(),c.end(),std::ostream_iterator<value_type>(std::cerr," "));
        //std::cerr << "\n";
    }

    raw_iterator to_raw(ptrdiff_t n) { return c.begin() + n; }
    ptrdiff_t from_raw(raw_iterator p) { return p - c.begin(); }
    
    friend class lazy_sequence_iterator<Generator>;
};

////////////////////////////////////////////////////////////////////////////////

template <class Generator>
class lazy_sequence_iterator 
  : public boost::iterator_facade< 
               lazy_sequence_iterator<Generator>
             , typename lazy_sequence<Generator>::value_type
             , boost::random_access_traversal_tag
             , typename lazy_sequence<Generator>::reference 
             > {
public:
    typedef typename lazy_sequence_iterator<Generator>::value_type value_type;
    typedef typename lazy_sequence_iterator<Generator>::reference reference;
private:
    lazy_sequence<Generator>* self;
    ptrdiff_t idx;
    bool at_end;
    
    ptrdiff_t distance_to(lazy_sequence_iterator<Generator> const& ptr) const
    {
        return ptr.idx - idx;
    }
    
    void advance(ptrdiff_t n)
    {
        if (at_end) {
            if (n >= 0) return;
            self->instantiate();
            idx = self->c.size() + n;
            at_end = false;
        } else {
            if (n > 0) {
                self->instantiate(idx + n);
                idx += n;
                if (idx >= 0 and size_t(idx) >= self->c.size()) {
                    at_end = true;
                    idx = self->c.size();
                }
            } else {
                idx += n;
            }
        }
    }
    
    void increment()
    {
        advance(1);
    }
    
    void decrement()
    {
        advance(-1);
    }
    
    value_type const& dereference() const 
    { 
        return (*self)[idx];
    }
    
    bool equal(lazy_sequence_iterator<Generator> const& other) const
    {
        if (at_end) return other.at_end;
        else return (not other.at_end) and (idx == other.idx);
    }
    
    lazy_sequence_iterator(lazy_sequence<Generator>* s, ptrdiff_t i, bool ae)
      : self(s), idx(i), at_end(ae) 
    {
        self->instantiate(idx);
        if (idx < 0 or self->c.size() <= size_t(idx)) {
            //std::cerr <<"really im at end: c.size="<<self->c.size()<<" idx="<<idx<<"\n";
            at_end = true;
            idx = self->c.size();
        } else {
            if (not at_end) {
                //std::cerr<<"look at what i point at: "<<dereference()<<"\n";
            }
        }
    }
    
    friend class boost::iterator_core_access;
    friend class lazy_sequence<Generator>;
public:
    lazy_sequence_iterator() : self(NULL), idx(0), at_end(true) {}
    
    void print(std::ostream& os) const
    {
        os << "self=" << (void*)(self) << ", idx=" << idx << " at_end=" << std::boolalpha << at_end;
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class Generator>
lazy_sequence_iterator<Generator> lazy_sequence<Generator>::begin() 
{ 
    return iterator(this,0,false); 
}

////////////////////////////////////////////////////////////////////////////////

template <class Generator>
lazy_sequence_iterator<Generator> lazy_sequence<Generator>::end() 
{ 
    return iterator(this,0,true); 
}

////////////////////////////////////////////////////////////////////////////////

template <class C, class T, class G>
std::basic_ostream<C,T>& 
operator << (std::basic_ostream<C,T>& os, lazy_sequence_iterator<G> const& p)
{
    p.print(os);
    return os;
}

template <class Generator>
lazy_sequence<Generator> 
make_lazy_sequence(Generator const& g)
{
    return lazy_sequence<Generator>(g);
}

////////////////////////////////////////////////////////////////////////////////
///
///  shared_lazy_sequence has the same behavior and purpose of a lazy_sequence
///  but is also a lightweight object.  it is copied by pointer.  note this 
///  does not change the semantics of any operations, since the contents of 
///  the sequences is const
///
////////////////////////////////////////////////////////////////////////////////
template <class Generator>
class shared_lazy_sequence {
public:
    typedef typename boost::result_of<Generator()>::type value_type;
    typedef value_type const& reference;
    typedef reference const_reference;
    typedef lazy_sequence_iterator<Generator> iterator;
    typedef iterator const_iterator;
private:
    typedef std::vector<value_type> container;
    typedef typename std::vector<value_type>::iterator raw_iterator;
public:
    iterator begin() const { return seq->begin(); }
    iterator end() const { return seq->end(); }
    reference operator[](size_t n) const { return seq->operator[](n); }
    
    shared_lazy_sequence(Generator const& g) 
      : seq(new lazy_sequence<Generator>(g)) {}
    template <class ConvertibleToSharedPtr>
    shared_lazy_sequence(ConvertibleToSharedPtr ptr)
      : seq(ptr) {}
    shared_lazy_sequence() {}
    
private:
    boost::shared_ptr< lazy_sequence<Generator> > seq;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__GENERATOR__LAZY_SEQUENCE_HPP


