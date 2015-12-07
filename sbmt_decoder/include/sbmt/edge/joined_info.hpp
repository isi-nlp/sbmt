# if ! defined(SBMT__EDGE__JOINED_INFO_HPP)
# define       SBMT__EDGE__JOINED_INFO_HPP

# include <sbmt/edge/info_base.hpp>
//#include <boost/utility/compressed_pair.hpp>
//# include <boost/detail/compressed_pair.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  joined_info is what allows orthogonal state information to be glued
///  together efficiently.
///
///  two joined_info objects are equivalent only if both components are.
///  hashing is kept consistent with the two by using boosts simple
///  hash_combine to mix the two hashes.
///
///  \note Info1 and Info2 are inherrited, as opposed to being members.  
///  Two reasons:
///  # allows for the scheme by which at any point an edge_info can be cast
///    to any of its components.
///  # allows for empty-base-optimization: if either type is empty, then the
///    size of joined_info is the size of the other type.
///
////////////////////////////////////////////////////////////////////////////////
template <class Info1, class Info2>
class joined_info 
  : public info_base< joined_info<Info1, Info2> >
  , public Info1
  , public Info2
{
public:
    typedef Info1 first_info_type;
    typedef Info2 second_info_type;
    typedef joined_info<Info1,Info2> info_type;
    typedef boost::tuple<info_type,score_t,score_t> result_type;
    
    first_info_type const& first_info() const
    { 
        return *static_cast<first_info_type const*>(this);
    }
    
    second_info_type const& second_info() const
    {
        return *static_cast<second_info_type const*>(this);
    }
    
public:
    joined_info(Info1 const& info1, Info2 const& info2)
    : Info1(info1), Info2(info2) {}
    
    joined_info() {}
    
     joined_info<Info1, Info2> & operator=(joined_info<Info1, Info2> const& o) 
     {
         const_cast<first_info_type&>(first_info()) = o.first_info();
         const_cast<second_info_type&>(second_info()) = o.second_info();
         return *this;
     }

    std::size_t hash_value() const
    {
        std::size_t retval = first_info().hash_value();
        boost::hash_combine(retval, second_info().hash_value());
        return retval;
    }
    
    bool equal_to(joined_info<Info1, Info2> const& other) const
    {
        return first_info().equal_to(other.first_info()) && 
               second_info().equal_to(other.second_info());
    }
    
    bool is_scoreable() const
    {
        return first_info().is_scoreable() &&
               second_info().is_scoreable();
    }
};

template<class C, class T, class TF, class Info1, class Info2>
void print(std::basic_ostream<C,T> &o, joined_info<Info1,Info2> const& info, 
          TF const&tf)
{
    o << print(info.first_info(), tf) << ',' << print(info.second_info(), tf);
}


//FIXME: this conforms to the old interface - see null_info, ngram_info for new interface

////////////////////////////////////////////////////////////////////////////////
///
/// some notes on joined info factory.  the factory applies your choice of 
/// weights to each component.  so in the event that three components are 
/// combined, some care must be taken.  if you have features F1, F2, F3 with
/// weights w1, w2, w3, declaring
///
/// \code
/// typedef joinF2F3   joined_info_factory<F2factory,F3factory>;
/// typedef joinF1F2F3 joined_info_factory<F1factory,joinF2F3>;
/// \endcode
///
/// works.  but to get the weights right you should do something like:
///
/// \code
/// F1factory*  f1     = new F1Factory();
/// F2factory*  f2     = new F1Factory();
/// F3factory*  f2     = new F1Factory();
/// joinF2F3*   f2f3   = new joinF2F3(w2,w3);
/// joinF1F2F3* f1f2f3 = new joinF1F2F3(w1, 1.0, f1, f2f3);
/// \endcode
///
/// (the pointers should actually be boost::shared_ptr)
///
////////////////////////////////////////////////////////////////////////////////
template <class InfoFactory1, class InfoFactory2>
class joined_info_factory
{
public:
    typedef typename InfoFactory1::info_type first_info_type;
    typedef typename InfoFactory2::info_type second_info_type;
    
    typedef joined_info<first_info_type,second_info_type> info_type;
    
    joined_info_factory()
    : info_factory1(new InfoFactory1())
    , info_factory2(new InfoFactory2()) {}
    
    joined_info_factory( boost::shared_ptr<InfoFactory1> factory1
                       , boost::shared_ptr<InfoFactory2> factory2 )
    : info_factory1(factory1)
    , info_factory2(factory2) {}

    template <class GT>
    score_t rule_heuristic(GT& gram, typename GT::rule_type r)
    {
        return info_factory1->rule_heuristic(gram,r) *
               info_factory2->rule_heuristic(gram,r);
    }
    
    InfoFactory1& first_info_factory() { return *info_factory1; }
    InfoFactory2& second_info_factory() { return *info_factory2; }

private:
    boost::shared_ptr<InfoFactory1> info_factory1;
    boost::shared_ptr<InfoFactory2> info_factory2;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__EDGE__JOINED_INFO_HPP
