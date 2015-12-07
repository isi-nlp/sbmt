#include <boost/io/ios_state.hpp>
#include <iosfwd>

namespace sbmt { namespace logmath {
    
struct set_format_scale 
{
    set_format_scale(format_scale fmt);
    void operator()(std::ios_base& io) const;
    static format_scale current_format_scale(std::ios_base& io);
    static long io_fmt_idx();
    static format_scale def;
    struct save : public boost::io::ios_iword_saver
    {
        typedef boost::io::ios_iword_saver Base;
        save(std::ios_base &ios) : Base(ios,io_fmt_idx()) {}
    };  
 private:
    format_scale fmt;
};

struct set_format_base
{
    set_format_base(format_base fmt_base);
    void operator()(std::ios_base& io) const;
    static long io_fmt_base_idx();
    static format_base current_format_base(std::ios_base& io);
    static format_base def;
    struct save : public boost::io::ios_iword_saver
    {
        typedef boost::io::ios_iword_saver Base;
        save(std::ios_base &ios) : Base(ios,io_fmt_base_idx()) {}
    };  
 private:
    format_base fmt_base;
};
    



template <class BaseT, class F,class CharT, class TraitsT>            
std::basic_ostream<CharT,TraitsT>&
operator << (std::basic_ostream<CharT,TraitsT>& o, basic_lognumber<BaseT,F> const& p) 
{
    format_base base=set_format_base::current_format_base(o);
    format_scale fmt=set_format_scale::current_format_scale(o);
    
    if (fmt==fmt_neglog10_scale) {
        o << p.neglog10();
    } else if (fmt==fmt_linear_scale || p.is_zero())
        o << p.linear();
    else if (base == fmt_base_e)
        o << "e^"<<p.ln();
    else
        o << "10^"<<p.log10();
    return o;
}

template <class BaseT, class F,class CharT, class TraitsT>            
std::basic_istream<CharT,TraitsT>&
operator>>(std::basic_istream<CharT,TraitsT>& in, basic_lognumber<BaseT,F>& p) 
{
    char c;
    double d;
    in >> std::ws;
    if (in.get(c)) {
        if (c=='e') {
            if (in.get(c)) {
                if (c=='^') {
                    if (in >> d) {
                        p.set(d,as_ln());
                    }
                } else {
                    in.setstate(std::ios_base::failbit);
                }
            }
            return in;
        } else {
            in.unget();
            if (in >> d) {
                if (in.get(c)) {
                    if (c=='^') {
                        double e;
                        if (in >> e) {
                            if (d==10) {// note: highly redundant and check probably not saving any cycles, but I'm trying to figure out why Perl-calculated sum of 10^X*...= 10^(X+... is off by 1 part in 10000
                                p.set(e,as_log10());
                            } else {
                                p.set(d,e); // d^e
                            }
                            return in;
                        }
                    } else {
                        in.unget();
                    }
                } else
                    in.clear(); // don't leave boost::lexical_cast crying because we didn't read an exponent.
                if (fmt_neglog10_scale==set_format_scale::current_format_scale(in)) {
                    p.set(-d,as_log10());
                } else {
                    p = d;
                }
            }
        }
    }
    return in;
}


} } // namespace sbmt::logmath
