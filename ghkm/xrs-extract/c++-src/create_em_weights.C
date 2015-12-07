#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <vector>
#include <cstdlib>

#include "WeightF.h"

void usage() {
  std::cerr << "Usage: create_em_weights -f <field name> -D <DB file> -e <em output>"
            << std::endl;
  exit(1);
}

// This code doesn't yet handle logs, so "-2.85561e+11ln" is 
// stored as "-2.85561e+11".
void
read_em_file(const char* em_file, mtrule::WeightF& wf, const char* field_name) {

  int field_index = wf.add_field(field_name);
  std::ifstream em_is(em_file);
  if(em_is.fail()) {
    std::cerr << "Error opening file : " << em_file << std::endl;
    exit(1);
  }
  wf.set_field_data(field_index,0,0); // there is no rule 0
  std::string tmp;
  int line=1;
  while(!em_is.eof()) {
	 getline(em_is,tmp); 
	 if(tmp == "(" || tmp == ")" || tmp == "")
	   continue;
	 // Process each token: 
	 std::stringstream ss(tmp);
	 float prob;
	 ss >> prob;
    wf.set_field_data(field_index,line,prob);
	 ++line;
  }
  em_is.close();
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
   char *db_file = NULL, 
	     *em_file = NULL,
		  *field_name = "em";
   while((opt = getopt(argc,argv,"D:e:f:")) != -1)
     switch(opt) {
       case 'D':
         db_file = optarg;
         break;
       case 'e':
         em_file = optarg;
         break;
       case 'f':
         field_name = optarg;
         break;
     }
	if(em_file == NULL)
	  usage();

   mtrule::WeightF wf;
	read_em_file(em_file,wf,field_name);
	if(db_file)
	  wf.set_ruleID_order(false);
	wf.print_header(std::cout);
	if(db_file)
	  wf.print_weights_DB_order(std::cout, db_file);
	else 
	  wf.print_weights(std::cout);

	return 0;

}
