#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

// Membership test: read rules from STDIN 
// and print them to STDOUT if they are present
// (or are not) in the DB.

#include "WeightF.h"
#include "hashutils.h"
#include "db_access.h"

const char* const attr_delim  = " ###";

void usage() {
  std::cerr << "Usage: membership_test -d <DB file>\n" 
				<< " -D <DB file> : DB file\n"
            << "           -n : print if rule is not in DB.\n";  
  exit(1);
}

void
print_rules(char* db_file, std::istream& in, 
            std::ostream& out,bool print_if_not_in) {

   DB *dbp;
	DBT key, data;
	int ret;

   // Open the dbfile database in read mode:
   db_init(dbp,false);
   db_open(dbp, db_file, true);

   std::string linestr;
   while(!in.eof()) {
     getline(in,linestr);
     std::string rule = linestr;
     size_t pos = linestr.find(attr_delim);
     if(pos != std::string::npos) {
       rule = rule.substr(0,pos);
     }
     bool is_in = 
       (db_lookup(dbp,const_cast<char*>(rule.c_str())) != NULL);
     if(is_in ^ print_if_not_in)
       out << linestr << std::endl; 
   }

	// Close database:
   db_close(dbp);
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
   char *db_file = NULL;
   bool print_if_not_in=false;
   while((opt = getopt(argc,argv,"D:n")) != -1)
     switch(opt) {
       case 'D':
         db_file = optarg;
         break;
       case 'n':
         print_if_not_in = true;
         break;
     }
	if(db_file == NULL)
	  usage();

	print_rules(db_file,std::cin,std::cout,print_if_not_in);

	return 0;

}
