// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/**
 * Implementation for XMLConfig class
 */
//*****************************************************************************
// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved


#include "Common/Util.h"

#include <algorithm>
#include <ctype.h>

#ifndef NO_QT
#include <qbytearray.h>
#endif

using namespace std;

namespace LW {

// This functions avoid silly asserts (???) in the CRT
bool isSpace(int ch) 
{
	if ((ch < 0) || (ch > 127)) {
		return false;
	}
	else {
		return (isspace(ch) != 0);
	}
}

bool isDigit(int ch) 
{
	return ((ch >= '0') && (ch <= '9'));
}

void trim(string& sInput)
{
	trimLeft(sInput);
	trimRight(sInput);
}

void trimLeft(string& sInput) 
{
	// Trim left
	sInput.erase(0, sInput.find_first_not_of(' '));
}

void trimRight(string& sInput) 
{
	// Trim right
	sInput.erase(sInput.find_last_not_of(' ') + 1 );
}

void toUpper(std::string& sStr) {
	std::transform(sStr.begin(), sStr.end(), sStr.begin(), static_cast<int(*)(int)> (toupper));
}

void toLower(std::string& sStr) {
	std::transform(sStr.begin(), sStr.end(), sStr.begin(), static_cast<int(*)(int)> (tolower));
}

void safeGetLine(std::istream& in, char* pBuffer, size_t nBufferSize)
{
	if (in.fail() && !in.bad() && !in.eof()) {
		in.clear();
	}

    in.getline(pBuffer, static_cast<std::streamsize>(nBufferSize));
	if (in.fail()&& !in.bad() && !in.eof()) {
		in.clear();
	}
}

#ifndef NO_QT

void StringConverter::convert(const QString& in, std::string& out)
{
	QByteArray arr = in.toUtf8();
	out.clear();
	out.append(arr.data(), arr.length());
}

void StringConverter::convert(const std::string& in, QString& out)
{
	out = QString::fromUtf8(in.data(), in.length());
}

#endif

} // namespace
