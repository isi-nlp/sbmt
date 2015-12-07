#include <sstream>
#include <iomanip>

#include "RuleDescriptor.h"
#include "defs.h"

namespace mtrule {

std::string 
RuleDescriptor::get_str_attribute(const std::string& name) const {
  StrHash_cit it = _AVstring.find(name);
  if(it == _AVstring.end())
	 return "";
  return it->second;
}

double 
RuleDescriptor::get_double_attribute(const std::string& name) const {
  DoubleHash_cit it = _AVdouble.find(name);
  if(it == _AVdouble.end())
	 return 0.0;
  return it->second;
}

int 
RuleDescriptor::get_int_attribute(const std::string& name) const {
  IntHash_cit it = _AVint.find(name);
  if(it == _AVint.end())
	 return 0;
  return it->second;
}

std::string 
RuleDescriptor::get_str(bool short_desc) const {
  std::stringstream ss;
  for(StrHash_cit it = _AVstring.begin(), it_end = _AVstring.end();
      it != it_end; ++it)
	 if(!short_desc || true || is_minimal_descriptor(it->first) || is_wsd_descriptor(it->first))
		ss << " " << it->first << "=" << it->second;
  for(IntHash_cit it = _AVint.begin(), it_end = _AVint.end();
      it != it_end; ++it)
	 if(!short_desc || true || is_minimal_descriptor(it->first) || is_wsd_descriptor(it->first))
		ss << " " << it->first << "=" << it->second;
  for(DoubleHash_cit it = _AVdouble.begin(), it_end = _AVdouble.end();
      it != it_end; ++it)
	 if(!short_desc || true || is_minimal_descriptor(it->first) || is_wsd_descriptor(it->first))
		ss << " " << it->first << "=" << std::setprecision(4) << it->second;
  return ss.str();
}

bool 
RuleDescriptor::is_minimal_descriptor(const std::string& d) const {
  if(d == "id") return true;
  if(d == SIZEID) return true;
  //if(d == "start") return true;
  //if(d == "end") return true;
  //if(d == "type") return true;
  //if(d == "fraccount") return true;
  if(d == "logp") return true;
  if(d == "p") return true;
  return false;
}

bool
RuleDescriptor::is_wsd_descriptor(const std::string& d) const {
  if(d == "WSD") return true;
  if(d == "sphrase") return true;
  if(d == "tphrase") return true;
  if(d == "lineNumber") return true;
  if(d == "align") return true;
  if(d == "srcb") return true;
  return false;
}


}


