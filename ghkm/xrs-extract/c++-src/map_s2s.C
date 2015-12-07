#include <iostream>
#include <string>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

#include "db_access.h"
#include "defs.h"

#define BUFFERSIZE 1638400

char rule[BUFFERSIZE], ID[BUFFERSIZE];
char delim[] = " -> ";
char header[] = "$$$";
//char comment[] = "% ";
char comment[] = "%%%";
bool reverse_db = false;

void usage() {
  std::cerr << "Usage: create_db [OPTIONS]\n" 
            << "where OPTIONS are: \n"
				<< " -D <DB file> : the DB file to create\n"
            << std::endl;
  exit(1);
}

void
add_rules_to_db(const std::string& dbfile, bool force, bool read_index,
                std::istream& in) {

   DB *rdbp;
	char *data;
    int *idata;

   db_init(rdbp,false);
   set_db_params(rdbp,DB_PAGE_SIZE,DB_NB_MIN_KEYS,DB_EXPECTED_NB_OF_RULES,
	              DB_AVERAGE_RULE_SIZE,DB_AVERAGE_RULEID_SIZE);

   // Open the dbfile database in read/write mode:
	std::cerr << "Creating DB..." << std::endl;
	std::string rules_db(dbfile);
   db_open(rdbp, rules_db.c_str(), false);

   int count=0;
   while(!in.eof()) {
      // get the first string.
	  in.getline(rule,BUFFERSIZE);
	  if(strlen(rule) == 0) 
	    continue;
      // get the second string.
       in.getline(ID,BUFFERSIZE);
	  ++count;
	  if(count % 10000 == 0) {
		  //std::cerr  <<++count<<std::endl;
	  }
	  data = NULL;
      data = db_lookup(rdbp,rule);
      if(data == NULL) {
         db_insert(rdbp,rule,ID);
       }
	}

	// Close database:
	std::cerr << "Closing DB." << std::endl;
   db_close(rdbp);
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
   char *dbfile = NULL;
	bool force =false,
	     read_index = false;
   while((opt = getopt(argc,argv,"D:i")) != -1)
     switch(opt) {
       case 'D':
         dbfile = optarg;
         break;
       case 'i':
         read_index = true;
         break;
     }
	if(dbfile == NULL)
	  usage();

   // Create index db file: 
	add_rules_to_db(dbfile,force,read_index,std::cin);
	return 0;

}
