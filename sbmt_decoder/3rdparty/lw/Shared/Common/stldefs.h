// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifdef WIN32 
#pragma message("\n\n*********** WIN32 defined **************\n\n")
#else
#endif
#ifdef _WIN32 
#pragma message("\n\n********** _WIN32 defined **************\n\n")
#else
#endif

#ifndef _STLDEFS_H
#define _STLDEFS_H
#if defined(_MSC_VER) && !defined(LM_NO_COMMON_LIB)
#include <Common/yvals.h>
#undef _SECURE_SCL
#undef _HAS_ITERATOR_DEBUGGING
#endif
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <list>
#include <stack>
#include <map>
#ifndef _MSC_VER
#   include <ext/hash_map>
#else
#   include <hash_map>
#endif
#include <set>
#include <iostream>
#include <iterator>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif
#include "Common/ErrorCode.h"

#if defined(_MSC_VER)
#pragma warning(disable:4786)
#endif

using namespace std;
//#if defined(WIN32)
//using stdext::hash_map;
//using stdext::hash_compare;
//#endif
using LW::ErrorCode;
using LW::Exception;

// ============================================================================
// Compiler Silencing macros
// Some compilers complain about parameters that are not used.  This macro
// should keep them quiet.
// ============================================================================
#if defined (ghs) || defined (__GNUC__) || defined (__hpux) || defined (__sgi) || defined (__DECCXX) || defined (__KCC) || defined (__rational__) || defined (__USLC__) || defined (ACE_RM544)
// Some compilers complain about "statement with no effect" with (a).
// This eliminates the warnings, and no code is generated for the null
// conditional statement.  NOTE: that may only be true if -O is enabled,
// such as with GreenHills (ghs) 1.8.8.
# define LW_UNUSED_ARG(a) do {/* null */} while (&a == 0)
#else /* ghs || __GNUC__ || ..... */
# define LW_UNUSED_ARG(a) (a)
#endif /* ghs || __GNUC__ || ..... */

#if defined (__sgi) || defined (ghs) || defined (__DECCXX) || defined(__BORLANDC__) || defined (__KCC) || defined (ACE_RM544) || defined (__USLC__)
# define LW_NOTREACHED(a)
#else  /* __sgi || ghs || ..... */
# define LW_NOTREACHED(a) a
#endif /* __sgi || ghs || ..... */

#if defined(_MSC_VER) && defined(_DEBUG)
#define _DumpLeaks(_MSG) {cout << "---" << _MSG << "---" << endl; _CrtDumpMemoryLeaks( );}
#else
#define _DumpLeaks(_MSG) {void 0}
#endif

#define C_BUFFERSIZE	4096
#define C_NOERR			0
#define C_ERR			1
#define C_FILE_ERR		2
#define C_NEGNOVAL		-1
#define C_LOWERLIM		1e-25
#define C_SEPARATOR		"|||"
#define C_VERBOSE	"verbose"
#define C_HELP	"help"

#if !defined(_MSC_VER)	
#include <errno.h>
#define _vsnprintf vsnprintf
#define _strdup strdup
#define strtok_s strtok_r
#define sprintf_s snprintf
#ifdef __min
#undef __min
#endif // __min
#ifdef __max
#undef __max
#endif // __max
#define __min min
#define __max max

// to conform with new Windows vs2005 CLR security stuff under Linux...
int _vsnprintf_s(
   char *buffer,
   size_t sizeOfBuffer,
   size_t count,
   const char *format,
   va_list argptr 
);

char *_getcwd(
    char *buffer,
    int maxlen
); 

typedef int errno_t;
errno_t _dupenv_s(
    char **buffer,
    size_t *sizeInBytes,
    const char *varname
);

errno_t fopen_s(
   FILE** pFile,
   const char *filename,
   const char *mode 
);

errno_t strncpy_s(
   char *strDestination,
   size_t sizeInBytes,
   const char *strSource,
   size_t count
);

errno_t strcpy_s(
   char *strDestination,
   size_t sizeInBytes,
   const char *strSource 
);

int _snprintf_s(
   char *buffer,
   size_t sizeOfBuffer,
   size_t count,
   const char *format,
   ...
);

#endif // WIN32

#ifdef __MINGW32__
#undef strtok_s
_CRTIMP char* __cdecl __MINGW_NOTHROW strtok_s(char*, const char*, char**);
#endif

//helper member variable access functions
#ifndef CONSTCONFMEMBERVARPTR
#define CONSTCONFMEMBERVARPTR(TYP,VARNAME) private: TYP *m_p ## VARNAME; public: const TYP &VARNAME () const {return *m_p ## VARNAME;} const bool Valid ## VARNAME () const {return (bool) m_p ## VARNAME;};
#define CONFMEMBERVAR(TYP,VARNAME) private: TYP m_ ## VARNAME; public: TYP &VARNAME () {return m_ ## VARNAME;} const TYP &VARNAME () const {return m_ ## VARNAME;}
#define CONSTCONFMEMBERVAR(TYP,VARNAME) private: TYP m_ ## VARNAME; public: const TYP &VARNAME () const {return m_ ## VARNAME;}
#endif

//handle wierd io problems
#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) {_IS.getline(_LINE, _SIZE, _DELIM); \
			if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear();\
			}

#define SAFE_READ(is, buffer, count) \
	if((is).fail() && !(is).bad() && !(is).eof()) {(is).clear();} \
	(is).read(buffer, count); 


#define ITWHILE(_X, _Y)	_X = _Y.begin(); while(_X != _Y.end())
#define ITFOR(_X, _Y)	for(_X = _Y.begin(); _X != _Y.end(); _X++)
#define ERASEVECT(_X) _X.clear()
#define ERASESTACK(_X) 	while(!_X.empty()) _X.pop();
#define COPYVECT(_SRC, _DST) copy(_SRC.begin(), _SRC.end(), back_inserter(_DST))
#define SORTVECT(_SRC) sort(_SRC.begin(), _SRC.end())

extern int g_iFeedBackLevel;

enum eFEEDBACKLEVEL {FBL_SILENT, FBL_STD, FBL_HI, FBL_DBG, FBL_DBG2, FBL_DBG3};

#define MAXFEEDBACK 5
#define	FEEDBACK(_d)		if(g_iFeedBackLevel >= _d)


inline void ERROUT(int err, const char *format_str, ...)
{
	char OutBuf[2048];
	// Start of variable args section.
	va_list argp;
	va_start(argp, format_str);
	int rc =_vsnprintf_s(OutBuf, 2047, 2047, format_str, argp);
	if (rc < 0) 
		OutBuf[2047] = 0;
	va_end (argp);
	cerr << OutBuf << endl;
	Exception e(err, OutBuf);
	throw e;
}

inline void ERROUT(const char *format_str, ...)
{
	char OutBuf[2048];
	// Start of variable args section.
	va_list argp;
	va_start(argp, format_str);
	int rc = _vsnprintf_s(OutBuf, 2047, 2047, format_str, argp);
	if (rc < 0) 
		OutBuf[2047] = 0;
	va_end (argp);
	cerr << OutBuf << endl;
	Exception e(LW::SVT_FATAL, OutBuf);
	throw e;
}

inline void WARNINGOUT(ostream &os, const char *format_str, ...)
{
	char OutBuf[2048];
	// Start of variable args section.
	va_list argp;
	va_start (argp, format_str);	
	int rc = _vsnprintf_s(OutBuf, 2047, 2047, format_str, argp);
	if (rc < 0) 
		OutBuf[2047] = 0;
	va_end (argp);
	os << "!Warning:" << OutBuf << "!" << endl;
}

/*#ifdef _DEBUG 
#define DEBUGOUT(_EXPR) {cerr << _EXPR << endl; cout << _EXPR << endl;} 
#define DEBUGASSERT(_EXPR) assert(_EXPR)
#else*/
#define DEBUGOUT(_EXPR) ((void)0)
#define DEBUGASSERT(_EXPR) ((void)0)
//#endif

typedef stack<string>   STRSTACK;
typedef vector<string>	STRVECT;
typedef vector<STRVECT>	STRVECTVECT;
typedef vector<int>		INTVECT;
typedef vector<INTVECT>	INTVECTVECT;
typedef vector<INTVECTVECT>	INTVECTVECTVECT;
typedef vector<unsigned int>	UINTVECT;
typedef vector<UINTVECT>	UINTVECTVECT;
typedef vector<UINTVECTVECT>	UINTVECTVECTVECT;
typedef vector<size_t>	SZTVECT;
typedef vector<SZTVECT>	SZTVECTVECT;
typedef vector<SZTVECTVECT>	SZTVECTVECTVECT;
typedef vector<float>	FLOATVECT;
typedef vector<bool>	BOOLVECT;
typedef vector<double>	DOUBLEVECT;
typedef set<string, less<string> > STRSET;
typedef set<unsigned int> UINTSET;
typedef set<size_t> SZTSET;
typedef set<int> INTSET;
typedef vector<UINTVECT*> VECTPUINTVECT;
typedef vector<SZTVECT*> VECTPSZTVECT;

typedef map<string, unsigned int, less<string> > STR2UINTMAP;
typedef map<string, size_t, less<string> > STR2SZTMAP;
typedef map<string, STRVECT, less<string> > STR2STRVECTMAP;

template<class T,class T2>
inline ostream&operator<<(ostream&out,const pair<T,T2>&x)
{
	return out<<x.first<<","<<x.second<<" ";
}

template<class T>
inline ostream&operator<<(ostream&out,const vector<T>&x)
{
	for(size_t i=0;i<x.size();++i)
		out << x[i] << " ";
	return out;
}

template<class A,class B>
inline ostream&operator<<(ostream&out,const map<A,B>&x)
{
	for(typename map<A,B>::const_iterator i=x.begin();i!=x.end();++i)
		out << i->first << ":" << i->second << ' ';
	return out;
}

template<class T>
inline ostream&operator<<(ostream&out,const list<T>&x)
{
	for(typename list<T>::const_iterator i=x.begin();i!=x.end();++i)
		out << *i << " ";
	return out;
}

/*template<class T> 
inline bool operator<(const vector<T> &x, const vector<T> &y)
{
	if( &x == &y )
		return 0;
	else
	{
		if( y.size()<x.size() )
			return !(y<x);
		for(size_t iii=0;iii<x.size();iii++)
		{
			if( x[iii]<y[iii] )
				return 1;
			else if( y[iii]<x[iii] )
				return 0;
		}
		return x.size()!=y.size();//??
	}
}*/

template<class T> 
inline bool operator==(const vector<T>&x,const vector<T>&y)
{
	if( x.size()!=y.size())
		return 0;
	for(size_t i=0;i<x.size();++i)
		if(x[i]!=y[i])
			return 0;
	return 1;
}

/**Dump to stream str vect*/
inline void DumpStrVect(ostream& out, STRVECT& strvect){
	STRVECT::iterator it;
	
	ITWHILE(it, strvect){
		out << it->c_str() << " ";
		it++;
	}
}
/**Dump to stream str vect with new line*/
inline void DumpStrVectNL(ostream& out, STRVECT& strvect){
	STRVECT::iterator it;
	
	ITWHILE(it, strvect){
		out << it->c_str() << endl;
		it++;
	}
}
/**Dump to stream string vect vect*/
inline void DumpStrVectVect(ostream& out, STRVECTVECT& strvv){
	STRVECTVECT::iterator it;
	
	ITWHILE(it, strvv){
		DumpStrVect(out, *it);
		out << endl;
		it++;
	}
}

/**Dump to stream int vect*/
template<class T>
inline ostream& DumpV(ostream& out, const vector<T>& x){
	for(size_t i = 0; i < x.size(); i++){
		out << x[i] << " ";
	}
	return out;
}
/**Dump to stream int vect vect*/
template<class T>
inline ostream& DumpV(ostream& out, const vector < const vector<T> >& x){
	for(size_t i = 0; i < x.size(); i++){
		out << "{";
		Dump(out, x[i]);
		out << "} ";
	}
	return out;
}
/**Dump to stream unsigned int vect vect vect*/
template<class T>
inline ostream& DumpV(ostream& out, const vector< const vector< const vector<T> > >& x){
	for(size_t i = 0; i < x.size(); i++){
		out << "{";
		Dump(out, x[i]);
		out << "} ";
	}
	return out;
}

/**Range compare of a UIntVect to a number, returning true if the number is in the list.*/
template<class T>
class CVectRangeCmp : public binary_function<vector<T>, T, bool> {
public:
	bool operator()(const vector<T> &List, const unsigned int iOffset) const {
		if(List[0] <= iOffset && List[List.size()-1] >= iOffset) return true;
		return false;
	}
};
#endif
