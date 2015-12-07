#include <iomanip>
#include <cstdlib>

#include "WeightF.h"
#include "MyErr.h"
#include "db_access.h"

namespace mtrule {

const char* const WeightF::header = "$$$";
const char* const WeightF::comment = "%";

WeightF::~WeightF() {
  for(int i=0, size=_data.size(); i<size; ++i)
    delete _data[i];
}

bool 
WeightF::read_header(std::istream& in, bool check, bool append) {

  // Just read one line from 'in':
  std::string tmp;
  getline(in,tmp); 

  // Find 'fields=':
  size_t pos1 = tmp.find("fields="); 
  size_t pos2 = tmp.find(" ",pos1+7); 
  std::string fields = tmp.substr(pos1+7,pos2-pos1-7);

  // Find 'order=' field:
  size_t pos3 = tmp.find("order=");
  size_t pos4 = tmp.find(" ",pos3+6);
  std::string order = tmp.substr(pos3+6,pos4-pos3-6); 
  _ruleID_order = (order == "ruleIDs");
 
  // Scan the fields section:
  pos1=0;
  pos2=fields.size()-1;
  int field_num = (append) ? _fields.size() : 0;
  while(true) { 
	 size_t pos = fields.find(",",pos1); 
	 if(pos ==  std::string::npos)
	   pos=pos2+1;
	 std::string field = fields.substr(pos1,pos-pos1);
	 if(check) {
		if(_fields[field_num] != field) {
		  *myerr << "Incorrect fields: " 
		            << field << " != "
						<< _fields[field_num] << std::endl;
		  return false;
		}
	 } else {
		if(static_cast<int>(_fields.size()) <= field_num) {
		  _fields.resize(field_num+1);
		  _data.resize(field_num+1);
		}
		_fields[field_num] = field;
		_data[field_num] = new std::vector<float>;
	 }
	 pos1 = pos+1;
	 if(pos>=pos2)
	   break;
	 ++field_num;
  }
  return true;
}

bool 
WeightF::same_header(const WeightF& wf) const {
  if(_fields.size() != wf._fields.size())
    return false;
  for(int i=0, size=_fields.size(); i<size; ++i)
    if(_fields[i] != wf._fields[i])
	   return false;
  return true;
}

void
WeightF::print_header(std::ostream& out, bool skip_missing_data) const {
  const char* order = (_ruleID_order) ? "ruleIDs" : "DB";
  out << "$$$ filetype=weight order=" << order << " fields=";
  bool first=true;
  for(size_t i=0; i<_fields.size(); ++i) {
    if(skip_missing_data && _data[i]->size() == 0)
	   continue;
    if(first) { first = false; }
	 else { out << ","; }
    out << _fields[i];
  }
  out << " version=0.1" << std::endl;
}

void
WeightF::read_weights(std::istream& in, int n) {
  int line=1;
  std::string tmp;
  while(!in.eof()) {
	 getline(in,tmp); 

	 if(tmp.length() == 0) 
	   continue;
	 if(strncmp(tmp.c_str(),header,strlen(header)) == 0 ||
		strncmp(tmp.c_str(),comment,strlen(comment)) == 0) {
	   continue;
	 }
	 // Process each token: 
	 size_t pos1=0, pos2=tmp.size()-1;
	 unsigned int field_num = 0;
	 while(true) {
		size_t pos = tmp.find(",",pos1);
		if(pos ==  std::string::npos)
		  pos=pos2+1;
		float c  = atof(tmp.substr(pos1,pos-pos1).c_str());
		if(n < 0 || static_cast<int>(field_num) == n) {
		  // Store data in memory:
		  std::vector<float>& cur_vect = *_data[field_num];
		  if(line >= static_cast<int>(cur_vect.size())) {
			 cur_vect.resize(line+1);
		  }
		  cur_vect[line] += c;
		}
		pos1 = pos+1;
		if(pos>=pos2)
		  break;
		++field_num;
    }
	 ++line;
	 if(_size<line)  {
		_size = line;
	 }
  }
  *myerr << "Done reading weights. Size: " << _size << std::endl;
}

// Print merged counts (ordering: ruleID)
void
WeightF::print_weights(std::ostream& out) const {
  *myerr << "printing rule weights..." << std::endl;
  for(int i=1; i<_size; ++i) {
    bool first=true;
    for(int j=0, size2=_data.size(); j<size2; ++j) {
	   if(_data[j]->size() <= 1)
		  continue;
	   if(first) { first = false; }
		else      { out << ","; }
		out << std::setprecision(8) << (*_data[j])[i];
	 }
	 out << "\n";
  }
}

// Print merged counts (ordering: as found in DB)
void
WeightF::print_weights_DB_order(std::ostream& out, const char* db_file) const {

  *myerr << "printing rule weights..." << std::endl;

  // Check if counts should be printed in DB order:
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
	 *myerr << "Error acquiring cursor for the database: " 
				 << db_file << std::endl;
	 exit(1);
  }
 
  // Initialize the keys/data return pair:
  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));
  while((ret = dbcp->c_get(dbcp,&key,&data,DB_NEXT)) == 0) {
	 int ruleID = atoi((char*)data.data);
    bool first=true;
	 for(int j=0, size2=_data.size(); j<size2; ++j) {
	   if(_data[j]->size() <= 1)
		  continue;
	   if(first) { first = false; }
		else      { out << ","; }
		if(static_cast<int>(_data[j]->size()) <= ruleID) {
		  *myerr << "index out of bounds: " << _data[j]->size() 
		            << " <= " << ruleID << std::endl;
		  std::exit(1);
		}
	   out << std::setprecision(8) << (*_data[j])[ruleID];
	 }
 	 out << "\n";
  }

  // Close DB:
  if(db_file)
	 db_close(dbp);
}

int 
WeightF::add_field(const std::string& field_name) {
  _fields.push_back(field_name);
  _data.push_back(new std::vector<float>);
  return _data.size()-1;
}

}
