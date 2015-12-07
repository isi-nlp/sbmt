#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

// Print rules with attributes read from a .weight file.

#include "WeightF.h"
#include "hashutils.h"
#include "db_access.h"

const char* const lhs_delim  = " -> ";
const char* const root_delim = "(";
const char* const header  = "$$$";
const char* const comment = "%";


void usage() {
  std::cerr << "Usage: print_rules -d <DB file> -w <weight file> -f <field name>\n" 
            << "    -D <DB file>  : DB file\n"
            << " -w <weight file> : attributes to print with rules.\n";
  exit(1);
}

void
print_rules(mtrule::WeightF& wf, char* db_file, 
            std::istream& in, std::ostream& out) {

   DB *dbp;
   DBC *dbcp;
   DBT key, data;
   int ret;

   // Open the dbfile database in read mode:
   db_init(dbp,false);
   db_open(dbp, db_file, true);

   // Acquire a cursor for the database: 
   if((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) { 
     dbp->err(dbp, ret, "DB->cursor");
     std::cerr << "Error acquiring cursor for the database: " 
               << db_file << std::endl;
     exit(1);
   }

   // Initialize the keys/data return pair:
   memset(&key, 0, sizeof(key));
   memset(&data, 0, sizeof(data));

   // Make sure that count comes first:
	const std::string& firstname = wf.get_field_name(0);
	assert(firstname == "count");

   // Traverse DB and print keys + weights:
   std::string tmp;
   int line=1;
   while((ret = dbcp->c_get(dbcp,&key,&data,DB_NEXT)) == 0) {
	  // Get to next line in count file:
     do {
       getline(in,tmp);
     } while(strncmp(tmp.c_str(),header,strlen(header)) == 0 ||
             strncmp(tmp.c_str(),comment,strlen(comment)) == 0);
     // Process each comma-separated element:
     size_t pos1=0, pos2=tmp.size()-1;
     unsigned int field_num = 0;
     while(true) {
       size_t pos = tmp.find(",",pos1);
       if(pos ==  std::string::npos)
         pos=pos2+1;
       const std::string& name = wf.get_field_name(field_num);
       float c  = atof(tmp.substr(pos1,pos-pos1).c_str());
		 if(field_num == 0) {
		 	// Skip if count is zero:
		 	if(c == 0)
				break;
			// Print rule with id:
            //printf("%.*s id=%.*s", (int)key.size, (char *)key.data,
			//							(int)data.size, (char *)data.data);
            printf("%.*s id=%d", (int)key.size, (char *)key.data,
										*((int*)data.data));
		 }
       printf(" %s=%.8g", name.c_str(),c);
       pos1 = pos+1;
       if(pos>=pos2)
         break;
       ++field_num;
     }
     ++line;
     printf("\n");
   } while(ret == 0);

   // Close database:
   db_close(dbp);
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
   char *db_file = NULL, 
        *weight_file = NULL;
   while((opt = getopt(argc,argv,"D:w:")) != -1)
     switch(opt) {
       case 'D':
         db_file = optarg;
         break;
       case 'w':
         weight_file = optarg;
         break;
     }
   if(db_file == NULL || weight_file == NULL)
     usage();

   // Read counts, fraccounts, and EM probs:
   std::ifstream weight_is(weight_file);
   if(weight_is.fail()) {
     std::cerr << "Error opening file : " << weight_file << std::endl;
     exit(1);
   }

   mtrule::WeightF wf;
   wf.read_header(weight_is,false,false);
   assert(!wf.is_ruleID_order());
   std::cout << "$$$ filetype=rule version=1.0" << std::endl;
   print_rules(wf,db_file,weight_is,std::cout);
   weight_is.close();

   return 0;

}
