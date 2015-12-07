#include <sbmt/logmath/logmath_io.hpp>
#include <sbmt/logmath/lexical_cast.hpp>

namespace sbmt { namespace logmath {

// LOG VS LINEAR

set_format_scale::set_format_scale(format_scale fmt) : fmt(fmt) {}

void set_format_scale::operator()(std::ios_base& io) const 
{
    io.iword(io_fmt_idx()) = fmt;
}

format_scale set_format_scale::current_format_scale(std::ios_base& io) 
{
    long f = io.iword(io_fmt_idx());
    if (f == fmt_default_scale)
        return def;

    return static_cast<format_scale>(f);
}

long set_format_scale::io_fmt_idx() 
{
    static const long _io_fmt_idx = std::ios_base::xalloc();
    return _io_fmt_idx;  
}

format_scale set_format_scale::def=fmt_log_scale;






std::ios_base& 
linear_scale(std::ios_base& ios)
{
    set_format_scale s(fmt_linear_scale);
    s(ios);
    return ios;
}

std::ios_base& 
log_scale(std::ios_base& ios)
{
    set_format_scale s(fmt_log_scale);
    s(ios);
    return ios;
} 

std::ios_base& 
neglog10_scale(std::ios_base& ios)
{
    set_format_scale s(fmt_neglog10_scale);
    s(ios);
    return ios;
} 





// (when log) BASE E vs BASE 10

set_format_base::set_format_base(format_base fmt_base) : fmt_base(fmt_base) {}

void set_format_base::operator()(std::ios_base& io) const 
{
    io.iword(io_fmt_base_idx()) = fmt_base;
}

format_base set_format_base::current_format_base(std::ios_base& io) 
{
    long f = io.iword(io_fmt_base_idx());
    if (f == fmt_default_base)
        return def;
    return static_cast<format_base>(f);
}

long set_format_base::io_fmt_base_idx() 
{
    static const long _io_fmt_base_idx = std::ios_base::xalloc();
    return _io_fmt_base_idx;  
}


////////////////////////////////////////////////////////////////////////////////

std::ios_base& 
log_base_e(std::ios_base& ios)
{
    set_format_base s(fmt_base_e);
    s(ios);
    return ios;
}

std::ios_base& 
log_base_10(std::ios_base& ios)
{
    set_format_base s(fmt_base_10);
    s(ios);
    return ios;
} 

format_base set_format_base::def=fmt_base_10;

////////////////////////////////////////////////////////////////////////////////

format_scale set_default_scale(format_scale fmt_scale) 
{
    format_scale &def=set_format_scale::def;
    format_scale old=def;
    if (fmt_scale==fmt_default_scale)
        def=fmt_log_scale;
    else
        def=fmt_scale;
    return old;
}

format_base set_default_base(format_base fmt_base) 
{
    format_base &def=set_format_base::def;
    format_base old=def;
    if (fmt_base==fmt_default_base)
        def=fmt_base_10;
    else
        def=fmt_base;
    return old;
}
    


#ifdef OLD_LOGMATH_LEXICAL_CAST
extern const boost::regex match_10_carat("^10\\^");
extern const boost::regex match_e_carat("^e\\^");
#endif

} }
