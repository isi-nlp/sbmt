#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <cstdlib>

// Simple program that prints the content of a DB to stdout.

#include "db_access.h"

#define BUFFERSIZE 16384

void usage() {
  std::cerr << "Usage: dump_db -D <DB file> [-1] [-2]\n" 
            << "where: \n"
				<< "   -D <DB file> : database to print to stdout\n"
				<< "             -1 : print keys only\n"
				<< "             -2 : print data only\n"
            << std::endl;
  exit(1);
}

void
traverse_db(char* dbfile, bool keys_only, bool data_only) {

   DB *dbp;
	DBC *dbcp;
	DBT key, data;
	int ret;

   db_init(dbp);

	/* Turn on additional error output. */ 
	dbp->set_errfile(dbp, stderr); 

   // Open the dbfile database in read/write mode:
   db_open(dbp, dbfile, true);

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

   // Traverse the database:
   while ((ret = dbcp->c_get(dbcp, &key, &data, DB_NEXT)) == 0)  {
	  if(keys_only)
	 	 printf("%.*s\n", (int)key.size, (char *)key.data);
	  else if(data_only)
	 	 //printf("%.*d\n", (int)data.size, *((int*)data.data));
	 	 printf("%d\n",  *((int*)data.data));
	  else {
		 //unsigned int ruleID = atoi((char *)data.data);
		 unsigned int ruleID = *((int*)data.data);
	 	 //printf("%012d\t%.*s\n", ruleID, (int)key.size, (char*)key.data);
	 	 printf("%d\t%.*s\n", ruleID, (int)key.size, (char*)key.data);
	 	 //printf("key: %.*s\ndata: %.*s\n\n", 
		 // (int)key.size, (char *)key.data,
		 // (int)data.size, (char *)data.data);
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
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
   char *dbfile = NULL;
	bool keys_only = false, data_only = false;
   while((opt = getopt(argc,argv,"D:12")) != -1)
     switch(opt) {
       case 'D':
         dbfile = optarg;
         break;
       case '1':
         keys_only = true;
         break;
       case '2':
         data_only = true;
         break;
     }
	if(dbfile == NULL)
	  usage();

   // Create index db file: 
   traverse_db(dbfile,keys_only,data_only);
	return 0;

}
