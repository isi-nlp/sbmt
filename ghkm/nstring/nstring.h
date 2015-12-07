#ifndef __NUM_STRING_H__
#define __NUM_STRING_H__

#include <string>
#include <iterator>
#include <vector>

#if (__GNUC__ >= 3)
#include <ext/hash_map>
#if (__GNUC_MINOR__ >= 1)
using __gnu_cxx::hash;
using __gnu_cxx::hash_map;
#else
using std::hash;
using std::hash_map;
#endif
#else
#include <hash_map>
using std::hash;
using std::hash_map;
#endif

#ifndef CLOG
#define PRINTLOGS 2
#define CLOG(L) \
  if(PRINTLOGS >= 2) \
  std::clog<<__BASE_FILE__<<"("<<__LINE__<<"): "<<L<<std::endl
#endif

namespace num {

struct hash_str {
  size_t operator()(const std::string& s) const {
    return hash<char const *>()(s.c_str());
  }
};

//! This sting class implements a character string data type that is 
//! internally represented as a numeric value (i.e. a string represented
//! as an int).
/*! Supported numeric values are: char, short, int, or long int.
 *  The class encapsulates a static vector and a static hashtable
 *  implementing the index and reverse index needed to map
 *  the internal numeric representation to a sequence of characters 
 *  (std::string) and vice-versa.
 *  It is particularly useful in cases where the string comparison
 *  is the main string operation being performed, and where other
 *  operations like insert, erase, replace, find, and substr are not 
 *  (or seldom) used.
 */
template <class numT>
class _nstring {

protected:
    
  typedef hash_map<const std::string, numT, hash_str> str_hash_map;
  typedef size_t size_type;

  //! Numeric value that works as a unique identifier for the string
  //! within the class.
  numT _num;
  //! Index table needed to map a std::string (e.g. as it is used as 
  //! a constructor argument) to a unique string identifier.
  static str_hash_map _index;
  //! Reverse index table needed to map the numeric identifier of 
  //! a string back to its original std::string representation.
  static std::vector<std::string*> _reverse_index; 
  //! Highest index value used so far. 
  /*! This value is used to create new (and unique) string identifiers. 
	*/
  static numT _last_index;

  //! Get the index of a given std::string object. If the string 
  //! isn't yet present in the tables, give it a unique identifier 
  //! (index), then store it in the index table and reverse index
  //! table.
  numT _get_string_index(const std::string& str) {
	 typename str_hash_map::iterator it(_index.find(str));
	 if(it != _index.end()) {
	   return it->second;
	 }
	 std::string *new_str = new std::string(str);
	 ++_last_index;
	 _index[*new_str] = _last_index;
	 _reverse_index.push_back(new_str);
	 return _last_index;  
  }

public:

  /**********************************************************
	* The following member functions define a subset of the
	* interface of std::string.
	* What is currently left out: all functions that assume
	* that a string is a sequence, e.g. erase(), insert(), 
	* substr(), and iterators begin() and end().
   **********************************************************/

  //! Creates an empty string.
  _nstring() { 
    _num = _get_string_index("");
  }
  //! Initialization from a c string.
  _nstring(const char* cstr) {
    _num = _get_string_index(cstr);
  }
  //! Copy constructor.
  _nstring(const _nstring& str) {
    _num = str._num;
  }
  //! Initialization from a c string, using only the 
  //! first n characters.
  _nstring(const char* cstr, int len) {
    std::string str(cstr,len);
	 _num = _get_string_index(str);
  }
  //! Comparison the content of two _string objects.
  bool compare(const _nstring& rhs) const {
    return _num == rhs._num;
  }
  //! Return a pointer to a null-terminated array of characters
  //! representing the content of the string's content.
  const char* c_str() const { 
    return _reverse_index[_num]->c_str();
  }
  size_type find(const std::string& str, size_type pos = 0) const { 
    return find(str.data(), pos, str.length()); 
  }
  size_type find(const char* s, size_type pos, size_type n) const {
	 const std::string* str = _reverse_index[_num];
    return str->find(s, pos, n); 
  }
  size_type find(char c, size_type pos = 0) const {
	 const std::string* str = _reverse_index[_num];
    return str->find(c, pos);
  }
  //! Print the string to a stream.
  std::ostream& operator<<(std::ostream& os) const {
	 return os.operator<<(_reverse_index[_num]->c_str());
	 //return os.operator<<(_reverse_index[_num]);
  }
  //! Read a string from a stream.
  std::istream& operator>>(std::istream& is) {
	 std::string str;
	 is >> str;
	 _num = _get_string_index(str);
	 return is;
  }

  /**********************************************************
	* The following member functions are additions to the
	* interface of std::string:
   **********************************************************/

  //! Constructor that takes a std::string as argument.
  _nstring(const std::string& str) { 
    _num = _get_string_index(str.c_str());
  }
  //! Clear content of _index and _reverse_index:
  static void _clear_tables() {
    for(int i=0, size=_reverse_index.size(); i<size; ++i)
	   delete _reverse_index[i];
	 _index.clear();
	 _reverse_index.clear();
  }
  //! Comparison the content of a _string with a std::string.
  /*! If performance is a major issue, avoid using this function
	*  if you can.
	*/
  bool compare(const std::string& rhs) const {
	 typename str_hash_map::const_iterator it = _index.find(rhs);
	 if(it == _index.end())
	   return false;
	 return (_num == it->second);
  }
  //! Return a reference to the std::string represented by num::_nstring.
  const std::string& str() const {
    return _reverse_index[_num];
  }
  //! Return the numeric value internal to the string.
  numT get_num() const { return _num; }
 
}; // class base_string

/**********************************************************
 * Static members specific to _nstring and independent of
 * the std::string interface.
 **********************************************************/

//! Definition of the static member _last_index 
//! (see declaration in the body of the class).
template<class numT> numT _nstring<numT>::_last_index = -1;
//! Definition of the static member _index
//! (see declaration in the body of the class).
template <class numT> typename _nstring<numT>::str_hash_map 
  _nstring<numT>::_index;
//! Definition of the static member _reverse_index
//! (see declaration in the body of the class).
template <class numT> std::vector<std::string*> 
  _nstring<numT>::_reverse_index;

/**********************************************************
 * Static members mimicing the std::string interface.
 **********************************************************/

//! Comparison operator: two _nstring<numT>.
template<class numT>
inline bool operator==(const _nstring<numT>& lhs, const _nstring<numT>& rhs) {
  return lhs.compare(rhs);
}
//! Returns true if the specified _nstring<numT> and std::string objects
//! represent the same string.
template<class numT>
inline bool operator==(const _nstring<numT>& lhs, const std::string& rhs) {
  return lhs.compare(rhs);
}
//! Returns true if the specified _nstring<numT> and std::string objects
//! represent different strings.
template<class numT>
inline bool operator!=(const _nstring<numT>& lhs, const std::string& rhs) {
  return !lhs.compare(rhs);
}
//! Read an object from a stream.
template <class numT> std::istream&
operator>>(std::istream& is, num::_nstring<numT>& nstr) {
  return nstr.operator>>(is);
}
//! Print an object to a stream.
template <class numT> std::ostream&
operator<<(std::ostream& os, const num::_nstring<numT>& nstr) {
  return nstr.operator<<(os);
}

//! String class using int's.
typedef _nstring<unsigned char>  cstring;
typedef _nstring<unsigned short> sstring;
typedef _nstring<unsigned int>   istring;
typedef _nstring<unsigned long>  lstring;

#undef CLOG

} // namespace num
#endif
