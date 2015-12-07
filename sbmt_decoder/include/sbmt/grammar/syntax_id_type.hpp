# ifndef SBMT__SYNTAX_ID_TYPE_HPP
# define SBMT__SYNTAX_ID_TYPE_HPP

# include <boost/cstdint.hpp>
# include <boost/integer_traits.hpp>
# include <graehl/shared/is_null.hpp>

namespace sbmt {

typedef boost::int64_t big_syntax_id_type;    
typedef boost::int32_t old_syntax_id_type;
typedef big_syntax_id_type syntax_id_type;

// -9,223,372,036,854,775,808
// or as my advisor would say, "bah. small integers" --michael
static const syntax_id_type NULL_SYNTAX_ID=boost::integer_traits<syntax_id_type>::const_min;  

/*
  //TODO: not safe until syntax_id_type is its own type and not typedef
inline void is_null(syntax_id_type i)
{ return i==NULL_SYNTAX_ID; }

inline void set_null(syntax_id_type &i)
{ i = NULL_SYNTAX_ID; }
*/

}

# endif
