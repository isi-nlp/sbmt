#include <algorithm>
#include <vector>
#include <functional>
#include <boost/tuple/tuple.hpp>

namespace sbmt {

namespace detail { namespace cube {

/// this is much closer to David's idea:  heapify the entire cube, and then process it,
template < class A, class B, class C, class Pred, class HeapOrder >
class cube_heap 
{
    typedef boost::tuple<A,B,C> tuple_type;
    Pred      pred;
    HeapOrder heaporder;
public:
    cube_heap( Pred pred = Pred()
             , HeapOrder h = HeapOrder() )
    : pred(pred)
    , heaporder(h) {}
     
    template <class RA, class RB, class RC>
    void operator()( RA& ra, RB& rb, RC& rc)
    {
        using boost::get;
        std::size_t s = (ra.end() - ra.begin()) * 
                        (rb.end() - rb.begin()) *
                        (rc.end() - rc.begin()) ;
        //std::cout << "cube size: "<< s << std::endl;
        
        std::vector<tuple_type> heap(s);
        std::size_t x = 0;
        for (typename RA::iterator aitr = ra.begin(); aitr != ra.end(); ++aitr)
        for (typename RB::iterator bitr = rb.begin(); bitr != rb.end(); ++bitr)
        for (typename RC::iterator citr = rc.begin(); citr != rc.end(); ++citr) {
            heap[x] = tuple_type(*aitr,*bitr,*citr);
            ++x;
        }
        
        std::make_heap(heap.begin(),heap.end(),heaporder);
        
        while ( x > 0 and 
               (pred(get<0>(heap[0]),get<1>(heap[0]),get<2>(heap[0])) != false))
        {
            std::pop_heap(heap.begin(), heap.begin() + x, heaporder);
            --x;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////
///
/// not written because of an obsession with generalization (though i do have
/// that problem).  written because the loop is easier to test this way.
/// instead of combining actual edges, i can test the loop with normal numbers
///
////////////////////////////////////////////////////////////////////////////////
template < class A, class B, class C
         , class Pred
         , class CA = std::greater<A>
         , class CB = std::greater<B>
         , class CC = std::greater<C> >
class cube_loop {
public: 
    cube_loop( Pred pred = Pred() 
             , CA  compa = CA()
             , CB  compb = CB()
             , CC  compc = CC() )
    : pred(pred)
    , compa(compa)
    , compb(compb)
    , compc(compc) {}
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// in notes below: (x,y,z) is an item in cube, x is outermost loop
    /// z is innermost loop.  the cubic assumption is that for
    /// (x0,y0,z0) and (x1,y1,z1) with x0 <= x1 y0 <= y1 and z0 <= z1 then if
    /// (x0,y0,z0) is rejected, then so is (x1,y1,z1)
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class RA, class RB, class RC>
    void operator()( RA& ra, RB& rb, RC& rc )
    {
        using namespace std;
    
        sort(ra.begin(),  ra.end(), compa);
        sort(rb.begin(),  rb.end(), compb);
        sort(rc.begin(),  rc.end(), compc);
    
        typename RA::iterator raitr  = ra.begin();
        typename RA::iterator raend  = ra.end();
    
        typename RB::iterator rbitr, rbend;
        typename RC::iterator rcitr, rcend;
    
        
        rbend = rb.end();
        
        for (; raitr != raend; ++raitr) {
            rbitr = rb.begin();
            rcend = rc.end();
            for (; rbitr != rbend; ++rbitr) {
                rcitr = rc.begin();
                for (; rcitr != rcend; ++rcitr) {
                    if(!pred(*raitr, *rbitr, *rcitr)) {
                        break;
                    } 
                }
                
                ////////////////////////////////////////////////////////////////
                ///
                /// since (x0,y0,z0) failed, for fixed x and increasing y, 
                /// we can assume that (x0,y,z0) will fail by cubic assumption,
                /// so set stopping condition to z0 in subsequent inner loops.
                ///
                ////////////////////////////////////////////////////////////////
                rcend = rcitr;
                
                ////////////////////////////////////////////////////////////////
                ///
                /// we are in a situation where (x,y,0) fails
                /// so, for fixed x and increasing y, cubic assumption says it
                /// will continue to fail, so we can break out of loop.
                ///
                ////////////////////////////////////////////////////////////////
                if (rcend == rc.begin()) break;
            }
            
            ////////////////////////////////////////////////////////////////////
            ///
            /// if (x,y0,0) fails...
            /// then for increasing x, (x,y0,0) will fail, so we can take
            /// y0 as the new end marker, and not bother exploring 
            /// (x,y0+1,0), (x,y0+2,0) etc.
            ///
            ////////////////////////////////////////////////////////////////////
            if (rcend == rc.begin()) rbend = rbitr;

            ////////////////////////////////////////////////////////////////////
            ///
            /// if x,0,0 fails, then for increasing x, everything will fail.
            /// so exit.
            ///
            ////////////////////////////////////////////////////////////////////
            if (rbend == rb.begin() and rcend == rc.begin()) break;
        }
    }
private:
    Pred pred;
    CA   compa;
    CB   compb;
    CC   compc;
};

////////////////////////////////////////////////////////////////////////////////

template<class GT, class ET>
struct prior_order
{
    typedef boost::tuple<typename GT::rule_type,
                         edge_equivalence<ET>,
                         edge_equivalence<ET> > tuple_type;
    bool operator()(tuple_type const& x, tuple_type const& y) const
    {
        using boost::get;
        return 
        pessimistic_rule_score(gram, get<0>(x)) * 
        get<1>(x).representative().score() * 
        get<2>(x).representative().score() <
        pessimistic_rule_score(gram, get<0>(y)) * 
        get<1>(y).representative().score() * 
        get<2>(y).representative().score() ;
    }
    prior_order(GT& gram):gram(gram){}
private:
    GT& gram;
};

template<class GT>
struct greater_rule_score
{
    bool operator()( typename GT::rule_type const& r1
                   , typename GT::rule_type const& r2 ) const
    {
        return pessimistic_rule_score(gram,r1) > 
               pessimistic_rule_score(gram,r2);

    }
    
    greater_rule_score(GT& gram) : gram(gram) {}
private:
    GT& gram;
};

}} // namespace detail::cube

////////////////////////////////////////////////////////////////////////////////

}
