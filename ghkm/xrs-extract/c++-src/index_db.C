#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <cstdlib>

#include "db_access.h"
#include "defs.h"

using namespace std;

#define BUFFERSIZE 16384

void usage() {
  std::cerr << "Usage: index_db -D <DB file> [-i <startindex>] [-r]\n" 
            << "where: \n"
				<< "        -d <DB file> : select a DB file\n"
				<< "    -i <start index> : number used to start indexing rules that\n"
				<< "                       haven't yet been assigned an index\n"
				<< " -r <reverse DB file>: create a reverse DB file, to map ruleIDs\n"
				<< "                       to rules.\n"
				<< "                  -f : force, i.e. ignore previous rule indexes\n"
            << std::endl;
  exit(1);
}

void
create_rule_indexes(char* dbfile, char* reverse_dbfile, int startindex, bool force) {

   DB *dbp, *rdbp;
	DBC *dbcp;
	DBT key, data;
	int ret;
	int curindex = startindex;

   // Init reverse_index DB: 
	if(reverse_dbfile != NULL) {
	  db_init(rdbp);
	  db_open(rdbp, reverse_dbfile, false);
	}
	
   db_init(dbp);
   set_db_cache_size(dbp,DB_CACHE_SIZE);
   set_db_params(dbp,DB_PAGE_SIZE,DB_NB_MIN_KEYS,DB_EXPECTED_NB_OF_RULES,
	              DB_AVERAGE_RULE_SIZE,DB_AVERAGE_RULEID_SIZE);

	/* Turn on additional error output. */ 
	dbp->set_errfile(dbp, stderr); 

   // Open the dbfile database in read/write mode:
   db_open(dbp, dbfile, false);

   // Acquire a cursor for the database: 
	if((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) { 
	  dbp->err(dbp, ret, "DB->cursor");
	  std::cerr << "Error acquiring cursor for the database: " 
	            << dbfile << std::endl;
	  exit(1);
	}

	// Initialize the key/data return pair: 
	memset(&key, 0, sizeof(key)); 
	memset(&data, 0, sizeof(data));

   // Traverse the database and assign an index to each entry:  
   while ((ret = dbcp->c_get(dbcp, &key, &data, DB_NEXT)) == 0)  {
     // Give unique identifier to rule:
	  //int index = atoi((char *)data.data);
	  int index = *((int*)data.data);
	  if(index == -1 || force) {
	    // If set at -1, it means that the rule hasn't 
	    // yet been assigned an index:
	    index = curindex++;
        if(curindex % 10000 == 0){
            cerr<<curindex<<endl;
        }
		 std::stringstream ss;
		 ss << index;
		 const char* indexstr = ss.str().c_str();
		 //data.data = (void *)indexstr;
		 data.data = (void *)(&index);
		 //data.size = strlen(indexstr)+1;
		 data.size = sizeof(int);
		 if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) != 0) {
			std::cerr << "Error while storing new index in: " 
						 << dbfile << std::endl;
			exit(1);
		 }
        //cerr<<(char*)key.data<<" :  "<<(char*)data.data<<" before:"<<endl;
        //cerr<<db_lookup(dbp, (char*)key.data)<<endl<<endl;
	  }
   }
   if (ret != DB_NOTFOUND) { 
     dbp->err(dbp, ret, "DBcursor->get"); 
	  std::cerr << "Error while traversing the database: " 
	            << dbfile << std::endl;
	  exit(1);

   }

	// Close database:
   db_close(dbp);
	if(reverse_dbfile != NULL)
	  db_close(rdbp);
	
	std::cerr << "End index: " << curindex << std::endl;
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
	int startindex = 1;
	bool force = false;
   char *dbfile = NULL, 
	     *reverse_dbfile = NULL;
   while((opt = getopt(argc,argv,"D:i:rfp")) != -1)
     switch(opt) {
       case 'D':
         dbfile = optarg;
         break;
       case 'i':
         startindex = atoi(optarg);
         break;
       case 'f':
         force = true;
         break;
       case 'r':
         reverse_dbfile = optarg;
         break;
     }
	if(dbfile == NULL)
	  usage();

   // Create index db file: 
	create_rule_indexes(dbfile,reverse_dbfile,startindex,force);
	return 0;

}
