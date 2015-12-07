# if !defined(SBMT__HASH__THREAD_POOL_HPP)
# define SBMT__HASH__THREAD_POOL_HPP
# include <boost/function.hpp>
# include <boost/thread/thread.hpp>
# include <boost/thread/condition.hpp>
# include <boost/thread/mutex.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/scoped_ptr.hpp>

# include <sbmt/hash/cloneable.hpp>

# include <stdexcept>
# include <cstddef>
# include <limits>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  difference between this thread and boost::thread is that
///  1) exceptions raised by thread-main are propegated through join(), or
///     throw_if_error()
///  2) threads are joined at destruction time if not done at join (RAII)
///
///  about exceptions and threads:
///  exceptions work by propagation along the function stack.  threads represent
///  the end of the function stack.  so any propagation across threads is a bit
///  unnatural.  however, since any thread is guaranteed to be dead after a
///  call to mythread.join(), this is as natural a place as any to rethrow an
///  exception that was not caught within a thread.
///
///  getting the right exception thrown:
///  to get the right type of exception thrown, you should derive your exception
///  class from sbmt::cloneable. if you dont, your exception will be translated
///  to a thread_runtime_error exception.
///  internally, all the standard exceptions defined in <stdexcept> will be
///  properly forwarded with the correct type.
///  \todo implement user-defined mechanism for watching for the right
///  exception and throwing the correct type, without having to derive from
///  cloneable.
///
////////////////////////////////////////////////////////////////////////////////
class thread : private boost::noncopyable
{
public:
    explicit thread(boost::function<void(void)> const&);
    thread();

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// determine if two threads instances represent the same thread resource
    ///
    ////////////////////////////////////////////////////////////////////////////
    //\{
    bool operator==(thread const&) const;
    bool operator!=(thread const&) const;
    //\}

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  calling join() blocks execution until the thread is finished.  dont
    ///  call it more than once.  your thread is gone once you return from join.
    ///  if an exception escapes your thread, it will be propogated when join
    ///  is called.
    ///
    ////////////////////////////////////////////////////////////////////////////
    void join();

    /// non-throwing version of join()
    void join_nothrow();

    /// will call join if not called before destruction
    ~thread();
private:
    void throw_if_error();
    void run(boost::function<void(void)> const&);
    boost::scoped_ptr<boost::thread> thrd;
    bool joined;
    boost::shared_ptr<cloneable> err;
};

class thread_runtime_error
: public std::runtime_error
, public virtual cloneable
{
public:
    thread_runtime_error(std::string const& msg)
    : std::runtime_error(msg) {}

    virtual void throw_self() const;
    virtual cloneable* clone_self() const;
    virtual ~thread_runtime_error() throw() {}
};

////////////////////////////////////////////////////////////////////////////////
///
///  allows you to dispatch a number of worker function threads, while limiting
///  the amount of simultaneous work that is done.  if you have a lot of jobs
///  that complete very quickly, a pool can be more efficient than just starting
///  and stopping a thread for each request.  also, on hpc where sentinal
///  programs watch your programs for load spikes, limiting the number of
///  threads to the processors available is courteous.
///
///  exceptions and threadpools, threadgroups, etc.
///  i cant think of any way to decide which exception to throw if multiple
///  threads in a group or pool throw (which is very common if you start
///  running out of memory).  so i just throw the last one i saw before join()
///  or wait() is called.
///
///  also, if one job throws, all remaining jobs are cancelled.  mainly this
///  decision was made because in mini_decoder, the main exception that gets
///  through is bad_alloc(out-of-memory).  if that happens, chances are good
///  the rest of your jobs are gonna throw, too.  regardless, typical behaviour
///  of an uncaught exception is to abort a program, so seems okay behaviour
///  to kill all jobs in the pool.  if you dont want this to happen, catch
///  the exceptions you can recover from inside your jobs routine.
///
///  \todo implement some sort of user-defined exception handler for getting
///  all exceptions from pool.  java has had a few iterations of of propagation
///  behavior, and might have a good interface.
///
///  thread-safety: calling add, wait, or join concurrently can result in a race
///  condition, resulting in work that is not processed, or processed after you
///  think all work is done.  typical behaviour has one thread (often main thread)
///  with access to thread_pool, and no access for the children threads, so this
///  is not a problem.  otherwise, you need to protect the calls yourself.
///
////////////////////////////////////////////////////////////////////////////////
class thread_pool
{

public:
   ////////////////////////////////////////////////////////////////////////////
   ///
   ///  the type of function or functor to submit as work to the thread pool
   ///  should have signature void(void).  Remember, because of boost::bind,
   ///  this is not a limitation at all.  you can bind any number of arguments
   ///  to any function or functor to create a void (void) functor.
   ///
   ////////////////////////////////////////////////////////////////////////////
   typedef boost::function<void(void)> function_t;

   ////////////////////////////////////////////////////////////////////////////
   ///
   ///  creates num_threads threads that jobs can be submitted to.
   ///  \param max_jobs referrs to the max pending jobs that can be in the
   ///  pool.  if more than max_jobs are waiting, then add() will block until
   ///  some jobs have completed.
   ///
   ////////////////////////////////////////////////////////////////////////////
   explicit thread_pool( size_t num_threads
                       , size_t max_jobs = std::numeric_limits<size_t>::max() );

   ////////////////////////////////////////////////////////////////////////////
   ///
   ///  add a job ( a functor of void(void) signature ) to be
   ///  processed by the pool's threads
   ///
   ////////////////////////////////////////////////////////////////////////////
   void add(function_t const& func);

   ////////////////////////////////////////////////////////////////////////////
   ///
   ///  blocks until all submitted work is finished, but does not terminate the
   ///  threads in the pool.
   ///  if any work caused an exception, then an exception will be thrown.
   ///
   ////////////////////////////////////////////////////////////////////////////
   void wait();

   ////////////////////////////////////////////////////////////////////////////
   ///
   ///  blocks until all submitted work is finished, and returns all threads to
   ///  to the calling thread.
   ///  if any work caused an exception to escape, then an exception will be
   ///  thrown.
   ///
   ////////////////////////////////////////////////////////////////////////////
   void join();

   /// does not propagate exceptions.  threads that throw just go away
   //  silently.  all exceptions thrown are caught internally, however.
   void join_nothrow();

   ~thread_pool();

private:
   typedef boost::mutex mutex_t;
   void work();
   void throw_if_error();

   boost::shared_ptr<cloneable> err;
   mutex_t mtx;
   std::size_t worker_count;
   bool join_requested;
   std::size_t max_jobs;
   std::size_t pending_size; // list has O(n) size() function

   boost::condition pending_or_join_requested;
   boost::condition work_finished;
   boost::condition pending_space_available;

   std::list< boost::shared_ptr<function_t> > pending;
   std::list<boost::shared_ptr<thread> > pool;
};

} // namespace sbmt


# endif // SBMT__HASH__THREAD_POOL_HPP
