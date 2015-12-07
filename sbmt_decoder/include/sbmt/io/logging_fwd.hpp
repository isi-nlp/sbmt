#ifndef   SBMT_IO_LOGGING_FWD_HPP
#define   SBMT_IO_LOGGING_FWD_HPP

#include <boost/operators.hpp>
#include <cassert>

#define SBMT_INHERIT_LEVEL 8
#define SBMT_PEDANTIC_LEVEL 7
#define SBMT_DEBUG_LEVEL 6
#define SBMT_VERBOSE_LEVEL 5
#define SBMT_INFO_LEVEL 4
#define SBMT_TERSE_LEVEL 3
#define SBMT_WARNING_LEVEL 2
#define SBMT_ERROR_LEVEL 1
#define SBMT_NEVER_LOG 0
#define SBMT_MOST_LOGGING_LEVEL SBMT_PEDANTIC_LEVEL
#define SBMT_LEAST_LOGGING_LEVEL SBMT_NEVER_LOG

namespace sbmt { namespace io {

namespace detail {

class logging_public_stream;

class logging_finish_stream;

} // namespace detail

class logging_stream;

class logging_level_manip;

class formatted_logging_msg;

detail::logging_finish_stream& endmsg(detail::logging_public_stream& log);

////////////////////////////////////////////////////////////////////////////////

class logging_level : public boost::totally_ordered<logging_level>
//,private boost::addable<logging_level,int>,private boost::subtractable<logging_level,int>
{
public:
    enum level { pedantic = SBMT_PEDANTIC_LEVEL
               , debug    = SBMT_DEBUG_LEVEL
               , verbose  = SBMT_VERBOSE_LEVEL
               , info     = SBMT_INFO_LEVEL
               , terse    = SBMT_TERSE_LEVEL
               , warning  = SBMT_WARNING_LEVEL
               , error    = SBMT_ERROR_LEVEL
               , none     = SBMT_NEVER_LOG
                 , inherit = SBMT_INHERIT_LEVEL };
    enum { most_logging = pedantic };
    enum { least_logging = none };

    logging_level(level lvl = none) : lvl(lvl) {}
    logging_level(unsigned int lvl) : lvl(level(lvl)) {}
    bool operator==(logging_level const& o) const { return lvl == o.lvl; }
    bool operator< (logging_level const& o) const { return lvl <  o.lvl; }
    void read(std::istream& in);
    void write(std::ostream& out) const;
  friend inline bool is_null(logging_level const& l) {
    return l.lvl==inherit;
  }
  logging_level &operator+=(int i) {
    assert(!is_null(*this));
    if (i<0) return *this-=-i;
    lvl=(level)((int)lvl+i);
    if (lvl>(level)most_logging)
      lvl=(level)most_logging;
    return *this;
  }
  logging_level &operator-=(int i) {
    assert(!is_null(*this));
    if (i<0) return *this+=-i;
    lvl=(level)((int)lvl-i);
    if (lvl<(level)least_logging || lvl>(level)most_logging) // overflow
      lvl=(level)least_logging;
    return *this;
  }
  logging_level operator+(int i) const {
    logging_level r=*this;
    r+=i;
    return r;
  }
  logging_level operator-(int i) const {
    logging_level r=*this;
    r-=i;
    return r;
  }

private:
    level lvl;
};

////////////////////////////////////////////////////////////////////////////////

inline std::istream& operator>>(std::istream& in, logging_level& lvl)
{
    lvl.read(in);
    return in;
}

////////////////////////////////////////////////////////////////////////////////

inline std::ostream& operator<<(std::ostream& out, logging_level const& lvl)
{
    lvl.write(out);
    return out;
}

////////////////////////////////////////////////////////////////////////////////

namespace {
    static logging_level lvl_inherit(logging_level::inherit);
    static logging_level lvl_pedantic(logging_level::pedantic);
    static logging_level lvl_debug(logging_level::debug);
    static logging_level lvl_verbose(logging_level::verbose);
    static logging_level lvl_info(logging_level::info);
    static logging_level lvl_terse(logging_level::terse);
    static logging_level lvl_warning(logging_level::warning);
    static logging_level lvl_error(logging_level::error);
    static logging_level lvl_none(logging_level::none);
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::io


#endif // SBMT_IO_LOGGING_FWD_HPP
