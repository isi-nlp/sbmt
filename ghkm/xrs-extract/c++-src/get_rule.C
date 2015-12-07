#include <iostream>
#include <string>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

#include "db_access.h"
#include "defs.h"

#define BUFFERSIZE 16384

// Extract a rule by its ID from the DB 
// (not yet implemented; need to change the
// format of the DB; see 
// http://www.sleepycat.com/docs/ref/am_conf/bt_recnum.html
// for more info).

// Warning in case you decide to transform the DB to allow 
// ruleID->rule_stirng lookups: 
// "Configuring a Btree for record numbers should not be done lightly. While 
// often useful, it may significantly slow down the speed at which items can 
// be stored into the database, and can severely impact application throughput."

char rule[BUFFERSIZE], lhs[BUFFERSIZE];

void usage() {
  std::cerr << "Usage: get_rule [OPTIONS]\n" 
            << "where OPTIONS are: \n"
				<< " -d <DB file> : rule DB file\n"
				<< " -i <rule ID> : ID of rule to query\n"
            << std::endl;
  exit(1);
}

void
get_rule(DB*& dbp, int ruleID) {
	char *data;
	int ret;
   // TODO:
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
	int ruleID;
	DB dbp;
	char *db_file;

   while((opt = getopt(argc,argv,"d:i:")) != -1)
     switch(opt) {
       case 'd':
         dbfile = optarg;
         break;
       case 'f':
         ruleID = atoi(optarg);
         break;
     }
	if(dbfile == NULL)
	  usage();

   db_init(dbp,true);
   set_db_cache_size(dbp,DB_CACHE_SIZE);
   set_db_params(dbp,DB_PAGE_SIZE,DB_EXPECTED_NB_OF_RULES,
	              DB_AVERAGE_RULE_SIZE,DB_AVERAGE_RULEID_SIZE);
   db_open(dbp, db_file, true);
	std::cout << get_rule(ruleID) << std::endl;
   db_close(dbp);
	return 0;

}
