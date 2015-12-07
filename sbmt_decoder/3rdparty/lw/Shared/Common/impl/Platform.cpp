// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

/*! \file
 *  The Platform specific function implementations
 */

#include <Common/stldefs.h> // declares some functions
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#if !defined(_MSC_VER)
// to conform with new Windows vs2005 CLR security stuff under Linux...
int _vsnprintf_s(
   char *buffer,
   size_t sizeOfBuffer,
   size_t count,
   const char *format,
   va_list argptr 
) {
    return _vsnprintf(buffer, sizeOfBuffer, format, argptr);  
}

#if !defined(__MINGW32__)
char *_getcwd(
    char *buffer,
    int maxlen
) {
    return getcwd(buffer, maxlen);
}
#endif

typedef int errno_t;
errno_t _dupenv_s(
    char **buffer,
    size_t *sizeInBytes,
    const char *varname
) {
    char *val;
    val = getenv(varname);
    errno_t err = errno;
    if (val) {
        *sizeInBytes = strlen(val);
        *buffer = _strdup(val);
    }
    else {
        *buffer = NULL;
        *sizeInBytes = 0;
    }
    return err;
}

errno_t fopen_s(
   FILE** pFile,
   const char *filename,
   const char *mode 
) {
    *pFile = fopen(filename, mode);
    return errno;
}

errno_t strncpy_s(
   char *strDestination,
   size_t sizeInBytes,
   const char *strSource,
   size_t count
) {
    strncpy(strDestination, strSource, sizeInBytes);
    return errno;
}

errno_t strcpy_s(
   char *strDestination,
   size_t sizeInBytes,
   const char *strSource 
) {
    strncpy(strDestination, strSource, sizeInBytes);
    return errno;
}

int _snprintf_s(
   char *buffer,
   size_t sizeOfBuffer,
   size_t count,
   const char *format,
   ...
) {
	va_list argp;
	va_start(argp, format);
	int rc =_vsnprintf_s(buffer, sizeOfBuffer, count, format, argp);
	va_end (argp);
    return rc;
}
#endif


int 
lw_snprintf(char *buffer, size_t count, const char *format, ...)
{
    va_list ap;
	if (!buffer)
		return -1;
	if (!format) {
		buffer[0] = 0;
		return -1;
	}
    va_start(ap, format);
	int ret;
#ifdef _MSC_VER
	ret = _vsnprintf_s(buffer, count, count-1, format, ap);
	if (ret < 0) {
		buffer[count-1] = 0;
		ret = static_cast<int>(count);
	}
#else
	ret = vsnprintf(buffer, count, format, ap);
#endif
    va_end(ap);
	return ret;
}

