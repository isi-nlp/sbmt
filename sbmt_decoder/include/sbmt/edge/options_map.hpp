# if ! defined(SBMT__EDGE__OPTIONS_MAP_HPP)
# define       SBMT__EDGE__OPTIONS_MAP_HPP

# include <map>
# include <string>
# include <boost/lexical_cast.hpp>
# include <boost/shared_ptr.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  an option_variable is anything that can be trivially converted to/from
///  a string using a lexical_cast ( ie supports >> and << stream operators)
///
////////////////////////////////////////////////////////////////////////////////
class any_option {
public:
    template <class Option>
    any_option(Option const& opt) : pimpl(new holder<Option>(opt)) {}
    any_option(any_option const& other) : pimpl(other.pimpl) {}
    
    void operator()(std::string const& value_string) 
    { pimpl->set(value_string); }
    
    operator std::string() const { return pimpl->get(); }
private:
    struct placeholder {
        virtual void set(std::string const& value_string) = 0;
        virtual std::string get() const = 0;
        virtual ~placeholder() {}
    };
    
    template <class Option>
    struct holder : placeholder {
        virtual void set(std::string const& value_string)
        {
            opt(value_string);
        }
        virtual std::string get() const 
        {
            return std::string(opt);
        }
        holder(Option const& opt) : opt(opt) {}
        Option opt;
    };
    boost::shared_ptr<placeholder> pimpl;
};

/// an option that will call a notification function after 
template <class Option, class Notifier>
class additional_notification_option {
public:
    additional_notification_option(Option const& opt, Notifier const& notifier)
      : opt(opt)
      , notifier(notifier) {}
    void operator()(std::string const& value_string)
    {
        opt(value_string);
        notifier();
    }
    operator std::string() const { return std::string(opt); }
private:
    Option opt;
    Notifier notifier;
};

template <class Var>
class option_variable {
public:
    option_variable(Var& var) : var(&var) {}
    void operator()(std::string const& val)
    {
        std::stringstream sstr(val);
        sstr >> std::boolalpha;
        sstr >> *var;
        std::stringstream sstr2(val);
        sstr2 >> std::noboolalpha;
        sstr2 >> *var;
    }
    operator std::string() const
    {
        return boost::lexical_cast<std::string>(*var);
    }
private:
    Var* var;
};

template <class Var>
option_variable<Var> optvar(Var& var) { return option_variable<Var>(var); }

template <class Option, class Notifier>
additional_notification_option<Option,Notifier>
add_notifier(Option const& opt, Notifier const& notifier)
{
    return additional_notification_option<Option,Notifier>(opt,notifier);
}

////////////////////////////////////////////////////////////////////////////////

struct described_option {
    any_option opt;
    std::string description;
    described_option(any_option const& opt, std::string const& description)
      : opt(opt)
      , description(description) {}
};

////////////////////////////////////////////////////////////////////////////////



class options_map {
    struct donothing {
        typedef void result_type;
        void operator()() {}
    };
    typedef std::map<std::string,described_option> options_map_type;
public:
    typedef options_map_type::iterator iterator;
    typedef options_map_type::const_iterator const_iterator;
    
    const_iterator begin() const { return opts.begin(); }
    const_iterator end() const { return opts.end(); }
    iterator begin() { return opts.begin(); }
    iterator end() { return opts.end(); }
    const_iterator find(std::string const& x) const { return opts.find(x); }
    iterator find(std::string const& x) { return opts.find(x); }
    
    std::string title() const { return t; }
    
    options_map(std::string const& title)
     : notifier(donothing())
     , t(title) {}
     
    options_map(std::string const& title, boost::function<void(void)> notifier)
      : notifier(notifier)
      , t(title) {}
      
    void set_option_value(std::string const& name, std::string const& value);
    void add_option( std::string const& option_name
                   , any_option option_setter
                   , std::string const& option_description )
    {
        opts.insert( std::make_pair( option_name
                                   , described_option( add_notifier(option_setter, notifier)
                                                     , option_description
                                                     )
                                   )
                   )
                   ;
    }
private:
    options_map_type opts;
    boost::function<void(void)> notifier;
    std::string t;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__EDGE__OPTIONS_MAP_HPP
