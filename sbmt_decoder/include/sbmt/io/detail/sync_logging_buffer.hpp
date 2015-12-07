# if ! defined(SBMT__IO__SYNC_SHARED_BUFFER_HPP)
# define       SBMT__IO__SYNC_SHARED_BUFFER_HPP

# include <boost/thread/recursive_mutex.hpp>
# include <boost/iostreams/categories.hpp>
# include <boost/iostreams/operations.hpp>
# include <boost/utility.hpp>

# include <vector>

namespace sbmt { namespace io { namespace detail {

////////////////////////////////////////////////////////////////////////////////
///
///  controls write-access to a shared stream.
///  keeps an internal buffer which it only writes to the shared stream on a
///  flush or close request.
///  obtains a mutex lock before every write
///
////////////////////////////////////////////////////////////////////////////////
class sync_logging_buffer
{
public:
    typedef char char_type;
    struct category 
      : boost::iostreams::sink_tag
      , boost::iostreams::closable_tag {};
    
    sync_logging_buffer( std::streambuf* out
                       , boost::recursive_mutex* mtx = NULL
                       , std::size_t buffer_bytes = 1024 );

      
    std::streamsize write(char const* s, std::streamsize in);

    bool flush();
    
    void close();
    
    std::streambuf* rdbuf();
    std::streambuf* rdbuf(std::streambuf* buf);
      
     ~sync_logging_buffer() { 
         //std::cout << pthread_self() << " delete sync " << filename << std::endl;
     }
private:
    void flush_impl();
    
    std::streambuf* out;
    std::vector<char> buffer;
    boost::recursive_mutex* mtx;
};

////////////////////////////////////////////////////////////////////////////////

} } } // namespace sbmt::io::detail

# endif //     SBMT__IO__SYNC_SHARED_BUFFER_HPP
