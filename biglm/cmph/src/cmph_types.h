#ifndef __CMPH_TYPES_H__
#define __CMPH_TYPES_H__

#ifdef _MSC_VER
typedef unsigned __int8  cmph_uint8;
typedef unsigned __int16 cmph_uint16;
typedef unsigned cmph_uint32;
#else
#include <stdint.h>
typedef uint8_t cmph_uint8;
typedef uint16_t cmph_uint16;
typedef uint32_t cmph_uint32;
#endif

typedef float cmph_float32;

typedef enum { CMPH_HASH_DJB2, CMPH_HASH_FNV, CMPH_HASH_JENKINS, 
	       CMPH_HASH_SDBM, CMPH_HASH_COUNT } CMPH_HASH;
extern const char *cmph_hash_names[];
typedef enum { CMPH_BMZ, CMPH_BMZ8, CMPH_CHM, CMPH_BRZ, CMPH_FCH, CMPH_COUNT } CMPH_ALGO; /* included -- Fabiano */
extern const char *cmph_names[];

#endif
