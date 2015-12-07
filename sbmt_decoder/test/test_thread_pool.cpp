
# include <boost/test/auto_unit_test.hpp>
# include <boost/bind.hpp>

# include <sbmt/hash/thread_pool.hpp>

using namespace boost;
using namespace sbmt;

////////////////////////////////////////////////////////////////////////////////

struct counter {
    counter():c(0){}
    
    void increase() 
    { 
        mutex::scoped_lock lk(mtx); ++c; 
    }
    
    void increase_loop(std::size_t n)
    {
        for (std::size_t x = 0; x != n; ++x) increase();
    }
    
    void increase_loop_throw(std::size_t n)
    { increase_loop(n); throw std::exception(); }
    
    std::size_t count() const 
    { 
        mutex::scoped_lock lk(mtx); 
        return c; 
    }
    
private:
    mutable mutex mtx;
    std::size_t c;
};

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE(test_thread_pool1)
{
    thread_pool pool(10);
    counter c;
    std::size_t n = 50;
    std::size_t m = 100;
    
    for (std::size_t x = 0; x != m; ++x) {
        pool.add(bind(&counter::increase_loop, &c, n));
    }
    pool.join();
    
    BOOST_CHECK_EQUAL(c.count(), n*m);
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE(test_thread_pool2)
{
    for (int t = 0; t != 10; ++t) {
    thread_pool pool(10);
    counter c;
    std::size_t n = 10;
    std::size_t m = 10;
    std::size_t p = 10;
    
    for (std::size_t x = 0; x != m; ++x) {
        for (std::size_t y = 0; y != p; ++y) {
            pool.add(bind(&counter::increase_loop, &c, n));
        }
        if (x + 1 != m) { 
            pool.wait();
        
            BOOST_CHECK_EQUAL(c.count(), (x + 1) * n * p);
        }
            
    }
    pool.join();
    
    BOOST_CHECK_EQUAL(c.count(), m * n * p);
    }
}

BOOST_AUTO_TEST_CASE(test_thread_pool3)
{
    for (int t = 0; t != 10; ++t) {
    thread_pool pool(10);
    counter c;
    std::size_t n = 10;
    std::size_t m = 10;
    std::size_t p = 10;
    
    for (std::size_t x = 0; x != m; ++x) {
        for (std::size_t y = 0; y != p; ++y) {
            pool.add(bind(&counter::increase_loop_throw, &c, n));
        }
        if (x + 1 != m) { 
            BOOST_CHECK_THROW(pool.wait(),thread_runtime_error);

            //BOOST_CHECK_EQUAL(c.count(), n);
        }
            
    }
    BOOST_CHECK_THROW(pool.join(),thread_runtime_error);
    
    //BOOST_CHECK_EQUAL(c.count(), n);
    }
}

////////////////////////////////////////////////////////////////////////////////

