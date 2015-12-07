# if ! defined(SBMT__IO__LOG_AUTO_REPORT_HPP)
# define       SBMT__IO__LOG_AUTO_REPORT_HPP

# include <sbmt/io/logging_stream.hpp>
# include <graehl/shared/time_space_report.hpp>
# include <graehl/shared/stopwatch.hpp>
namespace sbmt { namespace io {

////////////////////////////////////////////////////////////////////////////////
// replicating jons time-space-change to use walltime (user total time is 
// hard to translate to real times in a multi-threaded world)
////////////////////////////////////////////////////////////////////////////////
struct time_change 
{
    static char const* default_desc() 
    { return "\nelapsed: "; }

    graehl::stopwatch time;
    
    template <class O>
    void print(O &o) const
    {
        o << const_cast<graehl::stopwatch&>(time).recent_wall_time() << " (wall) sec";
    }
    typedef time_change self_type;

    TO_OSTREAM_PRINT    
};

struct time_space_change 
{
    static char const* default_desc() 
    { return "\ntime and memory used: "; }
    time_change tc;
    graehl::memory_change mc;
    void print(std::ostream &o) const
    {
        o << tc << ", memory " << mc;
    }
    typedef time_space_change self_type;
    TO_OSTREAM_PRINT
};

template <class Change>
struct log_auto_report
{
    Change change;
    logging_stream& o;
    logging_level lvl;
    std::string desc;
    bool reported;
    log_auto_report( logging_stream& o
                   , logging_level lvl
                   , std::string const& desc=Change::default_desc() )
      : o(o)
      , lvl(lvl)
      , desc(desc)
      , reported(false) {}

    void report()
    {
        o << logging_level_manip(lvl) << desc << change << endmsg;
        reported=true;
    }
    ~log_auto_report()
    {
        if (!reported)
            report();
    }
};

////////////////////////////////////////////////////////////////////////////////

typedef log_auto_report<time_change>       log_time_report;
typedef log_auto_report<time_space_change> log_time_space_report;

} } // namespace sbmt::io


# endif //     SBMT__IO__LOG_AUTO_REPORT_HPP
