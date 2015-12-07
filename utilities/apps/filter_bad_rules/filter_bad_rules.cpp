# include <RuleReader/Rule.h>
# include <iostream>
# include <string>
# include <time.h>
# include <cstring>

using namespace std;
using namespace ns_RuleReader;

static
const string
logstamp( void )
{
  struct tm now;
  char buffer[32];
  time_t then = time(0);
  memcpy( &now, localtime( &then ), sizeof(struct tm) );
  strftime( buffer, sizeof(buffer), "# %Y-%m-%d %H:%M:%S ", &now );
  return string(buffer);
}

int 
main( int argc, char* argv[] ) 
{
  bool verbose = false;
  time_t start = time(0);

  // speed it
  std::ios_base::sync_with_stdio(false); 

  // verbose mode?
  if ( argc > 1 ) {
    if ( strcmp(argv[1],"-v") == 0 || strcmp(argv[1],"--verbose") == 0 )
      verbose = true;
  }

  // say hello
  if ( verbose )
    cerr << logstamp() << "This is " << argv[0] << endl;

  std::string line;

  // off to work you go
  unsigned long long lineno = 0;
  unsigned long long fail = 0ull;
  while ( getline(cin,line) ) {
    ++lineno;
    if ( verbose && ( lineno & 0xFFFFFull ) == 0 ) 
      cerr << logstamp() << "processed " << lineno << " rules" << endl;

    try {
      Rule rule( line );
      if ( verbose ) cerr << "line " << lineno << endl;
      cout << line << endl;
    } catch (exception const& e) {
      // the exception usually contains the offending rule
      if ( verbose ) {
	cerr << lineno << ": " << e.what() << endl;
      } else {
	cerr << line << endl;
      }
      ++fail;
    }
  }

  if ( verbose ) { 
    if ( (lineno & 0xFFFFFull) ) 
      cerr << logstamp() << "processed " << lineno << " rules" << endl;
    cerr << logstamp()
	 << fail << " failed, "
	 << (lineno-fail) << " passed, "
	 << (time(0)-start) << " s duration"
	 << endl;
  }

  return 0;
}
