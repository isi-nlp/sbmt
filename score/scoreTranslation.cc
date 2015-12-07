/* (C) Franz Josef Och; Information Science Institute / University of Southern California */

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <cstring>
#include <ctype.h>
#include <sstream>
#include "Array2.hh"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <set>

#include "mystl.hh"

using std::ifstream;
using std::cout;
using std::endl;

using namespace std;
int NSIZE=4;

map<vector<string>,double> ngramWeights;
int makeCorrectUseOfLengthPenaltyOfBleu = 1;

int fabs(int a,int b)
{
  if ( a-b < 0 ) return b-a;
  else return a-b;
}

int type=0;
double references=0;
int verbose=0;

ostream&
operator<<(ostream&out,const vector<string>&s)
{
  for(unsigned int i=0;i<s.size();++i)out << s[i] << " ";
  return out;
}


vector<string>
vec(const string&s)
{
  string w;
  vector<string> vs;
  istringstream is(s.c_str());
  while(is>>w)vs.push_back(w);
  return vs;
}

template<class T>
void
operator+=(vector<T>&a,const vector<T>&b)
{
  assert(a.size()==b.size());
  for(unsigned int i=0;i<a.size();++i)
    a[i]+=b[i];
}

vector<double>
bootstrapResampling(const vector<vector<double> >&scores)
{
  assert(scores.size());
  vector<double> x(scores[0].size());
  for(unsigned int i=0;i<scores.size();++i)
    x+=scores[int((random()/(double)(RAND_MAX))*(scores.size()-1))];
  return x;
}

#include "bleu.hh"

int
per(const vector<string>&_s1,const vector<string>&_s2)
{
  vector<string>s1,s2;
  if( _s2.size()<_s1.size() )
    {s1=_s1;s2=_s2;}
  else
    {s2=_s1;s1=_s2;}
  int correct=0,total=0;
  map<string,int> words;
  for(unsigned int i=0;i<s1.size();++i){words[s1[i]]++;total++;}
  for(unsigned int i=0;i<s2.size();++i)
    if( words[s2[i]] )
      {
	words[s2[i]]--;
	correct++;
      }

  return total-correct;
}

template<class T>
int
lev(const T&s1,const T&s2)
{
  Array2<int,vector<int> > a(s1.size()+1,s2.size()+1,1000);
  Array2<pair<int,int>,vector<pair<int,int> > > back(s1.size()+1,s2.size()+1,pair<int,int>(0,0));
  for ( unsigned int i=0; i<=(unsigned int)(s1.size()); i++ ) {
    for( unsigned int j=0; j<=(unsigned int)(s2.size()); j++ ) {
      if( i==0 && j==0 ) {
	a(i,j)=0;
      } else {
	int aDEL=100,aINS=100,aSUB=100;
	if ( i>0 )
	  aDEL=a(i-1,j)+1;
	if ( j>0 )
	  aINS=a(i,j-1)+1;
	if ( i>0 && j>0 )
	  aSUB=a(i-1,j-1)+ !(s1[i-1]==s2[j-1]);
	if( aSUB<=aDEL && aSUB<=aINS ) {
	  a(i,j)=aSUB;
	  back(i,j)=pair<int,int>(i-1,j-1);
	} else if( aDEL<=aSUB && aDEL<=aINS ) {
	  a(i,j)=aDEL;
	  back(i,j)=pair<int,int>(i-1,j);
	} else {
	  a(i,j)=aINS;
	  back(i,j)=pair<int,int>(i,j-1);
	}
      }
    }
  }
  return a(s1.size(),s2.size());
}


double
computeErr(const vector<double>&x)
{
  vector<double> nhyp(NSIZE),nhypref(NSIZE);
  for(int i=0;i<NSIZE;++i)
    {
      nhyp[i]=x[i];
      nhypref[i]=x[i+NSIZE];
    }
  double totalHypLength=x[2*NSIZE];
  double totalRefLength=x[2*NSIZE+1];
  double totalSumRefLength=x[2*NSIZE+2];
  double average=0.0;
  double lengthPenalty=0.0;
  if( type==1 )
    {
      for(int i=0;i<NSIZE;++i){
	average+=log(nhypref[i]/(float)nhyp[i]);
      }
      if( totalHypLength<totalRefLength )
	lengthPenalty= 1-(double)(totalRefLength)/totalHypLength;
      return 100.0*exp(average/NSIZE+lengthPenalty);
    }
  else
    {
      double ngrams=0,scores=0;
      for(int i=0;i<NSIZE;++i){
	ngrams+=(float)nhyp[i];
	scores+=nhypref[i];
	average+=(nhypref[i]/(float)nhyp[i]);
      }
      lengthPenalty=references*nhyp[0]/totalSumRefLength;
      if( lengthPenalty>=1 ) lengthPenalty=1;
      else if( lengthPenalty<=0 ) lengthPenalty=0;
      else
	{
	  double beta= -log(0.5)/log(1.5)/log(1.5);
	  lengthPenalty= exp (-beta*log(lengthPenalty)*log(lengthPenalty));
	}
      return average*lengthPenalty;
    }
}

void
usage( const char* arg0 )
{
  const char* app = strrchr( arg0, '/' );
  if ( app ) ++app;
  else app = arg0;

  cerr << "Tool for scoring translation quality" << endl;
  cerr << "Usage: " << app << " [options] -hyp hypothesis ref1 [ref2 [..]]" << endl;
  cerr << endl;

  cerr << "Optional arguments:" << endl;
  cerr << " -prec i            i digits for score, i+2 for LR/BP (default i=1)" << endl;
  cerr << " -v                 verbosity level 1" << endl;
  cerr << " -V                 verbosity level 2" << endl;
  cerr << " -VV                verbosity level 3" << endl;
  cerr << " -n k               compute 1-grams through k-grams (default k=4)" << endl;
  cerr << " -per               [type 3]" << endl;
  cerr << " -perdels           [type 10]" << endl;
  cerr << " -wer               [type 0]" << endl;
  cerr << " -wer+per           [type 4]" << endl;
  cerr << " -bleu              [type 1, makeCorrectUseOfLengthPenalty=1]" << endl;
  cerr << " -bleuNistVersion   [type 1, makeCorrectUseOfLengthPenalty=0]" << endl;
  cerr << " -nist              [type 2, makeCorrectUseOfLengthPenalty=1]" << endl;
  cerr << " -nistWeights fn    For type=2, obtain weights from fn." << endl;
  cerr << endl;

  cerr << "Mandatory arguments: (file line counts must match)" << endl;
  cerr << " -hyp hypothesis    the translation to be scored." << endl;
  cerr << " ref1               first reference (mandatory)" << endl;
  cerr << " ref2               further reference (optional)" << endl;
  cerr << endl;

  exit(1);
}

int
main( int argc, char* argv[] )
{
  if ( argc<3 ) usage(argv[0]);

  vector<istream*> files(1,(istream*)(0));
  vector<string> l;
  string infoFileName;
  string hypFile;
  int nRef=0;
  int digits = 1;
  for ( int i=1; i<argc; ++i ) {
    string cursor(argv[i]);

    if ( cursor=="-h" || cursor=="--help" )
      usage(argv[0]);
    else if( cursor=="-v" )
      verbose =1;
    else if( cursor=="-V" )
      verbose=2;
    else if( cursor=="-VV" )
      verbose=3;
    else if( cursor=="-bleu" )
      type=1;
    else if( cursor=="-n" )
      NSIZE=atoi(argv[++i]);
    else if( cursor=="-prec" )
      digits=atoi(argv[++i]);
    else if( cursor=="-per" )
      type=3;
    else if( cursor=="-perdels" )
      type=10;
    else if( cursor=="-wer" )
      type=0;
    else if( cursor=="-wer+per" )
      type=4;
    else if( cursor=="-bleuNistVersion" )
      {
	type=1;
	makeCorrectUseOfLengthPenaltyOfBleu=0;
      }
    else if( cursor=="-nist" )
      type=2;
    else if( cursor=="-nistWeights" )
      {
	infoFileName=argv[i+1];
	++i;
      }
    else if( cursor=="-hyp" )
      {
	files[0]=new ifstream(argv[++i]);
	hypFile=argv[i];
      }
    else
      {
	files.push_back(new ifstream(argv[i]));
	nRef++;
      }
  }

  // sanity checks
  if ( files.size()==1 ) {
    cerr << "ERROR: no reference has been given." << endl;
    return 1;
  }
  if ( files[0]==0 ) {
    cerr << "ERROR: no hypothesis translation has been given." << endl;
    return 1;
  }
  if ( digits < 1 ) {
    cerr << "ERROR: too few digits precision." << endl;
    return 1;
  }

  swap(files[0],files[1]);
  references=files.size()-1;
  double totalWeights=0.0;
  if( type==2 )
    {
      ifstream infofile(infoFileName.c_str());
      string l;
      while(getline(infofile,l))
	{
	  vector<string> xl=tokenizeLine(l);
	  double weight=atof(xl[0].c_str());
	  ngramWeights[substr(xl,1,xl.size()-2)]=weight;
	  totalWeights+=weight;
	}
      if( verbose )
	cout << "READ: " << ngramWeights.size() << " weights for NIST scoring.\n";
      if( ngramWeights.size()==0 )
	{
	  cout << "NIST score not computed because weights are missing.\n";
	  exit(1);
	}
    }
  int n=0;

  if( type==0 || type==3 || type==4 || type==10 )
    {
      double err=0;
      double ges=0;
      vector<vector<double> > levs;
      bool errors=0;
      while(getline(files,l,errors))
	{
	  vector<string> l1=tokenizeLine(l[0]),l2=tokenizeLine(l[1]);
	  double lv=0;
	  if( type==10 )
	    {
	      set<string> x;
	      for(unsigned int i=0;i<l.size();++i)
		{
		  if( i==1 ) continue;
		  l1=tokenizeLine(l[i]);
		  for(unsigned int j=0;j<l1.size();++j)
		    x.insert(l1[j]);
		}
	      for(unsigned int i=0;i<l2.size();++i)
		if( x.count(l2[i])==0 )
		  {
		    lv++;
		    cerr << "ERROR: " << l2[i] << endl;
		  }
	    }
	  else
	    {
	      if(type==3 )
		lv=per(l1,l2);
	      else if( type==4 )
		lv=(per(l1,l2)+lev(l1,l2))/2.0;
	      else
		lv=lev(l1,l2);
	      for(unsigned int i=2;i<l.size();++i)
		if( type==3 )
		  lv=min(lv,double(per(tokenizeLine(l[i]),l2)));
		else if( type==4 )
		  lv=min(lv,double(lev(tokenizeLine(l[i]),l2)+per(tokenizeLine(l[i]),l2))/2.0);
		else
		  lv=min(lv,double(lev(tokenizeLine(l[i]),l2)));
	    }
	  vector<double> x;
	  x.push_back(lv);
	  levs.push_back(x);
	  err+=lv;
	  ges+=l1.size();
	  if( verbose>1 )
	    cout << n++ << " errors: " << lv << " " << lv/(float)(l2.size()) << " " << l2 << endl;
	}

      if( errors==1 )
	abort();
      if(verbose==0)cout.precision(4);
      cout << hypFile << " ";
      if( type==0 )
	cout << "WER";
      else if(type==3 )
	cout << "PER";
      else if(type==4)
	cout << "WER+PER";
      cout << "r"<<nRef<<"[%]    ";
      cout << (err/ges)*100.0;
      if(verbose==0)cout.precision(10);
      cout << " errors: " << err << " events: " << ges << " ";
      if(verbose==0)cout.precision(4);
      vector<double> errs;
      for(unsigned int i=0;i<1000;++i)
	{
	  vector<double> err=bootstrapResampling(levs);
	  errs.push_back(err[0]);
	}
      std::sort(errs.begin(),errs.end());
      double gesx=ges/100.0;
      cout << " 95%-conf.int.: " << errs[24]/gesx << " - " << errs[974]/gesx << " median: " << errs[499]/gesx;
      if( verbose==3 )
	{
	  for(unsigned int i=0;i<1000;++i)
	    cout << "RANK: " << i << " " << errs[i] << endl;
	}
      if(verbose==0)cout.precision(2);
      //cout << " delta: " << max((errs[24]-err)/gesx,(errs[974]-err)/gesx)  << endl;
      cout << " delta: " << max(err-errs[24]/gesx,errs[974]/gesx-err)  << endl;
    }
  else
    {
      vector<vector<double> > stats;
      vector<double> stat;
      vector<double> nhyp(NSIZE,0),nhypref(NSIZE,0);
      int totalRefLength=0,totalHypLength=0,totalSumRefLength=0;
      bool errors=0;
      while ( getline( files, l, errors ) ) {
	vector<double> nhypsent(NSIZE,0),nhypx(NSIZE,0);
	double a4=0,b4=0;
	vector<vector<string> > REFS;
	vector<string> HYP;
	for(unsigned int k=0;k<l.size();++k)
	  if( k==1 )
	    HYP=tokenizeLine(l[k]);
	  else
	    REFS.push_back(tokenizeLine(l[k]));
	int hypLength=HYP.size(),refLength=10000;
	int sumreflength=0;
	for(unsigned int k=0;k<REFS.size();++k)
	  {
	    sumreflength+=REFS[k].size();
	    totalSumRefLength+=REFS[k].size();
	    if( makeCorrectUseOfLengthPenaltyOfBleu )
	      {
		if ( fabs(int(REFS[k].size()) - int(HYP.size())) < fabs(int(refLength)-int(HYP.size())) )
		  refLength=REFS[k].size();
		else if ( fabs(int(REFS[k].size()) - int(HYP.size())) == fabs(int(refLength) - int(HYP.size())) &&
                  refLength>int(REFS[k].size()) )
		  refLength=REFS[k].size();
	      }
	    else
	      {
		if( int(REFS[k].size()) < refLength )
		  refLength=REFS[k].size();
	      }
	  }
	totalHypLength+=hypLength;
	totalRefLength+=refLength;
	for(int i=0;i<NSIZE;++i){
	  double a=0,b=0;
	  computeBLEU(REFS,HYP,i,a,b,type);
	  nhyp[i]+=a;
	  nhypref[i]+=b;
	  a4+=a;
	  b4+=b;
	  nhypsent[i]+=b;
	  nhypx[i]+=a;
	}
	stat.resize(2*NSIZE+3);
	for ( int i=0; i<NSIZE; ++i )
	  {
	    stat[i]=nhypx[i];
	    stat[NSIZE+i]=nhypsent[i];
	  }
	stat[2*NSIZE]=hypLength;
	stat[2*NSIZE+1]=refLength;
	stat[2*NSIZE+2]=sumreflength;
	if( verbose>1 )
	  cout << "SENTTOTAL " << a4 << " " <<  b4 << endl;
	if( verbose>1 )
	  {
	    cout << n++ << " errors: ";
	    for(int i=0;i<NSIZE;++i)
	      cout << nhypsent[i] << ' ';
	    cout << "length " << hypLength << " ";
	    for(int i=0;i<NSIZE;++i)
	      cout << nhyp[i] << ' ' << nhypref[i] << ' ';
	    cout << '\n';
	  }
	stats.push_back(stat);
      }
      if( errors )
	abort();
      double average=0.0;
      double lengthPenalty=0.0;
      double err=0.0;
      if( type==1 )
	{
	  for(int i=0;i<NSIZE;++i){
	    if( verbose )
	      cout << "n:" << i+1 << " ngrams:" << nhyp[i] << " fitting:" << nhypref[i] << " score:" << (nhypref[i]/(float)nhyp[i]) << endl;
	    average+=log(nhypref[i]/(float)nhyp[i]);
	  }
	  if( totalHypLength<totalRefLength )
	    lengthPenalty= 1-(double)(totalRefLength)/totalHypLength;
	  if (verbose==0) cout.precision(digits+2); // !!! HERE !!!
	  cout << hypFile << " BLEUr"<<nRef<<"n"<<NSIZE<<"[%] " << 100.0*exp(average/NSIZE+lengthPenalty) << " ";
	  cout << "brevityPenalty: " << exp(lengthPenalty) << " lengthRatio: " << totalHypLength/(double)(totalRefLength) << " ";
	  err=100.0*exp(average/NSIZE+lengthPenalty);
	}
      else
	{
	  double ngrams=0,scores=0;
	  for(int i=0;i<4;++i){
	    ngrams+=(float)nhyp[i];
	    scores+=nhypref[i];
	    if( verbose )
	      cout << "n:" << i+1 << " ngrams:" << nhyp[i] << " fitting:" << nhypref[i] << " score:" << (nhypref[i]/(float)nhyp[i]) << endl;
	    average+=(nhypref[i]/(float)nhyp[i]);
	  }
	  if( verbose )
	    cout << "average NIST score without references: " << average << endl;
	  lengthPenalty=references*nhyp[0]/totalSumRefLength;
	  if( verbose )
	    cout << "average reference length: " << totalSumRefLength/references << endl;
	  if( verbose )
	    cout << "initial length penalty: " << lengthPenalty << endl;
	  if( lengthPenalty>=1 ) lengthPenalty=1;
	  else if( lengthPenalty<=0 ) lengthPenalty=0;
	  else
	    {
	      double beta= -log(0.5)/log(1.5)/log(1.5);
	      lengthPenalty= exp (-beta*log(lengthPenalty)*log(lengthPenalty));
	    }
	  if( verbose )
	  cout << "real length penalty: " << lengthPenalty << endl;
	  if(verbose==0)cout.precision(digits+2); // !!! HERE !!!
	  cout << hypFile << " NISTr"<<nRef<<"n4     " << average*lengthPenalty << " brevityPenalty: " << lengthPenalty << " weight-sum: " << totalWeights;
	  err=average*lengthPenalty;
	}
      vector<double> errs;
      for(unsigned int i=0;i<1000;++i)
	{
	  vector<double> err=bootstrapResampling(stats);
	  errs.push_back( computeErr(err) );
	}
      std::sort(errs.begin(),errs.end());
      cout << " 95%-conf.: " << errs[24] << " - " << errs[974];
      if(verbose==0)cout.precision(2);
      cout << " delta: " << max((errs[24]-err),(errs[974]-err))  << endl;
    }
}



