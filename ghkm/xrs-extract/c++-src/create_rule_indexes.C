#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

#include "db_access.h"

#define BUFFERSIZE 16384

using namespace db_access;

void usage() {
  std::cerr << "Usage: create_rule_indexes -d <DB file> [-i <startindex>] [-r]\n" 
            << "where: \n"
				<< "   -d <data file> : select a DB file\n"
				<< " -i <start index> : number used to start indexing rules that\n"
				<< "                    haven't yet been assigned an index\n"
				<< "               -r : read-only: do not modify the database,\n"
				<< "                    but just show what changes would happen\n"
				<< "               -f : force, i.e. ignore previous rule indexes\n"
            << std::endl;
  exit(1);
}

void
create_rule_indexes(char* dbfile, int startindex, bool readonly, bool force) {

   DB *dbp;
	DBC *dbcp;
	DBT key, data;
	int ret;
	int curindex = startindex;

   initdb(dbp);

   // Open the dbfile database in read/write mode:
   opendb(dbp, dbfile, readonly);

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
	  int index = atoi((char *)data.data);
	  if(index == -1 || force) {
	    // If set at -1, it means that the rule hasn't 
	    // yet been assigned an index:
	    index = curindex++;
		 printf("index of %.*s : %.*s -> %d\n", 
				 (int)key.size, (char *)key.data,
				 (int)data.size, (char *)data.data,index); 

		 if(!readonly) {
			std::stringstream ss(index);
			const char* indexstr = ss.str().c_str();
			data.data = (void *)indexstr;
			data.size = sizeof(indexstr);
			if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) != 0) {
			  std::cerr << "Error while storing new index in: " 
							<< dbfile << std::endl;
			  exit(1);
			}
		 }
	  }
   }
   if (ret != DB_NOTFOUND) { 
     dbp->err(dbp, ret, "DBcursor->get"); 
	  std::cerr << "Error while traversing the database: " 
	            << dbfile << std::endl;
	  exit(1);

   }

	// Close database:
   closedb(dbp);
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
	int startindex = 1;
	bool read_only = false,
	     force = false;
   char *dbfile = NULL;
   while((opt = getopt(argc,argv,"d:i:rf")) != -1)
     switch(opt) {
       case 'd':
         dbfile = optarg;
         break;
       case 'i':
         startindex = atoi(optarg);
         break;
       case 'r':
         read_only = true;
         break;
     }
	if(dbfile == NULL)
	  usage();

   // Create index db file: 
	create_rule_indexes(dbfile,startindex,read_only,force);
	return 0;

}
