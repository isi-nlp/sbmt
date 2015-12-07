#ifndef   SBMT_IO_LOGFILE_REGISTRY_HPP
#define   SBMT_IO_LOGFILE_REGISTRY_HPP

# include <sbmt/io/logging_stream.hpp>

# include <boost/thread/tss.hpp>
# include <boost/thread/recursive_mutex.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/optional.hpp>

# include <string>
# include <map>
# include <iostream>

# include <graehl/shared/fileargs.hpp>
# include <graehl/shared/program_options.hpp>
# include <graehl/shared/defaulted.hpp>

namespace sbmt { namespace io {

////////////////////////////////////////////////////////////////////////////////
///
/// registers a domain associated with this->name() with the logfile_registry
/// when constructed.  intended to be used via the macros
/// SBMT_REGISTER_LOGGING_DOMAIN(d)
/// and
/// SBMT_REGISTER_CHILD_LOGGING_DOMAIN(d,parent)
///
////////////////////////////////////////////////////////////////////////////////
class logging_domain {
public:
    logging_domain(std::string n);
    logging_domain(std::string n, logging_domain const& parent);
    std::string name() const;
    std::size_t id() const { return id_; }
private:
    std::string name_;
    std::size_t id_;
};

////////////////////////////////////////////////////////////////////////////////
///
/// sets default values for given domains upon construction.
/// intended to be used via the macros
/// SBMT_SET_DOMAIN_LOGFILE(d,"filename")
/// and
/// SBMT_SET_DOMAIN_LOGGING_LEVEL(d,info)
///
////////////////////////////////////////////////////////////////////////////////
class domain_setter {
public:
    domain_setter(logging_domain const& d, std::string file);
    domain_setter(logging_domain const& d, logging_level);
};

////////////////////////////////////////////////////////////////////////////////

struct set_level;
struct set_file;
struct thread_data_t;

////////////////////////////////////////////////////////////////////////////////
///
///  singleton object that manages the lifetime of logging domains and their
///  associated logging_stream objects and logfiles.
///
////////////////////////////////////////////////////////////////////////////////
class logfile_registry {
public:
    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  when SBMT_REGISTER_LOGGING_DOMAIN is invoked, a logging domain is
    ///  registered via this function.  duplicate registrations of the same
    ///  domain name are ignored.  concurrent registrations are undefined
    ///
    ////////////////////////////////////////////////////////////////////////////
    std::size_t register_domain(logging_domain const& d);

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  when SBMT_REGISTER_CHILD_LOGGING_DOMAIN is invoked, a logging domain is
    ///  registered via this function, and is made dependent on the parent
    ///  domain.  duplicate registrations of the same domain name are ignored.
    ///  concurrent registrations are undefined.  The parent must have already
    ///  been registered.  The child domain will inherrit default settings from
    ///  the parent domain.
    ///
    ////////////////////////////////////////////////////////////////////////////
    std::size_t register_domain( logging_domain const& d
                               , logging_domain const& parent );

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  when SBMT_SET_DOMAIN_LOGGING_LEVEL is invoked, the previously
    ///  registered domain has its default logging level set to lvl.
    ///
    ////////////////////////////////////////////////////////////////////////////
    //void set_logging_level(logging_domain const& d, logging_level lvl); // deprecated
    boost::optional<logging_level>
    loglevel(logging_domain const& d) {
      domain_node & dnode=get_domain_node(d);
      if (dnode.lvl_set)
        return dnode.lvl;
      return boost::none;
    }

  logging_level
  use_loglevel(logging_domain const& d) {
    return get_domain_node(d).use_level();
  }

    boost::optional<logging_level>
        loglevel(logging_domain const& d, boost::optional<logging_level> lvl);

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  when SBMT_SET_DOMAIN_LOGFILE is invoked, the previously
    ///  registered domain has its default logfile set to lvl.
    ///
    ////////////////////////////////////////////////////////////////////////////
    void set_logfile(logging_domain const& d, std::string file); // deprecated


    /// redirect logging buffers.  currently only used for unit-tests
    //\{
    std::streambuf* rdbuf(std::string filename);
    std::streambuf* rdbuf(std::string filename, std::streambuf* buf);
    //\}

    std::string reflect() const; //debug

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  creates an options description based on the currently registered
    ///  domains. displays essentially like
    ///  \code
    ///   --parent.level (=info)
    ///   --parent.file  (=-2)
    ///   --parent.child1.level
    ///   --parent.child1.file
    ///   --parent.child2.level (=warning)
    ///   --parent.child2.file
    ///  \endcode
    ///
    ///  defaulted settings are displayed (root domains always have defaults);
    ///  inherrited settings go undisplayed, so in this example child1 has its
    ///  level and file settings inherrited from parent, and child2 has its
    ///  logfile inherrited from parent.
    ///
    ////////////////////////////////////////////////////////////////////////////
    graehl::printable_options_description<std::ostream> options();

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  returns a logging stream for the given domain.  logging_streams
    ///  provided by the logfile_registry are thread-specific.
    ///  different threads of execution will be given
    ///  different streams.  also, they are buffered so that you can write to
    ///  the same file on different threads without adding locks to your code.
    ///  because they are thread-specific, you should not hold onto a
    ///  logging_stream from the registry outside of function scope.  also, you
    ///  shouldn't flush or endl a log message except at the end of a logical
    ///  statement, since this keys the logging_stream to flush its buffer
    ///  to the common file.
    ///  If you stick to the logging macros (sbmt/io/logging_macros.hpp), you
    ///  will be fine.
    ///
    ////////////////////////////////////////////////////////////////////////////
    logging_stream& log(logging_domain const& d);

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  provides access to the logfile_registry singleton.
    ///
    ////////////////////////////////////////////////////////////////////////////
    static logfile_registry& instance();

    ~logfile_registry();
  std::string name(logging_domain const& d) {
    return get_domain_node(d).name();
  }

private:
    struct domain_node : boost::noncopyable {
        logging_domain d;
        domain_node* parent;
        domain_node(logging_domain const& d);
        domain_node( logging_domain const& d
                   , boost::shared_ptr<domain_node> const& parent );
        std::string name() const;
        bool lvl_set;
        logging_level lvl;
        bool file_set;
        std::string filename;
        logging_level use_level();
        std::string use_filename();
        logging_stream& log();
    };

    struct stream_and_mtx : boost::noncopyable {
        boost::recursive_mutex mtx;
        graehl::ostream_arg stream;
        stream_and_mtx(std::string filename);
    };

    void insert_logfile(std::string filename);
    size_t insert_helper( logging_domain const& d
                        , boost::shared_ptr<domain_node> ptr );

    stream_and_mtx& get_stream_and_mtx(std::string filename);
    domain_node& get_domain_node(logging_domain const& d);

    thread_data_t& get_thread_data();

    logfile_registry();
    static void create();
    static logfile_registry* self;
    static bool destroyed;

    typedef std::map< std::string, std::size_t >
            name_map_t;
    typedef std::map< std::string, boost::shared_ptr<stream_and_mtx> >
            stream_map_t;
    typedef std::vector< boost::shared_ptr<domain_node> >
            domain_map_t;

    // order of declaration important here.  when destroyed, first the
    // logging_streams held in thread_data must close, then the
    // ostream buffers held in streams (which are referenced by the
    // logging_streams) must close.
    name_map_t   name_map;
    stream_map_t streams;
    domain_map_t domains;
    boost::thread_specific_ptr<thread_data_t> thread_data;

    // protects access to name_map, streams, domains
    boost::recursive_mutex mtx;

    friend struct set_level;
    friend struct set_file;
    friend struct domain_node;
    friend struct stream_and_mtx;
    friend struct thread_data_t;
};

////////////////////////////////////////////////////////////////////////////////

inline logging_stream& registry_log(logging_domain const& d)
{ return logfile_registry::instance().log(d); }

////////////////////////////////////////////////////////////////////////////////

inline logfile_registry &logs() { return logfile_registry::instance(); }
inline boost::optional<logging_level> loglevel(logging_domain const& d) { return logs().loglevel(d); }
inline logging_level use_loglevel(logging_domain const& d) { return logs().use_loglevel(d); }
inline std::string name(logging_domain const& d) { return logs().name(d); }
inline logging_level or_null(boost::optional<logging_level> const& l) { return get_optional_value_or(l,lvl_inherit); }

class scoped_domain_settings {
public:
  scoped_domain_settings(logging_domain const& d, std::string file);
  scoped_domain_settings(logging_domain const& d, boost::optional<logging_level> lvl) : d(d) {
    set_level(lvl);
  }

  scoped_domain_settings(logging_domain const& d, logging_level lvl,bool increase_only=false) : d(d) {
    set_level(lvl,true,increase_only);
  }

  scoped_domain_settings(logging_domain const& d, int delta_logging_level) : d(d) {
    set_level(use_loglevel(d)+delta_logging_level);
  }

  scoped_domain_settings(logging_domain const& d,logging_level ld,logging_domain const& d_if,logging_level if_level_at_least=lvl_verbose,bool increase_only=true) : d(d) {
    set_level(ld
              ,use_loglevel(d_if)>=if_level_at_least
              ,increase_only);
  }

  scoped_domain_settings(logging_domain const& d,int delta_logging_level,logging_domain const& d_if,logging_level if_level_at_least=lvl_info) : d(d) {
    set_level(use_loglevel(d)+delta_logging_level
              ,use_loglevel(d_if)>=if_level_at_least);
  }

  ~scoped_domain_settings();
  void set_level(logging_level dlvl,bool enable=true,bool increase_only=false) {
    if (enable && (!increase_only || dlvl > use_loglevel(d)))
      set_level(boost::make_optional(dlvl));
  }
  void set_level(boost::optional<logging_level> lvl);
private:
  graehl::defaulted<bool,false> set; // we don't use the optional, because none means inherit.
  boost::optional<logging_level> lvl;
  logging_domain d;
};

} } // namespace sbmt::io

#endif // SBMT_IO_LOGFILE_REGISTRY_HPP
