#ifndef GRAEHL__SHARED__PROGRAM_OPTIONS_HPP
#define GRAEHL__SHARED__PROGRAM_OPTIONS_HPP

/* wraps boost options_description as printable_opts - show the options values used as well as defaults in usage or log messages.  boost program options library lacks any concept of printing configured values; it only supports parsing them from strings */

#ifdef _WIN32
#include <iso646.h>
#endif

# ifndef BOOST_SYSTEM_NO_DEPRECATED
#  define BOOST_SYSTEM_NO_DEPRECATED 1
# endif
#include <boost/program_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/range/value_type.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <fstream>
#include <boost/pool/object_pool.hpp>
#include <graehl/shared/prefix_option.hpp>
#include <graehl/shared/containers.hpp>
#include <deque>
#include <list>

namespace graehl {

template <class OD,class OptionsValues>
void caption_add_options(OD &optionsDesc, OptionsValues &vals)
{
  OD nested(vals.caption());
  vals.add_options(nested);
  optionsDesc.add(nested);
}

template <class OD,class OptionsValues,class Prefix>
void caption_add_options(OD &optionsDesc, OptionsValues &vals,Prefix const& prefix)
{
  OD nested(vals.caption());
  vals.add_options(nested,prefix);
  optionsDesc.add(nested);
}

inline bool contains(boost::program_options::variables_map const& vm,std::string const& key)
{ return (bool)vm.count(key); }

template <class V>
inline bool maybe_get(boost::program_options::variables_map const& vm,std::string const& key,V &val) {
  if (vm.count(key)) {
    val=vm[key].as<V>();
    return true;
  }
  return false;
}

inline std::string get_string(boost::program_options::variables_map const& vm,std::string const& key) {
  return vm[key].as<std::string>();
}

// change --opt-name=x --opt_name=x for all strings x.  danger: probably the argv from int main isn't supposed to be modified?
inline
int arg_minusto_underscore(char *s) {
  if (!*s || *s++ != '-') return 0;
  if (!*s || *s++ != '-') return 0;
  int chars_replaced=0;
  for(;*s;++s) {
    if (*s=='=')
      break;
    if (*s=='-') {
      *s='_';
      ++chars_replaced;
    }
  }
  return chars_replaced;
}

inline
int argv_minus_to_underscore(int argc, char **argv) {
  int chars_replaced=0;
  for (int i=1;i<argc;++i) {
    chars_replaced+=arg_minusto_underscore(argv[i]);
  }
  return chars_replaced;
}

template <class T>
boost::program_options::typed_value<T>*
defaulted_value(T *v)
{
  return boost::program_options::value<T>(v)->default_value(*v);
}

template <class T>
boost::program_options::typed_value<T>*
optional_value(T *v)
{
  return boost::program_options::value<T>(v);
}

inline void program_options_fatal(std::string const& msg) {
  throw std::runtime_error(msg);
}


inline std::string const& get_single_arg(boost::any& v,std::vector<std::string> const& values)
{
  boost::program_options::validators::check_first_occurrence(v);
  return boost::program_options::validators::get_single_string(values);
}

template <class I>
void must_complete_read(I &in,std::string const& msg="Couldn't parse")
{
  char c;
  if (in.bad())
    program_options_fatal(msg + " - failed input");
  if (in >> c)
    program_options_fatal(msg + " - got extra char: " + std::string(c,1));
}

template <class Ostream,class C>
void print_multitoken(Ostream &o,C const& v) {
  o<<"[";
  for (typename C::const_iterator i=v.begin(),e=v.end();i!=e;++i)
    o<<' '<<*i;
  o<<" ]";
}

template <class Ostream>
struct any_printer : public boost::function<void (Ostream &,boost::any const&)>
{
  typedef boost::function<void (Ostream &,boost::any const&)> F;

  template <class T>
  struct typed_print
  {
    void operator()(Ostream &o,boost::any const& t) const
    {
      o << *boost::any_cast<T const>(&t);
    }
  };

  template <class T>
  struct typed_print<std::vector<T> >
  {
    void operator()(Ostream &o,boost::any const& t) const
    {
      print_multitoken(o,*boost::any_cast<std::vector<T> const>(&t));
    }
  };

  template <class T>
  struct typed_print<std::deque<T> >
  {
    void operator()(Ostream &o,boost::any const& t) const
    {
      print_multitoken(o,*boost::any_cast<std::vector<T> const>(&t));
    }
  };

  template <class T>
  struct typed_print<std::list<T> >
  {
    void operator()(Ostream &o,boost::any const& t) const
    {
      print_multitoken(o,*boost::any_cast<std::vector<T> const>(&t));
    }
  };

  template <class T>
  static
  void typed_print_template(Ostream &o,boost::any const& t)
  {
    o << *boost::any_cast<T const>(&t);
  }

  any_printer() {}

  any_printer(const any_printer& x)
    : F(static_cast<F const&>(x))
  {}

  template <class T>
  explicit any_printer(T const* tag) : F(typed_print<T>()) {
  }

  template <class T>
  void set()
  {
    F f((typed_print<T>())); // extra parens necessary
    swap(f);
  }
};


// have to wrap regular options_description and store our own tables because
// author didn't make enough stuff protected/public or add a virtual print
// method to value_semantic
template <class Ostream>
struct printable_options_description
  : boost::program_options::options_description
{
  typedef printable_options_description<Ostream> self_type;
  typedef boost::program_options::options_description options_description;
  typedef boost::program_options::option_description option_description;
  typedef boost::shared_ptr<self_type> group_type;
  typedef std::vector<group_type > groups_type;
  struct printable_option
  {
    typedef boost::shared_ptr<option_description> OD;

    any_printer<Ostream> print;
    OD od;
    bool in_group;

    std::string const& name()
    { return od->long_name(); }

    std::string const& description()
    { return od->description(); }

    std::string const& vmkey()
    {
      return od->key(name());
    }
    template <class T>
    printable_option(T *tag, OD const& od) : print(tag),od(od),in_group(false) {}
    printable_option() : in_group(false) {}
  };
  typedef std::vector<printable_option > options_type;
  BOOST_STATIC_CONSTANT(unsigned,default_linewrap=80); // options_description::m_default_line_length
  printable_options_description(unsigned line_length = default_linewrap) :
    options_description(line_length) { init(); }

  typedef boost::object_pool<std::string> string_pool;
  printable_options_description(const std::string& caption,
                                unsigned line_length = default_linewrap)
    : options_description(caption,line_length), caption(caption) { init(); }

  void init() {
    n_this_level=0;
    n_nonempty_groups=0;
    descs.reset(new string_pool());
  }

  self_type &add_options()
  { return *this; }


  template <class V>
  self_type &
  multiple(char const* name,
           V *val,
           char const* description, bool hidden=false)
  {
    return (*this)(name,optional_value(val)->multitoken(),description,hidden);
  }

  template <class V>
  self_type &
  multiple(char const* name,
           V *val,
           std::string const& description, bool hidden=false)
  {
    return (*this)(name,optional_value(val)->multitoken(),description,hidden);
  }

  template <class V>
  self_type &
  optional(char const* name,
           V *val,
           std::string const& description, bool hidden=false)
  {
    return (*this)(name,optional_value(val),description,hidden);
  }

  template <class V>
  self_type &
  optional(char const* name,
           V *val,
           char const* description, bool hidden=false)
  {
    return (*this)(name,optional_value(val),description,hidden);
  }

  typedef std::string option_name;

  template <class V>
  self_type &
  defaulted(option_name name,
            V *val,
            std::string const& description, bool hidden=false)
  {
    return (*this)(name,defaulted_value(val),description,hidden);
  }

  template <class V>
  self_type &
  required(option_name name,
           V *val,
           std::string const& description, bool hidden=false)
  {
    return (*this)(name,defaulted_value(val)->required(),description,hidden);
  }


  template <class V>
  self_type &
  defaulted(char const* name,
            V *val,
            char const* description, bool hidden=false)
  {
    return (*this)(name,defaulted_value(val),description,hidden);
  }

  boost::shared_ptr<string_pool> descs; // because opts lib only takes char *, hold them here.
  template <class T,class C>
  self_type &
  operator()(char const* name,
             boost::program_options::typed_value<T,C> *val,
             std::string const& description,bool hidden=false)
  {
    return (*this)(name,val,cstr(description),hidden);
  }

  char const* cstr(std::string const& s)
  {
    return descs->construct(s)->c_str();
  }

  template <class T,class C>
  self_type &
  operator()(std::string const& name,
             boost::program_options::typed_value<T,C> *val,
             std::string const& description,bool hidden=false)
  {
    return (*this)(cstr(name),val,cstr(description),hidden);
  }

  std::size_t n_this_level,n_nonempty_groups;
  template <class T,class C>
  self_type &
  operator()(char const* name,
             boost::program_options::typed_value<T,C> *val,
             char const*description=NULL,bool hidden=false)
  {
    ++n_this_level;
    printable_option opt((T *)0,simple_add(name,val,description));
    if (!hidden)
      pr_options.push_back(opt);

    return *this;
  }


  self_type&
  add(self_type const& desc,bool hidden=false)
  {
    options_description::add(desc);
    if (hidden) return *this;
    groups.push_back(group_type(new self_type(desc)));
    if (desc.size()) {
      for (typename options_type::const_iterator i=desc.pr_options.begin(),e=desc.pr_options.end();
           i!=e;++i) {
        pr_options.push_back(*i);
        pr_options.back().in_group=true;
      }
      ++n_nonempty_groups; // could just not add an empty group. but i choose to allow that.
    }

    return *this;
  }

  void print_option(Ostream &o,
                    printable_option &opt,
                    boost::program_options::variable_value const & var,
                    bool only_value=false)
  {
    using namespace boost;
    using namespace boost::program_options;
    using namespace std;
    string const& name=opt.name();
    if (!only_value) {
      if (var.defaulted())
        o << "#DEFAULTED# ";
      if (var.empty()) {
        o << "#EMPTY# "<<name;
        return;
      }
      o << name<<" = ";
    }
    opt.print(o,var.value());
  }

  enum { SHOW_DEFAULTED=0x1
         , SHOW_EMPTY=0x2
         , SHOW_DESCRIPTION=0x4
         ,  SHOW_HIERARCHY=0x8
         ,  SHOW_EMPTY_GROUPS=0x10
         ,  SHOW_ALL=0x0FFF
         ,  SHOW_HELP=0x1000
  };

  typedef std::vector<printable_option> option_set;
  void collect_defaulted(boost::program_options::variables_map &vm,option_set &defaults)
  {
    for (typename options_type::iterator i=pr_options.begin(),e=pr_options.end();
         i!=e;++i) {
      printable_option & opt=*i;
      if (vm[opt.vmkey()].defaulted())
        defaults.push_back(opt);
    }
    for (typename groups_type::iterator i=groups.begin(),e=groups.end();
         i!=e;++i) {
      (*i)->collect_defaulted(vm,defaults);
    }
  }

  bool validate(boost::program_options::variables_map &vm)
  {
    // was going to check that required options were set here; it turns out that the built in ->defaulted() does this for me
    return true;
  }

  void print(Ostream &o,
             boost::program_options::variables_map &vm,
             int show_flags=SHOW_DESCRIPTION & SHOW_DEFAULTED & SHOW_HIERARCHY)
  {
    const bool show_defaulted=bool(show_flags & SHOW_DEFAULTED);
    const bool show_description=bool(show_flags & SHOW_DESCRIPTION);
    const bool hierarchy=bool(show_flags & SHOW_HIERARCHY);
    const bool show_empty=bool(show_flags & SHOW_EMPTY);
    const bool show_help=bool(show_flags & SHOW_HELP);
    const bool show_empty_groups=bool(show_flags & SHOW_EMPTY_GROUPS);

    using namespace boost::program_options;
    using namespace std;
    if (show_empty_groups || n_this_level || n_nonempty_groups>1)
      o << "### " << caption << endl;
    for (typename options_type::iterator i=pr_options.begin(),e=pr_options.end();
         i!=e;++i) {
      printable_option & opt=*i;
      if (!show_help && opt.name()=="help")
        continue;
      if (hierarchy and opt.in_group)
        continue;
      variable_value const & var=vm[opt.vmkey()];
      if (var.defaulted() && !show_defaulted)
        continue;
      if (var.empty() && !show_empty)
        continue;
      if (show_description)
        o << "# " << opt.description() << endl;
      print_option(o,opt,var);
      o << endl;
    }
    o << endl;
    if (hierarchy)
      for (typename groups_type::iterator i=groups.begin(),e=groups.end();
           i!=e;++i)
        if (show_empty_groups || (*i)->size())
          (*i)->print(o,vm,show_flags);
  }

  typedef std::vector<std::string> unparsed_args;

  // remember to call store(return,vm) and notify(vm)
  boost::program_options::parsed_options
  parse_options(int argc,char * argv[]
                , boost::program_options::positional_options_description *po=NULL
                , unparsed_args *unparsed_out=NULL
                , bool allow_unrecognized_positional=false
                , bool allow_unrecognized_opts=false
    ) {
    using namespace boost::program_options;
    command_line_parser cl(argc,const_cast<char **>(argv));
    cl.options(*this);
    if (po)
      cl.positional(*po);
    if (allow_unrecognized_opts)
      cl.allow_unregistered();
    parsed_options parsed=cl.run();
    std::vector<std::string> unparsed=collect_unrecognized(parsed.options,
                                                           po ? exclude_positional : include_positional);
    if (!allow_unrecognized_positional) {
      if (!unparsed.empty())
        program_options_fatal("Unrecognized argument: "+unparsed.front());
    }
    if (unparsed_out)
      unparsed_out->swap(unparsed);
    return parsed;
  }

  /// parses arguments, then stores/notifies from opts->vm.  returns unparsed
  /// options and positional arguments, but if not empty, throws exception unless
  /// allow_unrecognized_positional is true
  std::vector<std::string>
  parse_options(int argc,char *argv[],
                boost::program_options::variables_map &vm,
                boost::program_options::positional_options_description *po=NULL,
                bool allow_unrecognized_positional=false,
                bool allow_unrecognized_opts=false
    )
  {
    unparsed_args r;
    boost::program_options::store(parse_options(argc,argv,po,&r,allow_unrecognized_positional,allow_unrecognized_opts),vm);
    notify(vm);
    return r;
  }

  std::size_t ngroups() const { return groups.size(); }
  std::size_t size() const { return pr_options.size(); }

private:
  groups_type groups;
  options_type pr_options;
  std::string caption;
  boost::shared_ptr<option_description>
  simple_add(const char* name,
             const boost::program_options::value_semantic* s,
             const char * description = NULL)
  {
    typedef option_description OD;
    boost::shared_ptr<OD> od(
      (description ? new OD(name,s,description) : new OD(name,s))
      );
    options_description::add(od);
    return od;
  }
};

typedef printable_options_description<std::ostream> printable_opts;

// since this can't be found by ADL for many containers, use the macro PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(container<X>) below to place it as boost::program_options::validate
template<class C, class charT>
void validate_collection(boost::any& v,
                         const std::vector<std::basic_string<charT> >& s,
                         C*,
                         int)
{
  if (v.empty())
    v = boost::any(C());
  C* tv = boost::any_cast<C>(&v);
  assert(tv);
  for (unsigned i=0,e=s.size();i<e;++i) {
    try {
      boost::any a;
      std::vector<std::basic_string<charT> > cv;
      cv.push_back(s[i]);
      typedef typename boost::range_value<C>::type V;
      validate(a, cv, (V*)0, 0);
      add(*tv,boost::any_cast<V>(a));
    }
    catch(boost::bad_lexical_cast& /*e*/) {
      program_options_fatal("Couldn't validate option value: "+s[i]);
    }
  }
}

} //graehl


/** Validates sequences. Allows multiple values per option occurrence
    and multiple occurrences. boost only provides std::vector. */

// use these macros in global namespace. note that the preprocessor does not understand commas inside <> so you may need to use extra parens or a comma macro
// e.g. PROGRAM_OPTIONS_FOR_CONTAINER((std::map<int,int>))
#define PROGRAM_OPTIONS_FOR_CONTAINER(ContainerFullyQualified)    \
  namespace boost {                                               \
  namespace program_options {                                     \
  template<class charT>                                           \
  void validate(boost::any& v,                                    \
                const std::vector<std::basic_string<charT> >& s,  \
                ContainerFullyQualified* f,                       \
                int i)                                            \
  { graehl::validate_collection(v,s,f,i); }}}

//e.g. PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(class T,std::vector<T>)
#define PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(TemplateArgs,ContainerTemplate)  \
  namespace boost {                                                             \
  namespace program_options {                                                   \
  template<TemplateArgs,class charT>                                   \
  void validate(boost::any& v,                                                  \
                const std::vector<std::basic_string<charT> >& s,                \
                ContainerTemplate* f,                                        \
                int i)                                                          \
  { graehl::validate_collection(v,s,f,i); }}}

#ifndef TEMPLATE_COMMA
#define TEMPLATE_COMMA ,
#endif

PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(class T TEMPLATE_COMMA class A,std::deque<T TEMPLATE_COMMA A>)

PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(class T TEMPLATE_COMMA class A,std::list<T TEMPLATE_COMMA A>)

//boost program options already provides vector

#endif
