#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <iostream>
#include <cstdlib>

using namespace  std;

// Simple program that prints the content of a DB to stdout.

#include "db_access.h"

#define BUFFERSIZE 16384

void usage() {
  std::cerr << "Usage: query_db -D <DB file> ids \n" 
            << "where: \n"
				<< "   -D <DB file> : database to print to stdout\n"
            << std::endl;
  exit(1);
}

void
query_db(char* dbfile, istream & in) {

   DB *dbp;
//	DBC *dbcp;
//	DBT key, data;
//	int ret;

   db_init(dbp);

	/* Turn on additional error output. */ 
	dbp->set_errfile(dbp, stderr); 

   // Open the dbfile database in read/write mode:
   db_open(dbp, dbfile, true);

   unsigned int id;
   while(in >> id){
	   ostringstream ost; 
	   ost << id;
	   const char* indstr = db_lookup(dbp,const_cast<char*>(ost.str().c_str()));
	   if(indstr){
		   cout<<indstr<<endl;
	   } else {
		   cout<<"CANNOT FIND "<<id<<endl;
	   }
   }
	// Close database:
   db_close(dbp);
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
   char *dbfile = NULL;
	//bool keys_only = false, data_only = false;
   while((opt = getopt(argc,argv,"D:")) != -1)
     switch(opt) {
       case 'D':
         dbfile = optarg;
         break;
     }
	if(dbfile == NULL)
	  usage();

	query_db(dbfile, cin);
	return 0;
}
