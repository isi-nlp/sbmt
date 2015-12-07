// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef LW_ATOMIC_H
#define LW_ATOMIC_H 1

#if defined(_WIN32)
#	include <windows.h>
#endif 

#if defined(__GNUC__)
 #if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2)) 
  #include <ext/atomicity.h> 
 #else 
  #include <bits/atomicity.h> 
 #endif
#endif

namespace LW {

# if defined(__GNUC__)
typedef _Atomic_word VolatileInt;
# elif defined(_WIN32)
typedef volatile long VolatileInt;
# endif




#if defined(__GNUC__)
inline int
atomicIncrement(VolatileInt& lval)
{
    __gnu_cxx::__atomic_add(&(lval),1);
    return lval;
}

inline int
atomicDecrement(VolatileInt& lval)
{
    __gnu_cxx::__atomic_add(&(lval),-1);
    return lval;
}

#elif !defined(_WIN32)
inline int
atomicIncrement(VolatileInt& lval)
{
   __asm__ __volatile__("lock ; incl %0"
      : "=m" (lval) : "m" (lval));
	return lval;
}

inline int
atomicDecrement(VolatileInt& lval)
{
    __asm__ __volatile__("lock ; decl %0"
        : "=m" (lval) : "m" (lval));
    return lval;
}
#elif defined(_WIN32)

inline int
atomicIncrement(VolatileInt& lval)
{
    return InterlockedIncrement(&lval);
}

inline int
atomicDecrement(VolatileInt& lval)
{
    return InterlockedDecrement(&lval);
}

#endif

 

 
};


#endif // LW_ATOMIC_H
