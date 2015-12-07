#ifndef __RULEDESCRIPTOR_H__
#define __RULEDESCRIPTOR_H__

#include "hashutils.h"

namespace mtrule {

//! This class can encapsulate different descriptors of a rule
//! (i.e. all the stuff that comes after '###' in a rule file).
class RuleDescriptor {
  
public:

  std::string get_str_attribute(const std::string& name) const;
  double get_double_attribute(const std::string& name) const;
  int get_int_attribute(const std::string& name) const;

  void set_str_attribute(const std::string& name, const std::string& val) {
    _AVstring[name] = val;
  }
  void set_double_attribute(const std::string& name, double val) {
    _AVdouble[name] = val;
  }
  void set_int_attribute(const std::string& name, int val) {
    _AVint[name] = val;
  }

  std::string get_str(bool short_desc) const;

protected:

  //! Attribute-value pairs (values are string).
  StrHash    _AVstring;
  //! Attribute-value pairs (values are double).
  DoubleHash _AVdouble;
  //! Attribute-value pairs (values are int).
  IntHash _AVint;

  //! Determine if an attribute is minimal/important. This 
  //! generally applies to attributes whose values can't
  //! be determined by the rule itself (e.g. prob is minimal, 
  //! but nRHS is not)
  /*! I also removed some non-minimal stuff I no longer want
	*  to appear as descriptor, e.g. rule type and algo.
	*/
  bool is_minimal_descriptor(const std::string& d) const;
  bool is_wsd_descriptor(const std::string& d) const;
};

}

#endif
