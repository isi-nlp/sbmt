#define BOOST_AUTO_TEST_MAIN
#define GRAEHL__SINGLE_MAIN

#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED 1
#endif

//#define BENCH
#include <iostream>

#include <graehl/shared/config.h>
#include <graehl/tt/ttconfig.hpp>

#include <graehl/tt/transducergraph.hpp>
#include <graehl/shared/graph.hpp>

#include <graehl/shared/charbuf.hpp>

#include <graehl/shared/symbol.hpp>
#include <graehl/shared/string.hpp>
#include <graehl/shared/tree.hpp>
#include <graehl/tt/transducer.hpp>
#include <graehl/shared/hypergraph.hpp>
#include <graehl/shared/memoindex.hpp>

#include <graehl/shared/indexgraph.hpp>
#include <graehl/shared/treetrie.hpp>
#include <graehl/tt/transducermatch.hpp>

#include <graehl/forest-em/forest.hpp>


#ifndef GRAEHL_TEST

template <class TX>
void xdcr_cat(std::istream &in,std::ostream &out) {
  unsigned i=0;
  while (in) {
        ++i;
        TX t;
        in >> t;
        if (!in.good()) {
          if (in.eof()) {
                Config::warn() << "couldn't read: eof\n";
                return;
          }
          Config::warn() << "Transducer #" << i << " was bad!\n";

          //return;
          in.clear();      // cygwin running MSVC++ exe doesn't detect textfile eof! (expects an actual ascii ctrl-z)
        } else {
          out << t;
        }
  }
}

int
#ifdef _MSC_VER
__cdecl
#endif
main(int argc, char *argv[])
{

  INITLEAK;
    using namespace std;
    using namespace graehl;
  bool tt=true;
  Config::message() << "Expecting ";
  if (argc > 1) {
        tt=false;
        Config::message() << "tree-string" ;
  } else {
        Config::message() << "tree-tree" ;
  }
  Config::message() <<" transducers as input; will validate and echo them back" << endl;
  if (tt)
        xdcr_cat<TT>(cin,cout);
  else
        xdcr_cat<TS>(cin,cout);
  CHECKLEAK(0);
  return 0;
}

#endif
