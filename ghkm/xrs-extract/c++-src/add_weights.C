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

// Adds weights (generally counts) of one or more files. All files are 
// expected to define the same weights (e.g. "count" in the first column, 
// "fraccount" in the second). Weights are simply added, and 
// printed to std output:
//
// e.g. input:
// 2,0.5                     3,0.5
// output:
// 5,1.0

void usage() {
  std::cerr << "Usage: add_weights [-f <field name>] [-D <DB file>] "
            << "<file1> [<file2> [... [<fileN>]]]" << std::endl
				<< "If a DB file is specified, rule counts are printed in the order"
				<< "specified in the DB." << std::endl;
  exit(1);
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
   char *db_file = NULL,
	     *field_name = NULL;
   while((opt = getopt(argc,argv,"D:f:")) != -1)
     switch(opt) {
       case 'f':
         field_name = optarg;
         break;
       case 'D':
         db_file = optarg;
         break;
     }
	if(argc < 2)
	  usage();

	// Read all input files: 
   mtrule::WeightF wf;
	int field_index = -1;
   bool first_file=true;
	for(int i=1; i<argc; ++i) {
	  if(strcmp(argv[i],"-D") == 0 ||
	     strcmp(argv[i],"-f") == 0) {
	    ++i;
		 continue;
	  }
	  char* count_file = argv[i];
	  std::cerr << "reading: " << count_file << std::endl;
	  std::ifstream count_is(count_file);
	  if(count_is.fail()) {
		  std::cerr << "Error opening file : " << count_file << std::endl;
		  exit(1);
	  }
	  if(first_file) {
	    wf.read_header(count_is,false,false);
		 if(field_name) {
			field_index = wf.get_field_index(field_name);
	      wf.set_field_data_reserve(field_index, 150*1024*1024);
			std::cerr << "field index: " << field_index << std::endl;
	    }
	  } else
		 assert(wf.read_header(count_is,true,false));
	  wf.read_weights(count_is,field_index);
	  count_is.close();
	  first_file = false;
	}

	// Print new file:
	if(db_file != NULL)
	  wf.set_ruleID_order(false);
	wf.print_header(std::cout,true);
	if(db_file != NULL)
	  wf.print_weights_DB_order(std::cout,db_file);
	else
	  wf.print_weights(std::cout);  
	return 0;
}
