#ifndef   SBMT_SEARCH_CUBE_HEAP_HPP
#define   SBMT_SEARCH_CUBE_HEAP_HPP
#define  SBMT_CUBE_HEAP_REDUNDANT_PATHS 1

#include <vector>
#include <string>
#include <sstream>
#include <boost/functional/hash.hpp>
#include <stdexcept>
#include <boost/range.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class A, class B, class C>
class sorted_cube
{
public:
    typedef typename std::iterator_traits<A>::reference a_reference;
    typedef typename std::iterator_traits<B>::reference b_reference;
    typedef typename std::iterator_traits<C>::reference c_reference;
    
    class item
    {
    public:
        a_reference x() const { return *xitr; }
        
        b_reference y() const { return *yitr; }
        
        c_reference z() const { return *zitr; }
        
        item successor(int xyz) const;
        
        std::string position_string() const;
        
        bool has_successor(int xyz) const;
        
        std::size_t hash_value() const
        {
            using boost::hash_combine;
            size_t retval(0);
            hash_combine(retval,size_t(parent));
            hash_combine(retval,size_t(&(*xitr)));
            hash_combine(retval,size_t(&(*yitr)));
            hash_combine(retval,size_t(&(*zitr)));
            return retval;
        }
        
        bool operator==(item const& other) const
        {
            return parent == other.parent and 
                   xitr   == other.xitr   and
                   yitr   == other.yitr   and
                   zitr   == other.zitr;
        } 
        
    private:
        friend class sorted_cube;
        item( sorted_cube const* parent
            , A xitr
            , B yitr
            , C zitr )
        : parent(parent), xitr(xitr), yitr(yitr), zitr(zitr) {}
        
        sorted_cube const* parent;
        A xitr;
        B yitr;
        C zitr;
    };
    
    template <class AR, class BR, class CR>
    sorted_cube( AR const& xrange, BR const& yrange, CR const& zrange )
    : xbegin(boost::begin(xrange))
    , xend(boost::end(xrange))
    , ybegin(boost::begin(yrange))
    , yend(boost::end(yrange))
    , zbegin(boost::begin(zrange))
    , zend(boost::end(zrange)) {}
    
    template <class AR, class BR, class CR>
    void fill( AR const& xrange, BR const& yrange, CR const& zrange );
    
    bool empty() const;
    item corner() const;
    std::size_t size() const; /// O(sum of lengths of edges of cube)
    
private:
    friend class item;
    A xbegin; A xend;
    B ybegin; B yend;
    C zbegin; C zend;
};

template <class A, class B, class C>
std::size_t hash_value(typename sorted_cube<A,B,C>::item const& itm)
{
    return itm.hash_value();
}

template <class  X> struct member_hash
{
    std::size_t operator()(X const& x) const { return x.hash_value(); }
};



////////////////////////////////////////////////////////////////////////////////

template <class A, class B, class C>
bool sorted_cube<A,B,C>::item::has_successor(int xyz) const
{
    A xx(xitr); B yy(yitr); C zz(zitr);
    switch (xyz) {
        #if SBMT_CUBE_HEAP_REDUNDANT_PATHS
        case 1:
            if (xitr == parent->xend) return false; 
            ++xx;
            return xx != parent->xend;
            break;
        case 2:
            if (xitr != parent->xbegin) return false;
            if (yitr == parent->yend) return false; 
            ++yy;
            return yy != parent->yend;
            break;
        case 3:
            if (xitr != parent->xbegin) return false;
            if (yitr != parent->ybegin) return false;
            if (zitr == parent->zend) return false; 
            ++zz;
            return zz != parent->zend;
            break;
            
        #else 
        
        case 1:
            if (xitr == parent->xend) return false; 
            ++xx;
            return xx != parent->xend;
            break;
        case 2:
            if (yitr == parent->yend) return false; 
            ++yy;
            return yy != parent->yend;
            break;
        case 3:
            if (zitr == parent->zend) return false; 
            ++zz;
            return zz != parent->zend;
            break;
            
        #endif // SBMT_CUBE_HEAP_REDUNDANT_PATHS

    default:
        throw std::logic_error("Impossible direction (should be 1,2, or 3) for sorted_cube");
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

template <class A, class B, class C>
typename sorted_cube<A,B,C>::item
sorted_cube<A,B,C>::item::successor(int xyz) const
{
    A xx(xitr); B yy(yitr); C zz(zitr);
    switch (xyz) {
        case 1:
            return item(parent, ++xx, yitr, zitr);
            break;
        case 2:
            return item(parent, xitr, ++yy, zitr);
            break;
        case 3:
            return item(parent, xitr, yitr, ++zz);
            break;
    default:
        throw std::logic_error("Impossible direction (should be 1,2, or 3) for sorted_cube");
    }
    return item(parent,xitr,yitr,zitr);
}

////////////////////////////////////////////////////////////////////////////////

template <class A, class B, class C>
bool sorted_cube<A,B,C>::empty() const
{
    return xbegin == xend or
           ybegin == yend or
           zbegin == zend;
}

template <class A, class B, class C>
std::size_t sorted_cube<A,B,C>::size() const
{
    size_t xsz(0), ysz(0), zsz(0);
    for (A xitr = xbegin; xitr != xend; ++xitr) ++xsz;
    for (B yitr = ybegin; yitr != yend; ++yitr) ++ysz;
    for (C zitr = zbegin; zitr != zend; ++zitr) ++zsz;
    return xsz * ysz * zsz;
}

////////////////////////////////////////////////////////////////////////////////

template <class A, class B, class C>
typename sorted_cube<A,B,C>::item sorted_cube<A,B,C>::corner() const
{
    return item(this, xbegin, ybegin, zbegin);
}

////////////////////////////////////////////////////////////////////////////////

template <class A, class B, class C>
template <class AR, class BR, class CR>
void sorted_cube<A,B,C>::fill( AR const& xrange
                             , BR const& yrange
                             , CR const& zrange )
{
    using boost::begin;
    using boost::end;
    xbegin = begin(xrange); xend = end(xrange);
    ybegin = begin(yrange); yend = end(yrange);
    zbegin = begin(zrange); zend = end(zrange);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT_SEARCH_CUBE_HEAP_HPP
