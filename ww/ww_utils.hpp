#ifndef __ww_utils_hpp__
#define __ww_utils_hpp__ 
#include <string> 

#if defined(_STLPORT_VERSION)
#	include <hash_map>
#include <hash_set>

#	ifndef stlext_ns_alias_defined
#	define stlext_ns_alias_defined

namespace stlext = ::std;
namespace stlextp = ::std;


#	endif

namespace std{

  template<>  struct hash<std::string> {
    size_t operator()(const std::string& str) const{
      const char* str1 = str.c_str();
      hash<const char*> ha;
      return ha(str1);
    }
  };
}

#else

#	include <ext/hash_map>
#include <ext/hash_set>

#	ifndef stlext_ns_alias_defined
#		define stlext_ns_alias_defined
namespace stlext = ::__gnu_cxx;
namespace stlextp = ::std;
#	endif

namespace __gnu_cxx{

  template<>  struct hash<std::string> {
    size_t operator()(const std::string& str) const{
      const char* str1 = str.c_str();
      hash<const char*> ha;
      return ha(str1);
    }
  };
}


#endif


namespace ww_util {

struct noout {
    template <class X> void operator()(X const& x) const { return; }
};


}

#endif


