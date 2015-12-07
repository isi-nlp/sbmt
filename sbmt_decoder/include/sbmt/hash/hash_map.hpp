/**
 wrappers for STL hash_map
 */
#ifndef SBMT_HASH_MAP_H
#define SBMT_HASH_MAP_H 1

#include <utility> // this is just to get stlport macros in

#if defined(_STLPORT_VERSION) || defined(__SGI_STL_PORT)
#	include <hash_map>
#	ifndef stlext_ns_alias_defined
#       define stlext_ns_alias_defined
        namespace stlext = ::std;
        namespace stlextp = ::std;
#	endif


#else

#   include <ext/hash_map>

#   ifndef stlext_ns_alias_defined
#       define stlext_ns_alias_defined
        namespace stlext = ::__gnu_cxx;
        namespace stlextp = ::std;
#   endif


#endif

template<class Map, class Key, class Value>
stlextp::pair<typename Map::iterator, bool>
insert_into_map(Map& map, const Key& key, const Value& value)
{
	return map.insert(stlextp::make_pair(key, value));
}

#undef pair_ns


#endif // SBMT_HASH_MAP_H
