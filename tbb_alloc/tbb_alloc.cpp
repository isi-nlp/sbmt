# include <tbb/scalable_allocator.h>
# include <new>
# include <stdexcept>

namespace {

void* scalable_malloc_throw(size_t sz)
{
    void* ptr = scalable_malloc(sz);
    if (not ptr) throw std::bad_alloc();
    return ptr;
}

} // unnamed namespace

////////////////////////////////////////////////////////////////////////////////

void* operator new(size_t sz)
{
    return scalable_malloc_throw(sz);
}

////////////////////////////////////////////////////////////////////////////////

void* operator new(size_t sz, std::nothrow_t const&)
{
    return scalable_malloc(sz);
}

////////////////////////////////////////////////////////////////////////////////

void operator delete(void* ptr)
{
    scalable_free(ptr);
}

////////////////////////////////////////////////////////////////////////////////

void* operator new[](size_t sz)
{
    return scalable_malloc_throw(sz);
}

////////////////////////////////////////////////////////////////////////////////

void* operator new[](size_t sz, std::nothrow_t const&)
{
    return scalable_malloc(sz);
}

////////////////////////////////////////////////////////////////////////////////

void operator delete[](void* ptr)
{
    scalable_free(ptr);
}

void operator delete(void *ptr, std::nothrow_t const&) {
    scalable_free(ptr);
}

void operator delete[](void *ptr, std::nothrow_t const&) {
    scalable_free(ptr);
}

////////////////////////////////////////////////////////////////////////////////

