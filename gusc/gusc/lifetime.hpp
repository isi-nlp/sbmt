#ifndef   GUSC__LIFETIME_HPP
#define   GUSC__LIFETIME_HPP

namespace gusc {

struct create_shared {
private:
    template <class Alloc> 
    struct deleter {
        explicit deleter(Alloc& alloc,size_t n):palloc(&alloc),n(n) {}
        Alloc* palloc;
        size_t n;
        void operator()(typename Alloc::pointer p)
        {
            for (size_t x = 0; x != n; ++x) palloc->destroy(p+x);
            palloc->deallocate(p,n);
        }
    };
public:
    template <class X> struct result;
    template <class F, class Alloc>
    struct result<F(Alloc)> {
        typedef boost::shared_ptr<typename Alloc::value_type> type;
    };
    template <class Alloc>
    boost::shared_ptr<typename Alloc::value_type> operator()(Alloc& alloc, size_t n = 1) const
    {
        return boost::shared_ptr<typename Alloc::value_type>(alloc.allocate(n),deleter<Alloc>(alloc,n));
    }
};

struct create_plain {
public:
    template <class X> struct result;
    template <class F, class Alloc>
    struct result<F(Alloc)> {
        typedef typename Alloc::pointer type;
    };
    template <class Alloc>
    typename Alloc::pointer operator()(Alloc& alloc, size_t n = 1) const
    {
        return alloc.allocate(n);
    }
};

} // gusc

#endif // GUSC__LIFETIME_HPP