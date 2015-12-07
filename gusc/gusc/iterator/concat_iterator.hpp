# if ! defined(GUSC__ITERATOR__CONCAT_ITERATOR_HPP)
# define       GUSC__ITERATOR__CONCAT_ITERATOR_HPP

# include <boost/iterator/iterator_adaptor.hpp>
# include <boost/iterator/iterator_facade.hpp>
# include <boost/range.hpp>
# include <boost/tuple/tuple.hpp>

namespace gusc {
    
template <class Key>
class equal_range_extractor {
public:
    template <class S>
    std::pair<
      typename boost::range_iterator<S>::type
    , typename boost::range_iterator<S>::type
    > operator()(S& s) const
    {
        return s.equal_range(k);
    }
    
    equal_range_extractor(Key const& k) : k(k) {}
    
private:
    Key k;
};

template <class Key>
equal_range_extractor<Key> make_equal_range_extractor(Key const& k)
{
    return equal_range_extractor<Key>(k);
}

class begin_end_extractor {
public:
    template <class S>
    std::pair<
      typename boost::range_iterator<S>::type
    , typename boost::range_iterator<S>::type
    > operator()(S& s) const
    {
        return std::make_pair(s.begin(),s.end());
    }
};

template <class Sequence, class Iterator, class Extractor>
class sequence_of_iterators 
: public boost::iterator_facade<
           sequence_of_iterators<Sequence,Iterator,Extractor>
         , typename boost::iterator_value<Iterator>::type
         , boost::forward_traversal_tag
         , typename boost::iterator_reference<Iterator>::type
         , typename boost::iterator_difference<Iterator>::type
         > {

    Extractor extract;
    typename boost::range_iterator<Sequence>::type scurr;
    typename boost::range_iterator<Sequence>::type send;
    Iterator curr;
    Iterator end;
public:
    sequence_of_iterators(Sequence& seq, Extractor const& extract, bool begin = true)
    : extract(extract) 
    {
        scurr = begin ? boost::begin(seq) : boost::end(seq);
        send = boost::end(seq);
        if (begin and scurr != send) {
            boost::tie(curr,end) = extract(*scurr);
            skip_empties();
        }
    }
    
private:
    void skip_empties() 
    {
        if (curr == end and scurr != send) {
            while (curr == end) {
                ++scurr;
                if (scurr == send) break;
                boost::tie(curr,end) = extract(*scurr);
            }
        }   
    }
    
    friend class boost::iterator_core_access;
    
    bool equal(sequence_of_iterators const& other) const
    {
        if (scurr != other.scurr) return false;
        if (scurr == send) return true;
        return curr == other.curr;
    }
    
    typename boost::iterator_reference<Iterator>::type dereference() const
    {
        return *curr;
    }
    
    void increment()
    {
        ++curr;
        skip_empties();
    }
};

////////////////////////////////////////////////////////////////////////////////

/// concatenates two iterator ranges
template <class I>
class concat_iterator 
: public boost::iterator_adaptor<
             concat_iterator<I>
           , I
           , boost::use_default
           , boost::bidirectional_traversal_tag
         > {
    typedef boost::iterator_adaptor<
                concat_iterator<I>
              , I
              , boost::use_default
              , boost::bidirectional_traversal_tag
            > base_;
public:
    concat_iterator(I itr, I end1, I beg2, bool atend = false)
      : base_(itr)
      , end1(end1)
      , beg2(beg2)
      , second(atend)
    {
        if (base_::base_reference() == end1 and not second) {
            base_::base_reference() = beg2;
            second = true;
        }
        # if(GUSC__ITERATOR__REVERSE_DEBUG)
        std::clog << "orig: " << itr << "  "; 
        std::clog << "i: " << base_::base_reference() << "  ";
        std::clog << "e1: " << end1 << "  "; 
        std::clog << "b2: " << beg2 << "  " << std::endl; 
        # endif
    }
    
    void increment()
    {
        # if(GUSC__ITERATOR__REVERSE_DEBUG)
        std::clog << "+" << *base_::base_reference() << "+" 
                  << second << std::endl;
        # endif
        
        ++base_::base_reference();
        if (base_::base_reference() == end1 and not second) {
            base_::base_reference() = beg2;
            second = true;
        }
    }
    
    bool equal(concat_iterator const& other) const
    {
        return base_::base_reference() == other.base_reference() and
               second == other.second;
    }
    
    void decrement()
    {
        
        if (base_::base_reference() == beg2 and second) {
            base_::base_reference() = end1;
            second = false;
        }
        --base_::base_reference();
        # if(GUSC__ITERATOR__REVERSE_DEBUG)
        std::clog << "-" << *base_::base_reference() << "-" 
                  << second << std::endl;
        # endif
    }
    
private:
    I end1;
    I beg2;
    bool second;
};

////////////////////////////////////////////////////////////////////////////////

template <class I>
boost::iterator_range< concat_iterator<I> >
concatenate(I beg1, I end1, I beg2, I end2)
{
    # if(GUSC__ITERATOR__REVERSE_DEBUG)
    std::clog << "[" << beg1 << "," << end1 << ") ";
    std::clog << "[" << beg2 << "," << end2 << ") " << std::endl;
    # endif
    return boost::make_iterator_range(
             concat_iterator<I>(beg1,end1,beg2)
           , concat_iterator<I>(end2,end1,beg2,true)
           )
           ;
}

////////////////////////////////////////////////////////////////////////////////

template <class R>
boost::iterator_range< 
    concat_iterator<typename boost::range_result_iterator<R>::type> 
>
concatenate_ranges(R& r1, R& r2)
{
    return concatenate( boost::begin(r1), boost::end(r1)
                      , boost::begin(r2), boost::end(r2)
                      );
}

////////////////////////////////////////////////////////////////////////////////

template <class R>
boost::iterator_range< 
    concat_iterator<typename boost::range_const_iterator<R>::type> 
>
concatenate_ranges(R const& r1, R const& r2)
{
    return concatenate( boost::begin(r1), boost::end(r1)
                      , boost::begin(r2), boost::end(r2)
                      );
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__ITERATOR__CONCAT_ITERATOR_HPP
