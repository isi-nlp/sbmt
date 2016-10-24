// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#include "strmanip.h"
#include <stdio.h>
#ifdef WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <cstring>
#endif

vector<string> split(const string str, const char* dep)
{
  string s = "";
  s += str;
	vector<string> ret;

	char* p  = strtok((char*)s.c_str(), dep);

	if(!p) return ret;
	
	while(1) {
		ret.push_back(string(p));
		p  = strtok(NULL, dep);
		if(!p) break;
	}
	return ret;
}

bool blankLine(const char* line)
{
#ifdef WIN32
	char* tmpBuf = new char[strlen(line)+1];
#else 
	 char tmpBuf[strlen(line)+1];
#endif
	 strcpy(tmpBuf, line);
	 int index = (int)strlen(line)-1;
	 
	 while((index >= 0)&&(tmpBuf[index] == '\r' || tmpBuf[index] == '\n')) index--;
	 tmpBuf[index + 1] = '\0';
	 if(tmpBuf[0] == '\0')	 {
#ifdef WIN32
		 delete tmpBuf;
#endif
		 return true;
	 }
	 else {
#ifdef WIN32
		 delete tmpBuf;
#endif
		 return false;
	 }
}

char* rmReturn(char* buffer)
{
	int index = (int)(strlen(buffer) - 1);
  	while(index>= 0 &&(buffer[index] == '\r' || buffer[index] == '\n')) index--;
	buffer[index + 1] = '\0';
	return buffer;
}

bool isCommentLine(string line) {

 /*
  * ignore the comment line.
  */
  size_t i = 0;
  while(i < line.length()) {
      if(line[i] == ' ' || line[i] == '\t'){
          i++;
      } else {
          break;
      }
  }
  // there are still at least two characters remaining.
  if(i + 1 < line.length()){
    if(line[i] == '/'  && line[i+1] == '/'){
        // comment line.
        return true;
    }
  }

  return false;
}

#ifndef WIN32
// dectects the keyboard hit.
int kbhit()
{
    // chagne mode
    int fd = STDIN_FILENO;
    struct termios t;

    if (tcgetattr(fd, &t) < 0)
    {
            perror("tcgetattr");
            return -1;
    }

    t.c_lflag &= ~ICANON;

    if (tcsetattr(fd, TCSANOW, &t) < 0)
    {
            perror("tcsetattr");
            return -1;
    }

    setbuf(stdin, NULL);

    // detect keybd hit.
   fd_set rfds;
   struct timeval tv;
   int retval;

   // Watch stdin (fd 0) to see when it has input.
   FD_ZERO(&rfds);
   FD_SET(0, &rfds);
   // Wait up to five seconds.
   tv.tv_sec = 0;
   tv.tv_usec = 10;

   retval = select(1, &rfds, NULL, NULL, &tv);

   return retval;
}
#endif // Not WIN32

//! convers the characters in the string to the lower cases.
string& tolower(string &s)
{
    size_t i;
    for(i = 0; i < s.length(); ++i){
	s[i] = tolower((int)s[i]);
    }
    return s;
}

