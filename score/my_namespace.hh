/* (C) Franz Josef Och; Information Science Institute / University of Southern California */
#ifndef my_namespace_h
#define my_namespace_h

#include <map>

#ifdef MY_STLPORT
 #ifdef _STLP_DEBUG
  #define MY_NAMESPACE _STL
  using MY_NAMESPACE::search;
 #else
  #define MY_NAMESPACE _STL
  using MY_NAMESPACE::search;
 #endif
#else
 #define MY_NAMESPACE std
#endif

#endif
