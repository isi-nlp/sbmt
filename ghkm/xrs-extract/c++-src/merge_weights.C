#include <iostream>
#include <string>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <vector>
#include <cstdlib>
#include <cstring>

#include "WeightF.h"

// Merge weights of different files. File names in those files should all
// be different.

const char* const header  = "$$$";
const char* const comment = "%";

void usage() {
  std::cerr << "Usage: merge_weights <file1> [<file2> [... [<fileN>]]]"
            << std::endl;
  exit(1);
}

int main(int argc, char *argv[]) {

   // Scan arguments:
	if(argc < 2)
	  usage();

	// Read all input files: 
   mtrule::WeightF wf;
   bool first_file=true;
	std::vector<std::ifstream*> iss;
	for(int i=1; i<argc; ++i) {
	  if(strcmp(argv[i],"-d") == 0) {
	    ++i;
		 continue;
	  }
	  char* weight_file = argv[i];
	  std::cerr << "reading: " << weight_file << std::endl;
	  std::ifstream *weight_is = new std::ifstream(weight_file);
	  if(weight_is->fail()) {
		  std::cerr << "Error opening file : " << weight_file << std::endl;
		  exit(1);
	  }
	  wf.read_header(*weight_is,false,!first_file);
	  first_file = false;
	  iss.push_back(weight_is);
	}

	// Print merged weights:
	bool not_all_eof;
	wf.print_header(std::cout);
	while(true) {
	  not_all_eof=false;
	  std::string tmp;
	  for(int i=0, size=iss.size(); i<size; ++i) {
	    if(!iss[i]->eof()) {
		   do {
			  getline(*iss[i],tmp);
			} while(strncmp(tmp.c_str(),header,strlen(header)) == 0 ||
					  strncmp(tmp.c_str(),comment,strlen(comment)) == 0);
			if(tmp == "")
			  continue;
			not_all_eof=true;
			if(i>0) 
			  std::cout << ",";
			std::cout << tmp;
		 }
	  }
	  if(!not_all_eof)
	    break;
	  std::cout << "\n";
	}

   // Final cleanup:
	for(int i=0, size=iss.size(); i<size; ++i) {
	  iss[i]->close();
	  delete iss[i];
	}
	return 0;
}
