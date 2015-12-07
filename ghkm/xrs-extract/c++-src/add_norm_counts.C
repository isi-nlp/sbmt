#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

// Compute probabilities from counts.

#include "WeightF.h"
#include "hashutils.h"
#include "db_access.h"
#include "defs.h"

#define BUFFERSIZE (524288*2)
#define ROOTBUFFERSIZE 128

typedef hash_map<const std::string,double,hash_str> DoubleHash;
DoubleHash rootcount;
char rule[BUFFERSIZE],lhs[BUFFERSIZE],prevlhs[BUFFERSIZE],root[ROOTBUFFERSIZE];
const char* const lhs_delim  = " -> ";
const char* const root_delim = "(";
const char* const attr_delim = "###";
const char* const header  = "$$$";
const char* const comment = "%";

void usage() {
  std::cerr << "Usage: add_norm_counts -d <DB file> -w <weight file> -f <field name>\n" 
				<< "     -D <DB file> : DB file to read\n"
				<< " -w <weight file> : weight file containing counts to produce\n"
				<< "                    normalization group probabilities.\n"
				<< "  -f <field name> : field to for which normalization counts will be\n"
				<< "                    created; if field name is X, then\n"
				<< "                    a new field X_root and X_lhs will be added\n"
				<< "        -t <size> : threshold on number of internal nodes\n"
				<< "               -L : skip LHS normalization counts\n";
  exit(1);
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
  assert(len <= BUFFERSIZE);
  strncpy(out,rule,len);
  out[len] = '\0';
}

bool
above_threshold(const std::string& rulestr, int thres) {
	size_t pos = rulestr.find_last_of(attr_delim);
	if(pos == std::string::npos) {
		assert(false);
		return false;
	}
	size_t pos1 = rulestr.find(std::string(SIZEID)+"=",pos+1);
	if(pos1 == std::string::npos) {
		assert(false);
	  return false;
	}
	size_t pos2 = rulestr.find(" ",pos1+1);
	if(pos2 == std::string::npos) 
		pos2 = rulestr.size()-1;
	std::string intstr = rulestr.substr(pos1+sizeof(SIZEID),pos2-pos1-sizeof(SIZEID)+1);
	//std::cerr << "size: " << intstr << " thres: "  << thres << "\n";
	int size = atoi(intstr.c_str());
	return (size > thres);
}

void
add_norm_counts(mtrule::WeightF& wf, char* db_file, 
                std::istream& in, std::ostream& out,
                int column_to_normalize, int thres, bool dolhs) {

	std::cerr << "Adding normalization counts for: " 
	          << column_to_normalize << std::endl;

   DB *dbp;
	DBC *dbcp1, *dbcp2, *dbcp3;
	DBT key, data;
	int ret;

   // Open the dbfile database in read mode:
   db_init(dbp,false);
   db_open(dbp, db_file, true);

   // Acquire a cursor for the database: 
	if((ret = dbp->cursor(dbp, NULL, &dbcp1, 0)) != 0) { 
	  dbp->err(dbp, ret, "DB->cursor");
	  std::cerr << "Error acquiring cursor for the database: " 
	            << db_file << std::endl;
	  exit(1);
	}

   // Initialize the keys/data return pair:
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
   memset(&rule, 0, sizeof(rule));
   memset(&lhs, 0, sizeof(lhs));
   memset(&prevlhs, 0, sizeof(prevlhs));
   memset(&root, 0, sizeof(root));

   // Traverse the database (pass one to get rootcounts and 
	// rootfraccounts, and to set count of above-threshold rules
	// to 0):
	std::cerr << "Pass 1." << std::endl;
	dbcp1->c_dup(dbcp1,&dbcp2,DB_POSITION);
	dbcp1->c_dup(dbcp1,&dbcp3,DB_POSITION);
	int entry_ID=1;
	while((ret = dbcp1->c_get(dbcp1,&key,&data,DB_NEXT)) == 0) {
	  // Get rule:
     std::string rulestr((char *)key.data);
	  // Get root of constituent:
	  get_constituent(rulestr.c_str(),root_delim,root);
	  // If rule is too big, set its count to 0:
	  if(above_threshold(rulestr,thres))
	    wf.set_field_data(column_to_normalize,entry_ID,0.0);
	  // Add rule count to root count:
	  rootcount[root] += wf.get_field_data(column_to_normalize,entry_ID);
	  ++entry_ID;
	}
	dbcp1->c_close(dbcp1);
	entry_ID=1;
	std::cerr << "Pass 2." << std::endl;
	do {
	  int tmp_entry_ID = entry_ID; 
	  double lhscount=0.0;
	  bool first_iter=true;
	  dbcp3->c_close(dbcp3);
	  dbcp2->c_dup(dbcp2,&dbcp3,DB_POSITION);
	  if(dolhs) {
		 while((ret = dbcp3->c_get(dbcp3,&key,&data,DB_NEXT)) == 0) {
			get_constituent((char *)key.data,lhs_delim,lhs);
			if(strcmp(prevlhs,lhs) != 0 && !first_iter)
			  break;
			first_iter = false;
			lhscount += wf.get_field_data(column_to_normalize,tmp_entry_ID);
			strcpy(prevlhs,lhs);
			++tmp_entry_ID;
		 }
	  }
	  std::string tmp;
	  while((ret = dbcp2->c_get(dbcp2,&key,&data,DB_NEXT)) == 0) {
		 //int ruleID = atoi((char*)data.data);
		 get_constituent((char *)key.data,root_delim,root);
		 //do { getline(in,tmp); } 
		 //while(strncmp(tmp.c_str(),header,strlen(header))   == 0 ||
		 //      strncmp(tmp.c_str(),comment,strlen(comment)) == 0);
		 double w = wf.get_field_data(column_to_normalize,entry_ID);
		 if(dolhs)
			printf("%.8g,%.8g,%.8g\n", w,lhscount,rootcount[root]);
			//printf("%s,%.8g,%.8g\n", tmp.c_str(),lhscount,rootcount[root]);
		 else
			printf("%.8g,%.8g\n", w,rootcount[root]);
			//printf("%s,%.8g\n", tmp.c_str(),rootcount[root]);
		 if(entry_ID == tmp_entry_ID-1) {
		   break;
		 }
		 ++entry_ID;
	  } 
	  ++entry_ID;
	} while(ret == 0);

	// Close database:
	dbcp2->c_close(dbcp2);
	dbcp3->c_close(dbcp3);
   db_close(dbp);
	std::cerr << "Done adding normalization counts." << std::endl; 
}

int main(int argc, char *argv[]) {

   // Scan arguments:
   int opt;
   char *db_file = NULL, 
	     *weight_file = NULL;
   std::string field_name;
	int thres = 1000;
	bool lhs = true;
   while((opt = getopt(argc,argv,"D:w:f:t:L")) != -1)
     switch(opt) {
       case 'D':
         db_file = optarg;
         break;
       case 'w':
         weight_file = optarg;
         break;
       case 'f':
         field_name = optarg;
         break;
       case 't':
         thres = atoi(optarg);
         break;
       case 'L':
         lhs = false;
         break;
     }
	if(db_file == NULL || weight_file == NULL || field_name == "")
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
	int column_to_normalize = wf.get_field_index(field_name);
	if(lhs)
	  wf.add_field("lhs" + field_name);
	wf.add_field("root" + field_name);
	wf.print_header(std::cout);
	wf.read_weights(weight_is,column_to_normalize);
	//weight_is.clear();
	//weight_is.seekg(0,std::ios::beg);
   //std::string tmp;
	//getline(weight_is,tmp);
	std::cerr << "threshold: " << thres << "\n";
	add_norm_counts(wf,db_file,weight_is,std::cout,column_to_normalize,thres,lhs);
	weight_is.close();

	return 0;
}
