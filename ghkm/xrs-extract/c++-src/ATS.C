
/***************************************************************************
 * ATS.C/h
 * 
 * This class represents a set of alignment templates (constructed by
 * reading ATS files).
 *
 * Author: Michel Galley (galley@cs.columbia.edu)
 * $Id: ATS.C,v 1.1.1.1 2006/03/05 09:20:26 mgalley Exp $
 ***************************************************************************/

#include <sstream>
#include <map>
#include <cassert>
#include <cstdlib>

#include "ATS.h"
#include "defs.h"

namespace mtrule {

  // Parse lines in the following format:
  // AT 2 2 8ÔÂ ¼ä # during august @ 1 0  %% 2 =model1inv= 7.03903
  void
  ATS::read_ATS_file(std::ifstream& is) {
	
	 int line_num = 0;
	 while(!is.eof()) {
	   ++line_num;
		std::string line,token;
		getline(is,line); 
		if(line == "") 
		  break;
		// Process ATS line:
		std::stringstream sstr(line);
		sstr >> token;
		if(token[0] == '#')
		  continue;
		assert(token == "AT");
		sstr >> token;
		int targetlen = atoi(token.c_str());
		sstr >> token;
		int sourcelen = atoi(token.c_str());
		// Create target-language phrase:
		Phrase *tp = new Phrase(targetlen);
		for(int i=0; i<targetlen; ++i) {
		  sstr >> token;
		  tp->add(token);
		  //std::cout << token << " ";
		}
		sstr >> token;
		assert(token == "#");
		//std::cout << " --> ";
		// Create source-language phrase:
		Phrase *sp = new Phrase(sourcelen);
		for(int i=0; i<sourcelen; ++i) {
		  sstr >> token;
		  sp->add(token);
		  //std::cout << token << " ";
		}
		// Skip bad phrase pairs:
		if(sp->el(0) == "@IMPOSSIBLE2@" || 
		   tp->el(0) == "@IMPOSSIBLE2@") {
		  // Cleanup and go to the next:
		  delete tp; 
		  delete sp;
		  continue;
		}
		// Read the remaining tokens until the AT probability is identified:
		double prob = -1;
      while(!sstr.eof()) {
		  sstr >> token;
		  if(token == AT_PROB_IDENTIFIER) {
		    sstr >> token;
		    prob = atof(token.c_str());
			 break;
		  }
      }
		// Skip if there is no prob:
		if(prob < 0)
		  continue;
		//std::cerr << "p=" << prob << std::endl;
		// Add pair to entries:
	   _entries.insert(ATentries::value_type(sp->el(0),
		                new PhrasePair(sp,tp,prob,line_num)));
		//std::cout << std::endl;
	 }
  }

  ATS::~ATS() {
    for(ATentries_it it = _entries.begin(), it_end = _entries.end();
	     it != it_end; ++it)
		delete it->second;
  }

}
