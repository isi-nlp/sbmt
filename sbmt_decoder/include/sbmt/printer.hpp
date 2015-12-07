#ifndef SBMT__PRINTER_HPP
#define SBMT__PRINTER_HPP

#include <iosfwd>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  \defgroup printing Printing indexed tokens
///
///  in order to keep memory usage down, the sbmt library makes heavy use of
///  dictionaries to avoid redundant storage of strings.  for instance, in a 
///  grammar, the strings "NP" or "indonesia" probably show up a lot.  so rather
///  than store them in every rule, an index to the strings storage is held.  
///  and the index is not a pointer, since on 64 bit architectures, that can get
///  pretty heavy on memory usage as well.  to recover the string, one is 
///  required to pass the index to a dictionary.  we decided not to make 
///  dictionaries global, because in my (michael) limited experience as a 
///  programmer, i have almost always regretted the use of global structures 
///  early in a design phase.  you can always make a local object global later 
///  in the development process, but the opposite is trickier.
///
///  one problem with dictionaries, however, comes up when you want to display 
///  your object via the standard streaming interface.  how do you pass the 
///  dictionary along to the stream with your object?
///
///  the interface we adopt works like this:  say i want to make it easy to
///  send vector<indexed_token> to a stream.  i write a function that 
///  looks like this:
///  \code
///     template<class OS, class TF>
///     void print(OS& os, vector<indexed_token> const& v, TF const& tf)
///     {
///          vector<indexed_token>::const_iterator itr = v.begin();
///          vector<indexed_token>::const_iterator end = v.end();
///          for(; itr != end; ++itr) os << print(*itr,tf) << " ";
///     }
///  \endcode
///
///  when someone goes to print such a vector, they don't use my print function
///  directly.  instead, they use a helper print function (already written),
///  which calls my function:
///
///  \code
///     indexed_token_factory tf;
///     vector<indexed_token> v = create_my_tokens("A B C", tf); // made-up
///     cout << print(v,tf);
///  \endcode
///
///  this helper print function basically creates a light-weight object that
///  holds references to my vector and the dictionary (or token-factory, or
///  anything with the dictionary interface), and knows to call the appropriate
///  print function when it gets thrown in a stream.  this is a very similar 
///  method to how iostream manipulators (like std::setw and std::endl) work.
///
///  using this methodology, you can make printing functions easily for objects
///  that are composed of simpler objects that have indexed tokens as data,
///  without having to abandon the c++ stream interface or anything that uses it.
///
////////////////////////////////////////////////////////////////////////////////
///\{

////////////////////////////////////////////////////////////////////////////////
///
///  binds an object containing token items to its dictionary, token factory.
///  there is no need to create one yourself, just use print(obj,dict) inside
///  an iostream call.
///
///  example:
///
///  in_memory_token_factory tf;
///  indexed_syntax_rule rule("S(x0:NP x1:VP) -> x0 x1 ### id=1",tf);
///  cout << print(rule, tf) << endl;
///  \code
///
///  result:
///  \code
///  S(x0:NP x1:VP) -> x0 x1 ### id=1
///  \endcode
///
////////////////////////////////////////////////////////////////////////////////
template<class X, class TFT>
struct printer
{
    printer(X const& x_, TFT const& tft_)
    : x(x_)
    , tft(tft_) {}    
    X const& x;
    TFT const& tft;
    operator X const& () { return x; }
};

////////////////////////////////////////////////////////////////////////////////


template <class CH, class CT, class X, class TFT>
std::basic_ostream<CH,CT>&
operator<<(std::basic_ostream<CH,CT>& out, printer<X,TFT> const& p)

/*
template <class O, class X, class TFT>
O&
operator<<(O& out, printer<X,TFT> const& p)
*/
{
    print(out, p.x, p.tft);
    return out;
}

////////////////////////////////////////////////////////////////////////////////

template <class X, class Y>
printer< X,Y > 
print(X const& x, Y const& tf)
{
    return printer< X,Y >(x,tf);
}

///\}
////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif
