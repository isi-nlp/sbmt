// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _FILEOP_H
#define _FILEOP_H
#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#endif
#include "Common/stldefs.h"

#ifdef WIN32
#define SETBINARYCIN {if( _setmode( _fileno( stdin ), _O_BINARY ) == -1 ) cerr << "Cannot set 'stdin' into binary mode";}
#define SETBINARYCOUT {if( _setmode( _fileno( stdout ), _O_BINARY ) == -1 ) cerr << "Cannot set 'stdout' into binary mode";}
#else
#define SETBINARYCIN
#define SETBINARYCOUT
#endif

/**handle binary writes for output to file of probability tables based on sun binary format*/
#ifdef SLM_SWAP_BYTES
#define SWAP_FIELD(_x, _y, _size) {\
	if (_size == sizeof(int)){SWAP_WORD(_x, _y)}  \
	else if (_size == sizeof(double)){SWAP_DOUBLE(_x, _y)}\
	else if (_size == sizeof(short)) {SWAP_HALF((_x, _y))}  \
	else {SWAP_NSIZE(_x, _y, _size);\
}
			 
#define SWAP_HALF(_x, _y) {\
	*((char*)(_y)+1) = *((char*)(_x)+0); \
	*((char*)(_y)+0) = *((char*)(_x)+1); \
}

#define SWAP_WORD(_x, _y) {\
	*((char*)(_y)+3) = *((char*)(_x)+0); \
	*((char*)(_y)+2) = *((char*)(_x)+1); \
	*((char*)(_y)+1) = *((char*)(_x)+2); \
	*((char*)(_y)+0) = *((char*)(_x)+3); \
}
			 
#define SWAP_DOUBLE(_x, _y) {\
	*((char*)(_y)+7) = *((char*)(_x)+0); \
	*((char*)(_y)+6) = *((char*)(_x)+1); \
	*((char*)(_y)+5) = *((char*)(_x)+2); \
	*((char*)(_y)+4) = *((char*)(_x)+3); \
	*((char*)(_y)+3) = *((char*)(_x)+4); \
	*((char*)(_y)+2) = *((char*)(_x)+5); \
	*((char*)(_y)+1) = *((char*)(_x)+6); \
	*((char*)(_y)+0) = *((char*)(_x)+7); \
}

#define SWAP_NSIZE(_x, _y, _size) {\
	for(int _isize = 0; _isize < _size; _isize++)\
		*((char*)(_y)+(_size-_isize)) = *((char*)(_x)+_isize); \
}

inline void SafeBinWrite(ostream &_os, void *_x, unsigned int _size){
	if (_size == sizeof(short)) {char _tmpvar[2]; SWAP_HALF((char*)_x, _tmpvar); _os.write(_tmpvar, _size);}  
	else if (_size == sizeof(int)){char _tmpvar[4]; SWAP_WORD((char*)_x, _tmpvar); _os.write(_tmpvar, _size);}  
	else if (_size == sizeof(double)){char _tmpvar[8]; SWAP_DOUBLE((char*)_x, _tmpvar); _os.write(_tmpvar, _size);} 
	else {_os.write((char*)_x, _size);}  
}

inline void SafeBinRead(istream &_is, void *_x, unsigned int _size){
	if (_size == sizeof(int)){char _tmpvar[4]; _is.read( _tmpvar, _size); SWAP_WORD(_tmpvar, (char*)_x); }  
	else if (_size == sizeof(double)){char _tmpvar[8]; _is.read( _tmpvar, _size); SWAP_DOUBLE(_tmpvar, (char*)_x);}  
	else if (_size == sizeof(short)) {char _tmpvar[2]; _is.read( _tmpvar, _size); SWAP_HALF(_tmpvar, (char*)_x);}  
	else {_is.read( (char*)_x, _size);}  
}
#else
inline void SafeBinWrite(ofstream &_os, void *_x, unsigned int _size){
	_os.write((char*)_x, _size);
}
inline void SafeBinRead(ifstream &_is, void *_x, unsigned int _size){
	_is.read((char*)_x, _size);
}
#endif

/**Macro to determine if a file exists*/
#define IFNOTFILEFAIL(_FILENAMESTRING)  \
	if(!fileExist(_FILENAMESTRING)) \
		cerr << "Could not open file: " << _FILENAMESTRING.c_str() << endl; else 
			 
			 
//void GetFilenamesInDir(string &zFullPath, STRVECT &fnamelist);
/**Determines if a file exists*/
inline bool fileExist(const string &zFullPath){
	ifstream ifs(zFullPath.c_str());
//#if defined(linux) || defined(__linux) || defined(__linux__) 
//	ifstream ifs(zFullPath.c_str(), ios::nocreate| ios::in);
//#else
//	ifstream ifs(zFullPath.c_str(), ios::in);
//#endif
	if(ifs.fail() || !ifs.good()){
		ifs.close();
		return false;
	}
	ifs.close();
	return true;
}

inline bool fileExist(const char *zFullPath){
	ifstream ifs(zFullPath);
//#if defined(linux) || defined(__linux) || defined(__linux__) 
//	ifstream ifs(zFullPath.c_str(), ios::nocreate| ios::in);
//#else
//	ifstream ifs(zFullPath.c_str(), ios::in);
//#endif
	if(ifs.fail() || !ifs.good()){
		ifs.close();
		return false;
	}
	ifs.close();
	return true;
}

///tests eof by reading ahead 1 byte
inline bool safeEOF(istream &s){
	if(s.eof()) return true;
	(void)s.get();
	if(s.eof()) return true;
	s.unget();
	return false;
}
#endif
