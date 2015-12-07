# ifndef   SBMT_IO_DETAIL_LOGGING_FILTER_HPP
# define   SBMT_IO_DETAIL_LOGGING_FILTER_HPP

# include <boost/iostreams/categories.hpp>
# include <boost/iostreams/operations.hpp>
# include <boost/utility.hpp>
# include <iostream>

namespace sbmt { namespace io { namespace detail {

////////////////////////////////////////////////////////////////////////////////
///
///  a simple filter; depending on its state, it can either accept all data 
///  coming through it unchanged, or it can ignore all data coming through it.
///
///  the idea is that someone will want to change its ignoring state after 
///  seeing a stream manipulator of some kind.  In our code, the presence of the
///  logging_level manipulator in a logging_stream will cause the filter to 
///  change its state, so that input below a certain logging level threshold
///  will be discarded in the logging file.
///
////////////////////////////////////////////////////////////////////////////////
class logging_filter
{
public:
    typedef char char_type;
    struct category 
      : boost::iostreams::multichar_output_filter_tag {}; 
    
    logging_filter(logging_level ignore_above, logging_level* current_level)
      : ignore_above(ignore_above)
      , current_level(current_level) {}
    
    template <class SinkT>
      std::streamsize write(SinkT& sink, char const* s, std::streamsize n);
    
private:
    logging_level  ignore_above;
    logging_level* current_level; 
};

////////////////////////////////////////////////////////////////////////////////

template <class SinkT> 
std::streamsize 
logging_filter::write(SinkT& sink, char const* s, std::streamsize in)
{
    if (*current_level <= ignore_above)  {
        //std::cout << "* filter writing:\"";
        //for (int i = 0; i != in; ++i) std::cout << *(s + i);
        //std::cout << "\"" << std::endl;
        return boost::iostreams::write(sink,s,in);
    }
    else return in;
}

////////////////////////////////////////////////////////////////////////////////

} } } // namespace sbmt::io::detail

# endif // SBMT_IO_DETAIL_LOGGING_FILTER_HPP
