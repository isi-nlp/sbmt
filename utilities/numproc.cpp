#include "numproc.hpp"
#include <boost/static_assert.hpp>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/param.h>
    #include <sys/sysctl.h>
    #define  NUMPROC_USE_NCPU
#else
    #include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////////////

std::size_t numproc_online()
{
    std::size_t np = 1;
#if defined(NUMPROC_USE_NCPU)
    int mib[2];
    std::size_t len = sizeof(np);
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    sysctl(mib, 2, &np, &len, NULL, 0);
#elif defined(_SC_NPROCESSORS_ONLN)
    np = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROC_ONLN)
    np = sysconf(_SC_NPROC_ONLN);
#elif defined _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    np = info.dwNumberOfProcessors;
#else
    // can't compile numproc_online on this system
    BOOST_STATIC_ASSERT(0);
#endif
    if (np <= 0) np = 1;
    return np;
}

////////////////////////////////////////////////////////////////////////////////
