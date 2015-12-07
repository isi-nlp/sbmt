#include <iostream>
#include <string>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <cstdlib>

#include "db_access.h"
#include "defs.h"

#define BUFFERSIZE 16384

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
				<< "           -f : force overwriting rules\n"
				<< "                (even those that have an index)\n" 
            << "           -i : read IDs (odd-numbered lines must\n"
            << "                contain rule strings; even-numbered\n"
            << "                must contain rule IDs\n"
			<< "           -r : create a db mapping from ID to rule string.\n"
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
	  in.getline(rule,BUFFERSIZE);
	  if(strlen(rule) == 0) 
	    continue;
     if(read_index)
       in.getline(ID,BUFFERSIZE);
	  if(strncmp(rule,header,strlen(header)) == 0 ||
		  strncmp(rule,comment,strlen(comment)) == 0) {
		  std::cerr << "Skipping: " << rule << std::endl;
	    continue;
	  }
	  ++count;
	  if(count % 10000 == 0) {
		  //std::cerr  <<++count<<std::endl;
	  }
	  data = NULL;
      idata = NULL;
	  if(!force) {
          if(reverse_db){
              std::cout<<rule<<std::endl;
             data = db_lookup(rdbp,rule);
             std::cout<<data<<std::endl;
          } else {
             idata = db_lookup_s2i(rdbp,rule);
          }
      }
      if(reverse_db){
          if(data == NULL) {
             if(read_index) {
                 std::cout<<"AAding: "<<ID<<std::endl;
                 db_insert(rdbp,ID, rule);
             }
           else {
             db_insert(rdbp,rule,"-1");
           }
          }
      } else {
          if(idata == NULL) {
             if(read_index) {
                 int nID = atoi(ID);
                 db_insert(rdbp,rule,nID);
             }
           else {
                 db_insert(rdbp,rule,-1);
           }
          }
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
   while((opt = getopt(argc,argv,"D:fir")) != -1)
     switch(opt) {
       case 'D':
         dbfile = optarg;
         break;
       case 'i':
         read_index = true;
         break;
       case 'f':
         force = true;
         break;
       case 'r':
         reverse_db = true;
         break;
     }
	if(dbfile == NULL)
	  usage();

   // Create index db file: 
	add_rules_to_db(dbfile,force,read_index,std::cin);
	return 0;

}
