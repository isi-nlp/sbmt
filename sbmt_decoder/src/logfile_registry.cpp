
# include <graehl/shared/fileargs.cpp>
# include <sbmt/io/logging.hpp>
# include <sbmt/io/logging_macros.hpp>
# include <sbmt/io/logfile_registry.hpp>
# include <sbmt/io/logging_level.hpp>
# include <sbmt/io/detail/sync_logging_buffer.hpp>

# include <stdexcept>
# include <sstream>

# include <boost/lexical_cast.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/iterator/indirect_iterator.hpp>

# define SBMT_REGISTRY_DEBUG 0

# if SBMT_REGISTRY_DEBUG
  # include <boost/format.hpp>
  # define SBMT_REGISTRY_MSG( msg, args ) \
  do { \
    std::stringstream sstr; \
    sstr << boost::format(std::string("[logfile_registry] ") + (msg)) \
          % args; \
    sstr << "\n"; \
    std::clog << sstr.str() << std::flush; } while(false)
# else
  # define SBMT_REGISTRY_MSG( msg, args ) do {} while(false)
# endif

using namespace std;
using namespace boost;

namespace sbmt { namespace io {

////////////////////////////////////////////////////////////////////////////////
///
///  grouping the per-thread data into one class reduces the thread-specific
///  resource data to one pointer, and ensures that we destroy the data in
///  the right order. (boost::thread::tss cannot make any guarantees on the
///  order of destruction of thread-specific-data when a thread goes
///  out of scope)
///
////////////////////////////////////////////////////////////////////////////////
struct thread_data_t
{
    typedef detail::sync_logging_buffer sink_t;
    typedef std::map <
                std::string
              , boost::shared_ptr<sink_t>
            > buffer_map_t;
    typedef std::vector< boost::shared_ptr<logging_stream> > stream_map_t;

    // order of declaration is important.  when destroyed, first the
    // logging_streams in stream_map close, flushing to their referenced
    // buffers in buffer_map.  then the buffers in buffer_map can close.
    buffer_map_t buffer_map;
    stream_map_t stream_map;

    sink_t* set_buffer( logfile_registry::stream_and_mtx& sm );
    logging_stream* set_stream( sink_t*  buf
                              , logging_domain const& key
                              , string domain_name
                              , logging_level lvl );
    sink_t* get_buffer( std::string filename );
    logging_stream* get_stream( logging_domain const& domain );
    sink_t* use_streambuf( logfile_registry& reg
                         , logfile_registry::domain_node const& d );
};

////////////////////////////////////////////////////////////////////////////////

logging_stream*
thread_data_t::set_stream( thread_data_t::sink_t*  buffer
                         , logging_domain const& key
                         , string name
                         , logging_level lvl )
{
    SBMT_REGISTRY_MSG("creating a new stream for %s", name);
    boost::shared_ptr<logging_stream>
        str(new logging_stream(*buffer,name,lvl));
    if (stream_map.size() <= key.id()) stream_map.resize(2 * key.id() + 1);
    stream_map[key.id()] = str;
    return str.get();
}

////////////////////////////////////////////////////////////////////////////////

logging_stream*
thread_data_t::get_stream(logging_domain const& key)
{
    if (stream_map.size() <= key.id()) stream_map.resize(2 * key.id() + 1);
    return stream_map[key.id()].get();
}

////////////////////////////////////////////////////////////////////////////////

thread_data_t::sink_t*
thread_data_t::set_buffer(logfile_registry::stream_and_mtx& sm)
{
    SBMT_REGISTRY_MSG("setting buffer %s -> %s", sm.stream.name % sm.stream);
    boost::shared_ptr<thread_data_t::sink_t>
        buf(new sink_t( sm.stream.stream().rdbuf()
                      , &sm.mtx ) );
    buffer_map[sm.stream.name] = buf;
    return buf.get();
}

////////////////////////////////////////////////////////////////////////////////

thread_data_t::sink_t*
thread_data_t::get_buffer(string fname)
{
    return buffer_map[fname].get();
}

////////////////////////////////////////////////////////////////////////////////








////////////////////////////////////////////////////////////////////////////////

logging_domain::logging_domain(string n)
: name_(n)
{
    id_ = logfile_registry::instance().register_domain(*this);
}

////////////////////////////////////////////////////////////////////////////////

logging_domain::logging_domain(string n, logging_domain const& parent)
: name_(n)
{
    id_ = logfile_registry::instance().register_domain(*this,parent);
}

////////////////////////////////////////////////////////////////////////////////

string logging_domain::name() const { return name_; }

////////////////////////////////////////////////////////////////////////////////

domain_setter::domain_setter(logging_domain const& d, std::string filename)
{
    logfile_registry::instance().set_logfile(d,filename);
}

////////////////////////////////////////////////////////////////////////////////

domain_setter::domain_setter(logging_domain const& d, logging_level lvl)
{
    logfile_registry::instance().loglevel(d,lvl);
}

scoped_domain_settings::~scoped_domain_settings()
{
  if (set) {
    SBMT_VERBOSE_MSG(logging,"Restoring old logging level %1%.level=%2% (was %3%)",name(d)%or_null(lvl)%or_null(use_loglevel(d)));
    logfile_registry::instance().loglevel(d,lvl);
  }
}

void scoped_domain_settings::set_level(boost::optional<logging_level> newlvl)
{
  SBMT_VERBOSE_MSG(logging,"Temporarily, %1%.level=%2%",name(d)%or_null(newlvl));
  lvl=logs().loglevel(d,newlvl);
  set=true;
}


////////////////////////////////////////////////////////////////////////////////

logfile_registry* logfile_registry::self = 0;
bool logfile_registry::destroyed = false;

////////////////////////////////////////////////////////////////////////////////

logfile_registry::stream_and_mtx&
logfile_registry::get_stream_and_mtx(string filename)
{
    boost::recursive_mutex::scoped_lock lk(mtx);
    SBMT_REGISTRY_MSG("retrieving stream/mtx for %s", filename);
    stream_map_t::iterator pos = streams.find(filename);
    if (pos == streams.end())
        throw runtime_error(filename + " not registered with any domain");
    return *(pos->second);
}

////////////////////////////////////////////////////////////////////////////////

thread_data_t::sink_t*
thread_data_t::use_streambuf( logfile_registry& reg
                            , logfile_registry::domain_node const& d)
{
    SBMT_REGISTRY_MSG("find streambuf for domain %s", d.name());
    if (d.file_set or not d.parent) {
        sink_t* pret = get_buffer(d.filename);
        if (not pret)
            pret = set_buffer(reg.get_stream_and_mtx(d.filename));
        return pret;
    } else {
        SBMT_REGISTRY_MSG("defer to parent %s for streambuf", d.parent->name());
        return use_streambuf(reg,*d.parent);
    }
}

////////////////////////////////////////////////////////////////////////////////

thread_data_t& logfile_registry::get_thread_data()
{
    if (thread_data.get()) { return *thread_data; }
    else {
        thread_data.reset(new thread_data_t());
        return *thread_data;
    }
}

////////////////////////////////////////////////////////////////////////////////

std::streambuf* logfile_registry::rdbuf(std::string filename)
{
    stream_and_mtx& sm = get_stream_and_mtx(filename);
    return sm.stream.stream().rdbuf();
}

////////////////////////////////////////////////////////////////////////////////

std::streambuf*
logfile_registry::rdbuf(std::string filename, std::streambuf* buf)
{
    stream_and_mtx& sm = get_stream_and_mtx(filename);
    thread_data_t::sink_t*
        filtbuf = get_thread_data().get_buffer(filename);

    if (filtbuf) {
        filtbuf->close();
    }

    sm.stream.stream().rdbuf(buf);

    if (filtbuf) {
        filtbuf->rdbuf(sm.stream.stream().rdbuf());
    }
    return sm.stream.stream().rdbuf();
}

////////////////////////////////////////////////////////////////////////////////

string logfile_registry::domain_node::use_filename()
{
    string retval;
    if (file_set or not parent) {
        retval = filename;
        SBMT_REGISTRY_MSG("domain %s(%s) use_filename %s"
                         , name() % this % retval );
    } else {
        SBMT_REGISTRY_MSG("domain %s(%s) use_filename defer to parent %s(%s)"
                         , name() % this % parent->name() % parent );
        retval = parent->use_filename();
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

logging_level logfile_registry::domain_node::use_level()
{
    string fname = use_filename();
    logging_level l;
    if (fname == "-0") { l = lvl_none; }
    else if (lvl_set or not parent) {
        l = lvl;
        SBMT_REGISTRY_MSG("domain %s(%s) use_level %s"
                         , name() % this % lvl );
    } else {
        SBMT_REGISTRY_MSG("domain %s(%s) use_level defer to parent %s(%s)"
                         , name() % this % parent->name() % parent );
        l = parent->use_level();
    }
    return l;
}

////////////////////////////////////////////////////////////////////////////////

logfile_registry::domain_node::domain_node(logging_domain const& d)
  : d(d)
  , parent(NULL)
  , lvl_set(true)
  , lvl(0)
  , file_set(true)
  , filename("-0") {}

////////////////////////////////////////////////////////////////////////////////

logfile_registry::domain_node::domain_node(
                                    logging_domain const& d
                                  , shared_ptr<domain_node> const& parent
                                  )
  : d(d)
  , parent(parent.get())
  , lvl_set(false)
  , lvl(0)
  , file_set(false)
  , filename("-0") {}

////////////////////////////////////////////////////////////////////////////////

logfile_registry::stream_and_mtx::stream_and_mtx(string filename)
  : stream(filename) {}

////////////////////////////////////////////////////////////////////////////////

string logfile_registry::domain_node::name() const
{
    string retval;
    if (not parent) {
        retval = d.name();
    } else {
        retval = parent->name() + "." + d.name();
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

void logfile_registry::create()
{
    static logfile_registry r;
    self = &r;
}

////////////////////////////////////////////////////////////////////////////////

logfile_registry& logfile_registry::instance()
{
    if (not self) {
        if (destroyed) {
            throw runtime_error("registry accessed post-destruction");
        } else create();
    }
    return *self;
}

////////////////////////////////////////////////////////////////////////////////

size_t logfile_registry::insert_helper( logging_domain const& d
                                      , boost::shared_ptr<domain_node> ptr )
{
    boost::recursive_mutex::scoped_lock lk(mtx);
    size_t retval;
    name_map_t::iterator pos = name_map.find(d.name());
    if (pos != name_map.end()) {
        retval = pos->second;
    } else {
        retval = domains.size();
        domains.push_back(ptr);
        name_map.insert(make_pair(d.name(),retval));
    }
    return retval;
}

std::size_t logfile_registry::register_domain(logging_domain const& d)
{
    boost::recursive_mutex::scoped_lock lk(mtx);
    shared_ptr<domain_node> ptr(new domain_node(d));
    name_map_t::iterator pos = name_map.find(d.name());
    return insert_helper(d,ptr);
}

////////////////////////////////////////////////////////////////////////////////

std::size_t logfile_registry::register_domain( logging_domain const& d
                                             , logging_domain const& parent)
{
    boost::recursive_mutex::scoped_lock lk(mtx);
    shared_ptr<domain_node> ptr = domains.at(parent.id());
    shared_ptr<domain_node> child(new domain_node(d,ptr));
    return insert_helper(d,child);
}

////////////////////////////////////////////////////////////////////////////////

void logfile_registry::set_logfile( logging_domain const& d
                                  , string filename )
{
    domain_node& dnode = get_domain_node(d);
    thread_data_t& tdata = get_thread_data();
    insert_logfile(filename);
    dnode.filename = filename;
    dnode.file_set = true;
    SBMT_REGISTRY_MSG("domain %s(%s)(%s) set_logfile %s"
                     , dnode.name() % d.id() % &dnode % dnode.filename );
    if (tdata.get_stream(d)) {
        SBMT_REGISTRY_MSG("domain %s(%s)(%s) has logstream. reset."
                         , dnode.name() % d.id() % &dnode );
        tdata.set_stream( tdata.use_streambuf(*this,dnode)
                        , d
                        , dnode.name()
                        , dnode.use_level() );
    }
    SBMT_REGISTRY_MSG("registry: %s", reflect());
}

////////////////////////////////////////////////////////////////////////////////

void logfile_registry::insert_logfile(string filename)
{
    boost::recursive_mutex::scoped_lock lk(mtx);
    stream_map_t::iterator pos = streams.find(filename);
    if (pos == streams.end()) {
        SBMT_REGISTRY_MSG("insert_logfile(%s)", filename);
        shared_ptr<stream_and_mtx> ptr(new stream_and_mtx(filename));
        streams.insert(make_pair(filename,ptr));
    }
}

////////////////////////////////////////////////////////////////////////////////

boost::optional<logging_level>
logfile_registry::loglevel( logging_domain const& d
                           , boost::optional<logging_level> lvl )
{
    domain_node& dnode = get_domain_node(d);
    thread_data_t& tdata = get_thread_data();

    boost::optional<logging_level> ret;
    if (dnode.lvl_set) ret = dnode.lvl;

    if (lvl) {
        dnode.lvl = *lvl;
        dnode.lvl_set = true;
    } else {
      dnode.lvl = 0; // unnecessary, but should cause a segfault if code were buggy
        dnode.lvl_set = false;
    }

    if (tdata.get_stream(d)) {
        tdata.set_stream( tdata.use_streambuf(*this,dnode)
                        , d
                        , dnode.name()
                        , dnode.use_level() );
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

logging_stream& logfile_registry::log(logging_domain const& d)
{
    SBMT_REGISTRY_MSG("getting log %s(%s)", d.name() % d.id());
    thread_data_t& tdata = get_thread_data();
    logging_stream* plog = tdata.get_stream(d);
    if (not plog) {
        SBMT_REGISTRY_MSG("initting log %s(%s)", d.name() % d.id());
        domain_node& dnode = get_domain_node(d);
        plog = tdata.set_stream( tdata.use_streambuf(*this,dnode)
                               , d
                               , dnode.name()
                               , dnode.use_level() );
    }
    return *plog;
}

////////////////////////////////////////////////////////////////////////////////

logfile_registry::domain_node&
logfile_registry::get_domain_node(logging_domain const& d)
{
    SBMT_REGISTRY_MSG("get_domain_node %s(%s) -> %s"
                     , d.name() % d.id() % domains.at(d.id())->name());
    return *(domains.at(d.id()));
}

////////////////////////////////////////////////////////////////////////////////

string logfile_registry::reflect() const
{
    stringstream retval;
    boost::indirect_iterator<domain_map_t::const_iterator>
        itr = domains.begin(),
        end = domains.end();
    //cout << this << " ";
    for (int x = 0; itr != end; ++itr, ++x) {
        retval << " " << itr->name() << "(" << x << ")(" << &(*itr) << ")";
        retval << "{lvl=" << itr->lvl << ",";
        retval << "lvl_set=" << itr->lvl_set << ",";
        retval << "file=" << itr->filename << ",";
        retval << "file_set=" << itr->file_set << "}";
    }

    return retval.str();
}

////////////////////////////////////////////////////////////////////////////////

struct set_level {
    shared_ptr<logfile_registry::domain_node> ptr;
    set_level(shared_ptr<logfile_registry::domain_node> const& ptr)
      : ptr(ptr) {}
    void operator()(logging_level lvl) const {
        SBMT_REGISTRY_MSG("set_level operator() %s(%s)"
                         , ptr->name() % ptr.get() );
        ptr->lvl = lvl;
        ptr->lvl_set = true;
        SBMT_REGISTRY_MSG("registry: %s"
                         , (logfile_registry::instance().reflect()) );
    }
};

////////////////////////////////////////////////////////////////////////////////

struct set_file {
    shared_ptr<logfile_registry::domain_node> ptr;
    set_file(shared_ptr<logfile_registry::domain_node> const& ptr)
      : ptr(ptr){}
    void operator()(std::string const& filename) const {
        SBMT_REGISTRY_MSG("set_file operator() %s(%s)"
                         , ptr->name() % ptr.get() );
        logfile_registry::instance().insert_logfile(filename);
        ptr->filename = filename;
        ptr->file_set = true;
        SBMT_REGISTRY_MSG("registry: %s"
                         , (logfile_registry::instance().reflect()) );
    }
};

////////////////////////////////////////////////////////////////////////////////

graehl::printable_options_description<ostream> logfile_registry::options()
{
    using namespace graehl;
    namespace po = boost::program_options;
    stringstream header;
    header << "Logfile Registry Options: logging levels=[";
    unsigned int lvl = logging_level::least_logging;
    unsigned int last = logging_level::most_logging;
    for (; lvl < last; ++lvl) {
        header << logging_level(lvl) << ",";
    }
    header << logging_level(lvl) << "]";
    printable_options_description<ostream> od(header.str());

    domain_map_t::iterator itr = domains.begin(), end = domains.end();
    for (; itr != end; ++itr) {
        shared_ptr<domain_node> ptr = *itr;
        stringstream lvl_msg;
        stringstream file_msg;
        if (ptr->lvl_set) {
            lvl_msg << "[=" << ptr->lvl << "]";
        } else {
            lvl_msg << "[=inherit]";
        }
        od.add_options()
        ( (ptr->name() + ".level").c_str()
        , po::value<logging_level>(&(ptr->lvl))->notifier(set_level(ptr))
        , lvl_msg.str().c_str()
        );

        if (ptr->file_set) {
            file_msg << "[=" << ptr->filename << "]";
        } else {
            file_msg << "[=inherit]";
        }
        od.add_options()
        ( (ptr->name() + ".file").c_str()
        , po::value<string>(&(ptr->filename))->notifier(set_file(ptr))
        , file_msg.str().c_str()
        );
    }
    return od;
}

////////////////////////////////////////////////////////////////////////////////

logfile_registry::logfile_registry()
{
    boost::shared_ptr<stream_and_mtx> ptr(new stream_and_mtx("-0"));
    streams.insert(make_pair(string("-0"), ptr));
}

////////////////////////////////////////////////////////////////////////////////

logfile_registry::~logfile_registry()
{
    // note: order of clears is important here.  the logging_streams in domains
    // may need to flush the buffers held in streams.
    // this is just to be explicit, however; the objects are declared in the
    // correct order in the header for this to happen automatically.
    // if (thread_data.get()) thread_data.reset();
    // domain_map_t::iterator ditr = domains.begin(), dend = domains.end();
    // for (; ditr != dend; ++ditr) { ditr->second.reset(); }
    // stream_map_t::iterator sitr = streams.begin(), send = streams.end();
    // for (;sitr != send; ++sitr) { sitr->second.reset(); }

    self = NULL;
    destroyed = true;
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::io
