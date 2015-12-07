#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

#include "defs.h"
#include "db_access.h"
#include "hashutils.h"
#include "boost/tokenizer.hpp"

#define BUFFERSIZE 16384

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

//struct eqint {
//  bool operator()(int s1, int s2) const {
//	 return s1 == s2;
//  }
//};
//hash_set< int, hash<int>, eqint > rules;

std::vector<bool> rules;
char rule[BUFFERSIZE], prefix[BUFFERSIZE], prevprefix[BUFFERSIZE];
char lhs_delim[]  = " -> ";
char root_delim[] = "(";
char header[]     = "$$$";
char comment[]    = "%";

void usage() {
  std::cerr << "Usage: print_norm_groups -D <DB file> [OPTIONS]\n" 
            << "where OPTIONS are: \n"
				<< "   -f <deriv file> : derivation file specifying which\n"
				<< "                     rules should be specified in the\n"
				<< "                     normalization groups.\n"
				<< "                -r : create root normalization groups\n"
				<< "                     instead of groups with same LHS\n.";
  exit(1);
}

void
extract_rules_from_deriv(char* deriv_file) {

  // Define separating symbols for tokenization:
  boost::char_separator<char> sep(" \t\n(){}@");

  std::ifstream deriv(deriv_file);
  if(deriv.fail()) {
    std::cerr << "Error opening file : " << deriv_file << std::endl;
    exit(1);
  }
  std::string tmp;
  while(!deriv.eof()) {
	 getline(deriv,tmp); 
	 if(tmp == "")
	   continue;
	 // Tokenize current line:
	 tokenizer tokens(tmp,sep);
	 // Process each token: 
	 for (tokenizer::iterator tok_iter = tokens.begin();
			tok_iter != tokens.end(); ++tok_iter) {
		bool is_digit = true;
		const std::string& str = *tok_iter;
		for(std::string::const_iterator it = str.begin(), 
		    it_end = str.end(); it != it_end; ++it) {
		  if(!isdigit(*it))
		    is_digit = false; 
		}
		if(is_digit && str.size() > 0) {
			if(atoi(str.c_str()) >= rules.size()) { 
				std::cerr<< "id " <<str<<" >= rule size"<<rules.size()<<std::endl;
			}
	     rules[atoi(str.c_str())] = true;
	     //rules.insert(atoi(str.c_str()));
		}
	 }
  }
  deriv.close();
}

void
get_constituent(const char* rule, const char* delim, char* out) {
  const char *pos = strstr(rule,delim);
  assert(pos != rule);
  if(pos == NULL) {
    std::cerr << "Not a valide rule: " << rule << std::endl;
  }
  assert(pos != NULL);
  int len=pos-rule;
  strncpy(out,rule,len);
  out[len] = '\0';
}

void
print_normalization_classes(const char* db_file, 
                            std::ostream& out,
                            bool lhs_norm, 
									 bool filter) {

   DB *dbp;
	DBC *dbcp;
	DBT key, data;
	int ret;
	char *delim = (lhs_norm) ? lhs_delim : root_delim;

   db_init(dbp,false);

   // Open the dbfile database in read/write mode:
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
   memset(&rule, 0, sizeof(rule));
   memset(&prefix, 0, sizeof(prefix));
   memset(&prevprefix, 0, sizeof(prevprefix));

   // Traverse the database:
	bool one_in_norm=false;
	printf("("); 
	//int count=0;
	// print current key/val:
	while((ret = dbcp->c_get(dbcp,&key,&data,DB_NEXT)) == 0) {
	  get_constituent((char *)key.data,delim,prefix);
	  if(strcmp(prefix,prevprefix) != 0) {
		 // Different prefix (lhs or root):
		 if(one_in_norm) { 
			printf(")\n");
		 }
		 one_in_norm=false;
	  }
	  strcpy(prevprefix,prefix);
	  if(filter) {
	    //if(rules.find(atoi((char*)data.data)) == rules.end()) {
	    //if(!rules[atoi((char*)data.data)]) {
	    if(!rules[*((int*)data.data)]) {
		   std::cerr << "WARNING: " <<*( (int*)data.data) 
			          << " not used in any dforest." << std::endl;
		   continue;
		 }
	  }
	  if(!one_in_norm) {
	    printf("(");
	    one_in_norm=true;
	  } else {
	    printf(" ");
	  }
	  //printf("%.*s", (int)data.size, (char *)data.data);
	  printf("%d", *((int*)data.data));
	}
	if(one_in_norm)
	  printf(")"); 
	printf(")\n"); 

	// Close database:
   db_close(dbp);
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
	bool lhs_norm = true;
   char *db_file = NULL, 
	     *deriv_file = NULL;
   while((opt = getopt(argc,argv,"D:f:r")) != -1)
     switch(opt) {
       case 'D':
         db_file = optarg;
         break;
       case 'f':
         deriv_file = optarg;
         break;
       case 'r':
         lhs_norm = false;
         break;
     }
	if(db_file == NULL)
	  usage();

   // Extract rules used in the derivation file:
   //rules.reserve(NB_RULES);
   rules.resize(NB_RULES);
	if(deriv_file != NULL)
	  extract_rules_from_deriv(deriv_file);

   // Create index db file: 
	print_normalization_classes(db_file,std::cout,lhs_norm,deriv_file != NULL);
	return 0;

}
