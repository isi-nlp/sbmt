#ifndef   SBMT__HASH__READ_WRITE_MUTEX_HPP
#define   SBMT__HASH__READ_WRITE_MUTEX_HPP

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  multiple clients may read simultaneously
///  but write access is exclusive
///  writers are favoured over readers
///  author: Paul Bridger
///
///  note: recent versions of boost have removed their read_write_lock because
///  it had dead-lock problems, and the author disappeared from the community.
///  --michael
///
////////////////////////////////////////////////////////////////////////////////
class read_write_mutex : boost::noncopyable
{
public:
    read_write_mutex() :
        num_readers(0),
        num_pending_writers(0),
        is_current_writer(false)
    {}

    // local class has access to read_write_mutex private members, as required
    class scoped_read_lock : boost::noncopyable
    {
    public:
        scoped_read_lock(read_write_mutex& rwLock) :
            rw_mtx(rwLock)
        {
            rw_mtx.acquire_read_lock();
        }

        ~scoped_read_lock()
        {
            rw_mtx.release_read_lock();
        }

    private:
        read_write_mutex& rw_mtx;
    };

    class scoped_write_lock : boost::noncopyable
    {
    public:
        scoped_write_lock(read_write_mutex& rwLock) :
            rw_mtx(rwLock)
        {
            rw_mtx.acquire_write_lock();
        }

        ~scoped_write_lock()
        {
            rw_mtx.release_write_lock();
        }

    private:
        read_write_mutex& rw_mtx;
    };


private: // data
    boost::mutex mtx;

    unsigned int num_readers;
    boost::condition no_readers;

    unsigned int num_pending_writers;
    bool is_current_writer;
    boost::condition writer_finished;


private: // internal locking functions
    void acquire_read_lock()
    {
        boost::mutex::scoped_lock lock(mtx);

        // require a while loop here, since when the writerFinished condition is
        // notified, we should not allow readers to lock if there is a writer 
        // waiting.  if there is a writer waiting, we continue waiting
        while(num_pending_writers != 0 || is_current_writer)
        {
            writer_finished.wait(lock);
        }
        ++num_readers;
    }

    void release_read_lock()
    {
        boost::mutex::scoped_lock lock(mtx);
        --num_readers;

        if(num_readers == 0)
        {
            // must notify_all here, since if there are multiple waiting writers
            // they should all be woken (they continue to acquire the lock 
            // exclusively though)
            no_readers.notify_all();
        }
    }

    // this function is currently not exception-safe:
    // if the wait calls throw, m_pendingWriter can be left in an inconsistent 
    // state
    void acquire_write_lock()
    {
        boost::mutex::scoped_lock lock(mtx);

        // ensure subsequent readers block
        ++num_pending_writers;
        
        // ensure all reader locks are released
        while(num_readers > 0)
        {
            no_readers.wait(lock);
        }

        // only continue when the current writer has finished 
        // and another writer has not been woken first
        while(is_current_writer)
        {
            writer_finished.wait(lock);
        }
        --num_pending_writers;
        is_current_writer = true;
    }

    void release_write_lock()
    {        
        boost::mutex::scoped_lock lock(mtx);
        is_current_writer = false;
        writer_finished.notify_all();
    }
};


} // namespace sbmt

#endif // SBMT__HASH__READ_WRITE_MUTEX_HPP
