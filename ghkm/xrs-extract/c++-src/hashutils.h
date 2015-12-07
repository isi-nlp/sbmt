#ifndef __HASHUTILS_H__
#define __HASHUTILS_H__

#if (__GNUC__ >= 4)
#include <ext/hash_map>
#include <ext/hash_set>
#define __GNU_CXX_NAMESPACE
#else

#if (__GNUC__ == 3)
#include <ext/hash_map>
#include <ext/hash_set>
#if (__GNUC_MINOR__ >= 1)
#define __GNU_CXX_NAMESPACE
#else
#define STD_NAMESPACE
#endif
#else
#include <hash_map>
#include <hash_set>
#define STD_NAMESPACE
#endif

#endif

#ifdef __GNU_CXX_NAMESPACE
using __gnu_cxx::hash;
using __gnu_cxx::hash_set;
using __gnu_cxx::hash_map;
using __gnu_cxx::hash_multimap;
#else
#ifdef STD_NAMESPACE
using std::hash;
using std::hash_set;
using std::hash_map;
using std::hash_multimap;
#endif
#endif

/*******************************************************************
 * Hashing "string"'s using the "char*" function defined in the 
 * SGI implementation of the STL:
 *******************************************************************/
struct hash_str {
  size_t operator()(const std::string& s) const {
    return hash<char const *>()(s.c_str());
  }
};

/*******************************************************************
 To hash integers:
 *******************************************************************/

//#define HASH_INT_HSIZE 128
// number of buckets: 193

// Integers:
struct hash_int {
  size_t operator()(int v) const {
    return hash<int>()(v);
  }
};

// Hashing pointers (memory addresses) the same way as
// hashing integers:
struct hash_ptr {
  size_t operator()(void* v) const {
    return hash<long>()((long)v);
  }
};

#define HASH_INT_HSIZE 128
 
/*******************************************************************
 Typedefs for some basic kinds of hashmaps
 *******************************************************************/

typedef hash_map<const std::string, std::string, hash_str> StrHash;
typedef StrHash::iterator StrHash_it;
typedef StrHash::const_iterator StrHash_cit;
typedef hash_multimap<const std::string, std::string, hash_str> StrHashM;
typedef StrHashM::iterator StrHashM_it;
typedef StrHashM::const_iterator StrHashM_cit;
typedef hash_map<const std::string, double, hash_str> DoubleHash;
typedef DoubleHash::iterator DoubleHash_it;
typedef DoubleHash::const_iterator DoubleHash_cit;
typedef hash_multimap<const std::string, double, hash_str> DoubleHashM;
typedef DoubleHashM::iterator DoubleHashM_it;
typedef DoubleHashM::const_iterator DoubleHashM_cit;
typedef hash_map<const std::string, int, hash_str> IntHash;
typedef IntHash::iterator IntHash_it;
typedef IntHash::const_iterator IntHash_cit;
typedef hash_multimap<const std::string, int, hash_str> IntHashM;
typedef IntHashM::iterator IntHashM_it;
typedef IntHashM::const_iterator IntHashM_cit;

#endif
