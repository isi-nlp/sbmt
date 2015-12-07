# if ! defined(GUSC__ITERATOR__OSTREAM_ITERATOR)
# define       GUSC__ITERATOR__OSTREAM_ITERATOR

# include <boost/function_output_iterator.hpp>
# include <boost/iterator/iterator_adaptor.hpp>

namespace gusc {
    
namespace internal {

////////////////////////////////////////////////////////////////////////////////

struct writeout {
    
    std::string delim;
    std::ostream* out;
    bool first;
    writeout(std::ostream& out, std::string const& delim)
    : delim(delim)
    , out(&out) 
    , first(true) {}
    
    template <class X>
    void operator()(X const& x)
    {
        if (not first) { (*out) << delim; }
        (*out) << x;
        first = false;
    }
};

////////////////////////////////////////////////////////////////////////////////

}

////////////////////////////////////////////////////////////////////////////////

typedef boost::function_output_iterator<internal::writeout> ostream_iterator_;

inline ostream_iterator_ 
ostream_iterator(std::ostream& out, std::string const& delim = "")
{
    return ostream_iterator_(internal::writeout(out,delim));
}


////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__ITERATOR__OSTREAM_ITERATOR
