/* (C) Franz Josef Och; Information Science Institute / University of Southern California */
#ifndef my_stl_h_defined
#define my_stl_h_defined

#include <vector>
#include <iostream>
#include <list>
#include <string>
#include <utility>
#include <map>
#include <sstream>
#include <fstream>
#include <stack>
#include <cassert>
#include "my_namespace.hh"

using MY_NAMESPACE::string;
using MY_NAMESPACE::vector;
using MY_NAMESPACE::ostream;
using MY_NAMESPACE::istringstream;
using MY_NAMESPACE::list;
using MY_NAMESPACE::pair;
using MY_NAMESPACE::stack;
using MY_NAMESPACE::cout;
using MY_NAMESPACE::endl;

template<class T,class T2>
MY_NAMESPACE::ostream&operator<<(MY_NAMESPACE::ostream&out,const MY_NAMESPACE::pair<T,T2>&x)
{
  return out<<x.first<<","<<x.second<<" ";
}

template<class T>
MY_NAMESPACE::ostream&operator<<(MY_NAMESPACE::ostream&out,const MY_NAMESPACE::vector<T>&x)
{
    for(unsigned int i=0;i<x.size();++i)
	out << x[i] << " ";
    return out;
}

template<class A,class B>
MY_NAMESPACE::ostream&operator<<(MY_NAMESPACE::ostream&out,const MY_NAMESPACE::map<A,B>&x)
{
  for(typename MY_NAMESPACE::map<A,B>::const_iterator i=x.begin();i!=x.end();++i)
    out << i->first << ":" << i->second << ' ';
  return out;
}

template<class T>
ostream&operator<<(MY_NAMESPACE::ostream&out,const MY_NAMESPACE::list<T>&x)
{
  for(typename MY_NAMESPACE::list<T>::const_iterator i=x.begin();i!=x.end();++i)
    out << *i << " ";
  return out;
}

template<class T> bool operator<(const MY_NAMESPACE::vector<T> &x, const MY_NAMESPACE::vector<T> &y)
{
  if( &x == &y )
    return 0;
  else
    {
      if( y.size()<x.size() )
	return !(y<x);
      for(unsigned int iii=0;iii<x.size();iii++)
	{
	  if( x[iii]<y[iii] )
	    return 1;
	  else if( y[iii]<x[iii] )
	    return 0;
	}
      return x.size()!=y.size();//??
    }
}

template<class T> bool operator==(const MY_NAMESPACE::vector<T>&x,const MY_NAMESPACE::vector<T>&y)
{
    if( x.size()!=y.size())
	return 0;
    for(unsigned int i=0;i<x.size();++i)
	if(x[i]!=y[i])
	    return 0;
    return 1;
}

vector<string> tokenizeLine(const string&w)
{
  string s;
  std::istringstream iw(w.c_str());
  vector<string> a; 
  while(iw>>s)a.push_back(s);
  return a;
}

vector<string> tokenizeLine(const string&_w,char c)
{
  string w=_w;
  for(unsigned int i=0;i<w.length();++i)
    if(w[i]==c)
      w[i]=' ';
  return tokenizeLine(w);
}

vector<string> 
tokenizeLine(std::istringstream&iw)
{
  string s;
  vector<string> a; 
  while(iw>>s)a.push_back(s);
  return a;
}


bool 
tokenizeBiLine(std::istringstream&iline,vector<string>&l1,vector<string>&l2)
{
  l1.clear();l2.clear();
  bool flag=0;
  string s;
  while(iline>>s)
    if( s=="#")
      flag=1;
    else
      if( flag==0 )
	l1.push_back(s);
      else
	l2.push_back(s);
  return flag;
}

bool tokenizeBiLine(string line,vector<string>&l1,vector<string>&l2)
{
  std::istringstream iline(line.c_str());
  return tokenizeBiLine(iline,l1,l2);
}

template<class A,class B,class C>
class tri
{
public:
  A a;
  B b;
  C c;
  tri(){};
  tri(const A&_a,const B&_b,const C&_c)
    : a(_a),b(_b),c(_c) {}
};
template<class A,class B,class C>
tri<A,B,C>  make_tri(const A&a,const B&b,const C&c)
{
  return tri<A,B,C>(a,b,c);
}
template<class A,class B,class C>
bool operator==(const tri<A,B,C>&x,const tri<A,B,C>&y)
{ return x.a==y.a&&x.b==y.b&&x.c==y.c;}

template<class A,class B,class C>
ostream& operator<<(ostream&x,const tri<A,B,C>&y)
{ return x << y.a << ","<<y.b<<","<<y.c<< " ";}

template<class A,class B,class C>
bool operator<(const tri<A,B,C>&x,const tri<A,B,C>&y)
{
  if(y.b<x.b)return 1;
  if(x.b<y.b)return 0;
  if(x.a<y.a)return 1;
  if(y.a<x.a)return 0;
  if(x.c<y.c)return 1; 
  if(y.c<x.c)return 0;
  return 0;
}

bool getline(const vector<MY_NAMESPACE::istream*>&files,vector<string>&lines,bool&errorOccurred)
{
  string line;
  lines.resize(files.size());
  vector<bool> ok(files.size());
  for(unsigned int i=0;i<files.size();++i)
    ok[i]=getline(*files[i],lines[i]);
  for(unsigned int i=1;i<ok.size();++i)
    if( ok[i]!=ok[0] )
      {
	MY_NAMESPACE::cerr << "Error: stream0: " << ok[0] << " stream " << i << " " << ok[i] << MY_NAMESPACE::endl;
	errorOccurred=1;
	return 0;
      }
  return ok[0];
}


typedef list<pair<string,string> > AttributeList ;

void makeTag(const vector<string>&w,const vector<list<pair<string,string> > >&a, stack<tri<string,AttributeList,int> >&tags,
	     list<tri<string,AttributeList,pair<int,int> > > &spans,bool v=0)
{
  string ww=w.back();
  assert(ww[0]=='<');
  assert(ww[ww.length()-1]=='>');
  //MY_NAMESPACE::cerr << "ww: " << ww << endl;
  if( ww[1]=='/' )
    {
      string www=ww.substr(2,ww.length()-3);
      if( tags.size()==0 )
	{
	  if( v ) MY_NAMESPACE::cerr << "WARN: stack is empty: " << www << '\n';
	}
      else
	if( tags.top().a!=www )
	  {
	    MY_NAMESPACE::cerr << "ERROR: WRONG element on top of stack: '" << tags.top().a << "' '" << www << "'\n";
	  }
	else
	  {
	    spans.push_back(tri<string,AttributeList,pair<int,int> > (www,tags.top().b,MY_NAMESPACE::make_pair(tags.top().c,w.size()-1)));
	    tags.pop();
	  }
    }
  else
    {
      string www=ww.substr(1,ww.length()-2);
      tags.push(tri<string,AttributeList,int>(www,a.back(),w.size()-1));
    }
}

bool _mtokenizeXML(const string&s,vector<string>&w,list<tri<string,AttributeList,pair<int,int> > >&spans,stack<tri<string,AttributeList,int> >&tags,bool v=0)
{
  bool ok=1;
  bool readXML=0;
  w.clear();
  int state=0;
  vector<list<pair<string,string> > > a;
  for(unsigned int i=0;i<s.length();++i)
    {
      char c=s[i];
      switch( state )
	{
	case 0: // befWord
	  if( !isspace(s[i]) )
	    {
	      w.push_back(string());
	      w.back()+=c;
	      a.push_back(list<pair<string,string> >());
	      if( c=='<' )
		state=1;
	      else
		state=2;
	    }
	  break;
	case 1:
	  readXML=1;
	  if( c==' ' )
	    state=3;
	  else 
	    {
	      w.back()+=c;
	      if( c=='>' )
		{
		  makeTag(w,a,tags,spans,v);
		  w.pop_back();
		  state=0;
		}
	    }
	  break;
	case 2:
	  if( c=='<' )
	    {
	      readXML=1;
	      w.push_back(string());
	      w.back()+=c;
	      a.push_back(list<pair<string,string> >());
	      state=1;
	    }
	  else if( isspace(c) )
	    state=0;
	  else
	    w.back()+=c;
	  break;
	case 3: 
	  if(isspace(c))
	    {}
	  else if(c=='>')
	    {
	      if(a.back().size())
		{
		  if(a.back().back().first.length()!=0)
		    {
		      MY_NAMESPACE::cerr << "WARN: length not zero: ***" << s << "***   *@" << a.back().back().first << "@*\n";
		      ok=0;
		    }
		  a.back().pop_back();
		}
	      w.back()+=c;
	      makeTag(w,a,tags,spans,v);
	      w.pop_back();
	      state=0;
	    }
	  else if(c!='=')
	    {
	      if( a.back().size()==0 )
		a.back().push_back(pair<string,string>());
	      a.back().back().first+=c;
	    }
	  else
	    state=4;
	  break;
	case 4:
	  /*	  if(c!='"'&&c!='\'')
	    {
	      MY_NAMESPACE::cerr << "ERROR1: missing \" at position " << i << " seen '" << c << "' in " << s << '\n';
	      ok=0;
	    }*/
	  if( c=='"' )
	    state =5;
	  else if(c=='\'')
	    state=6;
	  else if(isspace(c))
	    ;
	  else 
	    {a.back().back().second+=c;state=7;}
	  break;
	case 5:
	  if(c!='"')
	    a.back().back().second+=c;
	  else
	    {
	      state=3;
	      a.back().push_back(pair<string,string>());
	    }
	  break;
	case 6:
	  if(c!='\'')
	    a.back().back().second+=c;
	  else
	    {
	      state=3;
	      a.back().push_back(pair<string,string>());
	    }
	  break;
	case 7:
	  if(c!='>'&&!isspace(c))
	    a.back().back().second+=c;
	  else
	    {
	      if(c=='>')
		--i;
	      state=3;
	      a.back().push_back(pair<string,string>());
	    }
	  break;
	default:
	  abort();
	}
    }
  if( state!=0&&state!=2 )
    {
      MY_NAMESPACE::cerr << "ERROR: wrong state at end of line: " << state << '\n';
      ok=0;
    }
  return ok==1 && readXML==1;
}

inline int fabs(int x)
{
  if (x<0 )
    return -x;
  else
    return x;
}
#if !defined(__linux__)
inline double fabs(double    x)
{
  if (x<0 )
    return -x;
  else
    return x;
}
#else
inline long double fabs(long double    x)
{
  if (x<0 )
    return -x;
  else
    return x;
}
#endif

bool mtokenizeXML(const string&s,vector<string>&w,list<tri<string,AttributeList,pair<int,int> > >&spans,bool v=0)
{
  stack<tri<string,AttributeList,int> > tags;
  return _mtokenizeXML(s,w,spans,tags,v);
}
//#if defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
double memoryUse()
{
  //return (sysconf(_SC_AVPHYS_PAGES)*sysconf(_SC_PAGE_SIZE))/(1024.0*1024.0);
  return 0.0;
}
double used_time()
{
#if defined(__linux__)
  enum __rusage_who who =RUSAGE_SELF;
#else
  int who=(int)RUSAGE_SELF;
#endif
  struct rusage rusage;
  getrusage(who, &rusage);
  return rusage.ru_utime.tv_sec+rusage.ru_utime.tv_usec/1000000.0;
}
//#endif
#include <string>

#ifdef PRINT_TIME_STDERR
#define PRINT_TIME_COUT MY_NAMESPACE::cerr
#else
#define PRINT_TIME_COUT MY_NAMESPACE::cout
#endif

class PrintTime
{
 public:
  string info;
  bool flushit;
  time_t startTime;
  PrintTime(const string&i,bool fi=1) : info(i),flushit(fi)
    {
      PRINT_TIME_COUT << "START: " << info;
      if(flushit)
	PRINT_TIME_COUT << endl;
      else
	PRINT_TIME_COUT << '\n';
      startTime=time(NULL); 
    }
  ~PrintTime()
    {
      PRINT_TIME_COUT << "  END: " << difftime(time(NULL),startTime)  << " " << info;
      if(flushit)
	PRINT_TIME_COUT << endl;
      else
	PRINT_TIME_COUT << '\n';      
    }
}; 

class Accum
{
 private:
  double sum;
  int nElem;
  double theMinimum,theMaximum;
  bool minimumUpdated;
 public:
  Accum() : sum(0),nElem(0){}
  void add(double x)
    {
      if(nElem==0)
	{
	  minimumUpdated=1;
	  theMinimum=x;
	  theMaximum=x;
	}
      else
	{
	  if( x<theMinimum )
	    {
	      theMinimum=x;
	      minimumUpdated=1;
	    }
	  else
	    minimumUpdated=0;
	  theMaximum=MY_NAMESPACE::max(theMaximum,x);
	}
      sum+=x;
      nElem+=1;
    }
  double minElement()const
    {
      assert(nElem);
      return theMinimum;
    }
  double maxElement()const
    {
      assert(nElem);
      return theMaximum;
    }
  double avg()const
    {
      assert(nElem);
      return sum/nElem;
    }
  bool defined()const
    {
      return nElem>0;
    }
  bool minUpdated()
    {
      return minimumUpdated;
    }
};

ostream&operator<<(ostream&out,const Accum&x)
{
  if( x.defined() )
    return out << x.minElement() << "," << x.avg() << "," << x.maxElement() << " ";
  else
    return out << "undefined ";
};

#endif
