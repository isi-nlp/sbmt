#ifndef WeightFile
#define WeightFile

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <unistd.h>
#include <sys/resource.h>

namespace mtrule {

//! Class that simlifies the access to rule weight files (.weight
//! extension), which are meant to contain probabilities and counts.
/*! These files are compact, comma-separated data files. The semantics
 *  of each column in the file is specified in the header of the file.
 */
class WeightF {
  
  public:
    //! Default constructor.
	 WeightF() : _size(0) {}
    //! Default destructor.
	 ~WeightF();
    //! Read header, and store _ruleID_order, _fields, and _version. 
	 //! If 'check' is true, don't store anything, but check that the
	 //! header that is read matches data stored in current object.
	 //! If 'append' is set to true, do not erase existing fields 
	 //! (if there are any).
	 bool read_header(std::istream& in, bool check, bool append);
	 //! Returns true if headers of passed object are the same as current
	 //! object.
	 bool same_header(const WeightF& wf) const;
	 //! Print a header file.
    void print_header(std::ostream& out, bool skip_missing_data=false) const;	
	 //! Set number of rules.
	 void set_size(int size) { _size  = size; }
	 //! Get number of rules.
	 int get_size() const { return _size; }
	 //! Read weights off the file and store them in _data. Read all fields,
	 //! unless 'n' is non-negative, in which case only the n-th column
	 //! is stored. 
	 void read_weights(std::istream& in, int n = -1);
	 //! Print weights stored in memory in ruleID order.
	 void print_weights(std::ostream& out) const;
	 //! Print weights stored in memory in the order defined by a DB. 
	 void print_weights_DB_order(std::ostream& out, const char* db_file) const;
	 //! Appends a new field, and returns its index in the list of fields.
	 int add_field(const std::string& field_name);
	 int get_field_index(const std::string& field_name) const {
	   for(size_t i=0; i<_fields.size(); ++i)
		  if(_fields[i] == field_name) 
		    return i;
		return -1;
	 }
	 void set_field_data_reserve(int i, int size) { _data[i]->reserve(size); }
	 std::string get_field_name(int i) const { return _fields[i]; }
	 int get_num_field() const { return _fields.size(); }
	 void set_field_data(int field_index, int data_index, float v) { 
	   if(_size <= data_index) {
		  _size = data_index+1;
		  _data[field_index]->resize(_size);
		}
	   (*_data[field_index])[data_index] = v;
	 }
	 float get_field_data(int field_index, int data_index) const {
	   assert(field_index < static_cast<int>(_data.size()));
	   if(data_index  >= static_cast<int>(_data[field_index]->size())) {
		  std::cerr << "out of range: " << data_index << " " << _data[field_index]->size() 
		            << std::endl;
		}
	   return (*_data[field_index])[data_index];
	 }
	 bool is_ruleID_order() const { return _ruleID_order; }
	 void set_ruleID_order(bool o) { _ruleID_order=o; }

  protected:
	 //! If set to true, lines are ordered by ruleID numbers. Otherwise,
	 //! lines are ordered by 
	 bool _ruleID_order; 
    //! Name of the fields (1st element represent 1st column, etc)
	 std::vector<std::string> _fields;
	 //! Ptr to data structures encapsulating data in the file
	 //! (not all data is necessarily stored in _data; most of it
	 //!  might remain only on disk).
	 std::vector<std::vector<float>*> _data;
	 //! WeightFile version number:
	 float _version;
	 //! Number of rules:
	 int _size;

  public: 
    //! Header to skip when reading data.
	 const static char* const header;
    //! Comment to skip when reading data.
	 const static char* const comment;
};

}

// Sample weight file:
// $$ filetype=weight order=ruleIDs fields=count,fraccount version=0.1
// 1,1
// 1,1
// 1,1
// 36,36
// 1,1
// 1,1
// 1,1
// 2,2

#endif
