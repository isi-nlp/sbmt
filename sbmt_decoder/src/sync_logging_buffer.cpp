# include <sbmt/io/detail/sync_logging_buffer.hpp>
# include <iostream>

namespace sbmt { namespace io { namespace detail {

////////////////////////////////////////////////////////////////////////////////

sync_logging_buffer::sync_logging_buffer( std::streambuf* out
                                        , boost::recursive_mutex* mtx
                                        , std::size_t buffer_bytes )
  : out(out)
  , mtx(mtx)
{
      if (mtx) buffer.reserve(2 * buffer_bytes);
}
////////////////////////////////////////////////////////////////////////////////

std::streamsize 
sync_logging_buffer::write(char const* s, std::streamsize in)
{

    //std::cout << "* sync::write() called" << std::endl;
    buffer.insert(buffer.end(),s, s + in);
    return in;

}

////////////////////////////////////////////////////////////////////////////////

void sync_logging_buffer::close()
{
    //std::cout << std::endl;
    //std::cout << pthread_self() << "******************************************" << std::endl;
    //std::cout << pthread_self() << "* sync::close() called on "<< filename << (void*)this  << std::endl;
    //std::cout << pthread_self() << "******************************************" << std::endl;
    if (buffer.size() > 0) {
        flush_impl();
    } 
    //std::cout << pthread_self() << "******************************************"   << std::endl;
    //std::cout << pthread_self() << "* sync::close() finished on "<< filename << (void*)this  << std::endl;
    //std::cout << pthread_self() << "******************************************"   << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

bool sync_logging_buffer::flush()
{
    if (buffer.size() > 0) {
        if (buffer[buffer.size() - 1] != '\n') buffer.push_back('\n');
        flush_impl();
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

std::streambuf* sync_logging_buffer::rdbuf()
{
    return out;
}

////////////////////////////////////////////////////////////////////////////////

std::streambuf* sync_logging_buffer::rdbuf(std::streambuf* buf)
{
    flush_impl();
    out = buf;
    return out;
}

////////////////////////////////////////////////////////////////////////////////

static void write_and_flush(std::streambuf* out, std::vector<char>& buffer)
{
    std::streamsize 
		s = buffer.size() > 0 
		  ? boost::iostreams::write(*out,&buffer[0],buffer.size())
		  : 0
		  ;
    //the rest of boost::iostreams does not handle non-blocking
    //but if/when it does, we need to handle this assert properly
    assert(s == (std::streamsize)buffer.size());
    s=0; //avoids unreferenced warning in release code...
    buffer.resize(0);
    //out->pubsync();
}

void sync_logging_buffer::flush_impl()
{
    //std::cout << std::endl;
    //std::cout << pthread_self() << "*****************************************"       << std::endl;
    //std::cout << pthread_self() << "* sync::flush_impl() called on "<< filename << (void*)this  << std::endl;
    //std::cout << pthread_self() << "*****************************************"       << std::endl;
    if (mtx) {
        boost::recursive_mutex::scoped_lock lk(*mtx);
        write_and_flush(out,buffer);
    } else {
        write_and_flush(out,buffer);
    }
    //std::cout << pthread_self() << "*****************************************"         << std::endl;
    //std::cout << pthread_self() << "* sync::flush_impl() finished on "<< filename << (void*)this  << std::endl;
    //std::cout << pthread_self() << "*****************************************"         << std::endl;
}



////////////////////////////////////////////////////////////////////////////////

} } }// namespace sbmt::io::detail
