// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _strmanip_H_
#define _strmanip_H_

#include <string>
#include <vector>

using namespace std;

//vector<string> split(string  s, const char* sep);
vector<string> split(const string  , const char* sep);

bool blankLine(const char* line);
bool isCommentLine(string line);

char* rmReturn(char* line);


//! convers the characters in the string to the lower cases.
string& tolower(string &s);

// dectects the keyboard hit.
int kbhit();


#endif
