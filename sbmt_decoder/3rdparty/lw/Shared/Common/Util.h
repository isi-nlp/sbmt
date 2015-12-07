// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include <string>
#include <iostream>

#ifndef NO_QT
#include <qstring.h>
#endif

#ifndef _UTIL_H
#define _UTIL_H

#ifdef _WIN32
#define FILE_PATH_SEPARATOR '\\'
#else
#define FILE_PATH_SEPARATOR '/'
#endif

namespace LW {

// These functions avoid silly asserts (???) in the CRT
bool isSpace(int ch);
bool isDigit(int ch);

/// Trim leading and trailing spaces in a string
void trim(std::string& s);
/// Trim leading spaces in a string
void trimLeft(std::string& s);
/// Trim trailing spaces in a string
void trimRight(std::string& s);
/// Convert string to uppercase
void toUpper(std::string& s);
/// Convert string to lowercase
void toLower(std::string& s);

/// This function will reset the failbit in case getline() fails because the buffer is too small
void safeGetLine(std::istream& in, char* pBuffer, size_t nBufferSize);

#ifndef NO_QT
class StringConverter {
	static void convert(const QString& in, std::string& out);
	static void convert(const std::string& in, QString& out);
};
#endif

} // namespace LW

#endif
