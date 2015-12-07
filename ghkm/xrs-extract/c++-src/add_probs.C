#include <iostream>
#include <string>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <vector>
#include <cfloat>
#include <iomanip>
#include <cstdlib>

#include "WeightF.h"
#include "db_access.h"

// Print a probability file that 'extract' can use 
// to score derivations. If the probability is already
// available as a field of the input weight file, you just
// need to use '-1' to select the desired column. If not
// (you need to divide counts by normalization counts), 
// use -1 and -2 to select nominator and denominator.
// Probabilities are printed in rule ID order.

std::vector<float> _probs;
const char* const header  = "$$$";
const char* const comment = "%";

void usage() {
  std::cerr << "Usage: add_probs -w <weight_file> -D <DB file> -1 <nominator> [ -2 <denominator> ]\n";
  exit(1);
}

int main(int argc, char *argv[]) {

  // Scan arguments:
  int opt;
  char *weight_file = NULL,
       *db_file = NULL,
       *field_name1 = NULL,
       *field_name2 = NULL;
  while((opt = getopt(argc,argv,"D:w:1:2:")) != -1)
    switch(opt) {
     case 'w':
       weight_file = optarg;
       break;
     case 'D':
       db_file = optarg;
       break;
     case '1':
       field_name1 = optarg;
       break;
     case '2':
       field_name2 = optarg;
       break;
  }
  if(!weight_file || !db_file || !field_name1)
    usage();

  // Read all input files: 
  std::ifstream is(weight_file);
  if(is.fail()) {
    std::cerr << "Error opening file : " << weight_file << std::endl;
    exit(1);
  }
  mtrule::WeightF wf;
  wf.read_header(is,false,false);
  int index1 = wf.get_field_index(field_name1), 
      index2 = -1;
  if(field_name2 != NULL) 
    index2 = wf.get_field_index(field_name2);

  // Print probabilities in ruleID order: 
  int ret;
  DBT key, data;
  DB *dbp=NULL;
  DBC *dbcp=NULL;
	
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

  // Init data:
  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));

  // Compute all probs:
  std::string tmp;
  _probs.push_back(0);

  // Initialize the keys/data return pair:
  while((ret = dbcp->c_get(dbcp,&key,&data,DB_NEXT)) == 0) {
	 int ruleID = atoi((char*)data.data);
    getline(is,tmp); 

    if(tmp.length() == 0) 
      continue;
    if(strncmp(tmp.c_str(),header,strlen(header)) == 0 ||
      strncmp(tmp.c_str(),comment,strlen(comment)) == 0) {
      continue;
    }
    // Process each token: 
    size_t pos1=0, pos2=tmp.size()-1;
    unsigned int field_num = 0;
    double num=0, denom=DBL_MAX;
    while(true) {
      size_t pos = tmp.find(",",pos1);
      if(pos ==  std::string::npos)
        pos=pos2+1;
      float c  = atof(tmp.substr(pos1,pos-pos1).c_str());
      // Store data in memory:
      if(static_cast<int>(field_num) == index1)
        num = c;
      else if(static_cast<int>(field_num) == index2)
        denom = c;
      pos1 = pos+1;
      if(pos>=pos2)
        break;
      ++field_num;
    }
    if(ruleID >= static_cast<int>(_probs.size())) {
      _probs.resize(ruleID+1);
    }
    if(field_name2 != NULL)
      _probs[ruleID] = (num/denom);
    else 
      _probs[ruleID] = num;
  }

  // Close DB:
  if(db_file)
	 db_close(dbp);

  for(size_t i=1; i<_probs.size(); ++i)
    std::cout << std::setprecision(9) << _probs[i] << std::endl;

  return 0;
}
