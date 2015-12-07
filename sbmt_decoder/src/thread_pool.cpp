# include <sbmt/hash/thread_pool.hpp>
# include <boost/bind.hpp>

#ifdef _WIN32
#include <iso646.h>
#endif

# include <boost/thread/once.hpp>
#include <iostream>

using boost::bind;
using boost::function;
using boost::shared_ptr;
using boost::call_once;
using boost::ref;
using std::list;
using std::auto_ptr;

namespace sbmt {
    
////////////////////////////////////////////////////////////////////////////////

void thread_runtime_error::throw_self() const
{
    thread_runtime_error err(*this);
    throw err;
}

cloneable* thread_runtime_error::clone_self() const
{
    return new thread_runtime_error(*this);
}
    
////////////////////////////////////////////////////////////////////////////////

template <class ExceptionT>
class exception_wrap : public ExceptionT, public virtual cloneable
{
public:
    exception_wrap(const char* msg)
    : ExceptionT(msg) {}
    
    exception_wrap(){}
    
    virtual cloneable* clone_self() const { return new exception_wrap(*this); }
    virtual void throw_self() const { throw ExceptionT(*this); }
};

////////////////////////////////////////////////////////////////////////////////

shared_ptr<cloneable> err_bad_alloc;
shared_ptr<cloneable> err_bad_cast;
shared_ptr<cloneable> err_bad_exception;
shared_ptr<cloneable> err_bad_typeid;
shared_ptr<cloneable> err_thread_runtime;

////////////////////////////////////////////////////////////////////////////////

void init_exceptions()
{
    err_bad_alloc.reset(new exception_wrap<std::bad_alloc>());
    err_bad_cast.reset(new exception_wrap<std::bad_cast>());
    err_bad_exception.reset(new exception_wrap<std::bad_exception>());
    err_bad_typeid.reset(new exception_wrap<std::bad_typeid>());
    err_thread_runtime.reset(
        new thread_runtime_error("uncaught exception in thread")
    );
}

boost::once_flag init_exceptions_flag = BOOST_ONCE_INIT;

////////////////////////////////////////////////////////////////////////////////

# define SBMT_CATCH_STD_EXCEPTION(Type) \
catch (Type const& e) { err.reset(new exception_wrap<Type>(e.what())); } 

void try_catch(function<void(void)> const& f, shared_ptr<cloneable>& err)
{
    using namespace std;
    // the outer tryblock is in case the unthinkable happens:
    // one of the exceptions we clone throws an exception on creation!
    try { 
        try {
            f();
        } 
        // inherrits from cloneable, can perfectly clone exception
        catch(cloneable const& e) { err.reset(e.clone_self()); } 
        // no-string-argument exceptions; avoid cloning by passing along 
        // a pre-built copy.
        
        // listed reverse order of inherritence tree to throw most
        // specific exception possible
        catch(bad_alloc const&) { err = err_bad_alloc; } 
        catch(bad_cast const&) { err = err_bad_cast; } 
        catch(bad_exception const&) { err = err_bad_exception; } 
        catch(bad_typeid const&) { err = err_bad_typeid; }
        
        // these are the exceptions taking string argument. clone the string 
        // argument.  
        SBMT_CATCH_STD_EXCEPTION(ios_base::failure)
        SBMT_CATCH_STD_EXCEPTION(domain_error)
        SBMT_CATCH_STD_EXCEPTION(invalid_argument)
        SBMT_CATCH_STD_EXCEPTION(length_error)
        SBMT_CATCH_STD_EXCEPTION(out_of_range)
        SBMT_CATCH_STD_EXCEPTION(overflow_error)
        SBMT_CATCH_STD_EXCEPTION(underflow_error)
        SBMT_CATCH_STD_EXCEPTION(range_error)
        SBMT_CATCH_STD_EXCEPTION(logic_error)
        SBMT_CATCH_STD_EXCEPTION(runtime_error)
        catch(exception const& e) { err.reset(new thread_runtime_error(e.what())); }
        catch(...) {
            err = err_thread_runtime;
        }
    }
    // in case our cloning goes bad (causing a bad_alloc or some other such error)
    catch(bad_alloc const&) { err = err_bad_alloc; }
    catch(...) { err = err_thread_runtime; }
}

////////////////////////////////////////////////////////////////////////////////

thread::thread()
: joined(true)
{
    call_once(init_exceptions, init_exceptions_flag);
    throw std::runtime_error("gotcha fool");
}

thread::thread(function<void(void)> const& f)
: joined(true)
{
    call_once(init_exceptions, init_exceptions_flag);
    auto_ptr<boost::thread> p(new boost::thread(bind(try_catch,f,ref(err))));
    thrd.reset(p.release());
    joined = false;
}

////////////////////////////////////////////////////////////////////////////////

void thread::join_nothrow() 
{ 
    joined = true;
    thrd->join(); 
    //std::cerr << "thread joined" << std::endl;
}

void thread::join()
{
    join_nothrow();
    throw_if_error();
}

void thread::throw_if_error()
{
    if (err) err->throw_self();
}

thread::~thread()
{
    if (!joined) join_nothrow();
    //std::cerr << "thread destroyed" << std::endl;
}
   
////////////////////////////////////////////////////////////////////////////////

void thread_pool::work()
{

    {
        mutex_t::scoped_lock lk(mtx);
        ++worker_count;
    }
    bool finished = false;
    shared_ptr<function_t> f;
    shared_ptr<cloneable> e;
    while (!finished) {
       {
           mutex_t::scoped_lock lk(mtx);
           while (!join_requested and pending.empty()) {
               --worker_count;
               if (worker_count == 0)
                   work_finished.notify_all();
               pending_or_join_requested.wait(lk);
               ++worker_count;
           }
           if(pending.empty()) {
               finished = join_requested;
               continue;
           } else if (err) {
               pending.clear();
               pending_size = 0;
               finished = join_requested;
               continue; // dont keep running jobs if uncaught exceptions occur
           } else {
               f = pending.front();
               pending.pop_front();
               --pending_size;
               if (pending_size < max_jobs) {
                   pending_space_available.notify_all();
               }
           }
       }
       if (*f) try_catch(*f,e);
       {
           mutex_t::scoped_lock lk(mtx);
           if (e and !err) err = e;
       }
    }
    {
       mutex_t::scoped_lock lk(mtx);
       --worker_count;      
    }

}
       
////////////////////////////////////////////////////////////////////////////////

void thread_pool::join_nothrow()
{
    {
        mutex_t::scoped_lock lk(mtx);
        join_requested = true;
        pending_or_join_requested.notify_all();
    }
    list< shared_ptr<thread> >::iterator itr = pool.begin(),
                                         end = pool.end();
    for (;itr != end; ++itr) {
        //std::cerr << "begin join" << std::endl;
        (*itr)->join_nothrow();
        //std::cerr << "end join" << std::endl;
    }
    //std::cerr << "thread_pool joined" << std::endl;
}

void thread_pool::throw_if_error()
{
    if (err) {
        shared_ptr<cloneable> err2(err);
        err.reset();
        err2->throw_self();
    }
}

void thread_pool::join()
{
    join_nothrow();
    throw_if_error();
}

void thread_pool::wait()
{
    mutex_t::scoped_lock lk(mtx);
    while (worker_count > 0 or not pending.empty()) {
        work_finished.wait(lk);
    }
    throw_if_error();
}

thread_pool::~thread_pool()
{
    //mutex_t::scoped_lock lk(mtx);
    //join_requested = true;
    //pending_or_join_requested.notify_all();
    if (not join_requested) join_nothrow();
    //std::cerr << "thread_pool destroyed" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

thread_pool::thread_pool( std::size_t max_running 
                        , std::size_t max_jobs )
:  worker_count(0)
, join_requested(false)
, max_jobs(max_jobs)
, pending_size(0)
{                   
    if (max_running <= 0) {
        std::invalid_argument e(
            "thread_pool::thread_pool(num_threads): "
            "num_threads must be greater than 0"
        );
        throw e;
    }
    if (max_jobs <= 0) {
        std::invalid_argument e(
            "thread_pool::thread_pool(num_threads,max_jobs): "
            "requires 0 < max_jobs"
        );
        throw e;
    }
    try {
        for (std::size_t x = 0; x != max_running; ++x) {
            shared_ptr<thread> pt;
            pt.reset(new thread(bind(&thread_pool::work,this)));
            pool.push_back(pt);
        }
    } catch(...) {
        join_nothrow();
        throw;
    }
}

////////////////////////////////////////////////////////////////////////////////

void thread_pool::add(function_t const& func)
{
    mutex_t::scoped_lock lk(mtx);
    if (err) return;
    while (pending_size >= max_jobs) {
        pending_space_available.wait(lk);
    }
   
    shared_ptr<function_t> pf(new function_t(func));
    pending.push_back(pf);
    ++pending_size;
    pending_or_join_requested.notify_all();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

