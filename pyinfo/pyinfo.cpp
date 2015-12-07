# include <sbmt/edge/any_info.hpp>
#include <sbmt/edge/null_info.hpp>

# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_io.hpp>

# include <string>

#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <gusc/generator/transform_generator.hpp>

#include <boost/tokenizer.hpp>

using namespace boost;
using namespace boost::python;
using namespace std;
using namespace sbmt;

// convert Python iterator to gusc generator
// based on boost::python::stl_input_iterator
struct generator_from_python {
  typedef object result_type;

  generator_from_python(object iterable) : 
    iterator(iterable.attr("__iter__")())
  { 
    get_next();
  }
  
  result_type operator()() {
    handle<> cur = next;
    assert(cur);
    get_next();
    return extract<object>(cur.get());
  }
  
  operator bool() const {
    return bool(next); // next is null when there is no more
  }
  
private:
  void get_next() {
    PyObject *result = PyIter_Next(iterator.ptr());
    if (!result && PyErr_Occurred())
      throw_error_already_set();
    next = handle<>(allow_null(result));
  }

  object iterator;
  handle<> next;
};

///////////////////////////////////////////////////////////////////////////////
// code to work with sbmt module

typedef sbmt::detail::rule_info opaque_rule;
BOOST_PYTHON_OPAQUE_SPECIALIZED_TYPE_ID(opaque_rule)

template <typename T>
T** opaque_arg(T const *&x) {
  // contortions to get opaque_pointer_converter working
  // why doesn't it work with a const *?
  // why do I need to pass a pointer-to-pointer?
  return const_cast<T**>(&x);
}

extern "C" { void initpysbmt(); }

///////////////////////////////////////////////////////////////////////////////
// allow pretty much any Python object to be an info

class pyinfo : public info_base<pyinfo> {
public:
  pyinfo(object self) : self(self) {}

  bool equal_to(pyinfo const& other) const { 
    try {
      return self == other.self;
    } catch (error_already_set) {
      PyErr_Print();
      throw;
    }
  }

  size_t hash_value() const { 
    try {
      return extract<size_t>(self.attr("__hash__")()); 
    } catch (error_already_set) {
      PyErr_Print();
      throw;
    }
  }

  friend class pyinfo_factory;

private:
  object self;
};

///////////////////////////////////////////////////////////////////////////////

class pyinfo_factory {
public:
  // required by interface
  typedef pyinfo info_type;
    
    // info, inside-score, heuristic
  typedef boost::tuple<info_type,score_t,score_t> result_type;
    
  pyinfo_factory(object self) : self(self) {}
    
    template <class Grammar>
    score_t 
    rule_heuristic(Grammar& grammar, typename Grammar::rule_type rule) const
    {
      try {
	object result = self.attr("rule_heuristic")(ref(grammar), opaque_arg(rule));
	score_t heuristic = score_t(extract<double>(result), as_neglog10());
	return heuristic;
      } catch (error_already_set) {
	PyErr_Print();
	throw;
      }
    }
    
    template <class Grammar>
    bool 
    scoreable_rule(Grammar& grammar, typename Grammar::rule_type rule) const
    {
      try {
	object result = self.attr("scoreable_rule")(ref(grammar), opaque_arg(rule));
	return extract<bool>(result);
      } catch (error_already_set) {
	PyErr_Print();
	throw;
      }
    }

  struct make_result {
    typedef pyinfo_factory::result_type result_type;

    result_type operator()(object t) const {
      //info_type info(t[0]);
      score_t prob = score_t(extract<double>(t[1]), as_neglog10());
      score_t heuristic = score_t(extract<double>(t[2]), as_neglog10());
      return boost::tuples::make_tuple(t[0],prob,heuristic);
    }
  };

  typedef gusc::transform_generator<generator_from_python, make_result> result_generator;
    template <class ConstituentIterator>
    result_generator
    create_info( grammar_in_mem const& grammar
               , grammar_in_mem::rule_type rule
               , iterator_range<ConstituentIterator> const& range )
    {
      try {
	// make list of child infos
	boost::python::list children;
	ConstituentIterator ci = begin(range), ce = end(range);
	for (; ci != ce; ++ci)
	  children.append(ci->info()->self);

	object result = self.attr("create_info")(ref(grammar), opaque_arg(rule), children);

	return gusc::generate_transform(generator_from_python(result), make_result());
      } catch (error_already_set) {
	PyErr_Print();
	throw;
      }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    
    template <class ConstituentIterator, class ScoreOutputIterator>
    ScoreOutputIterator
    component_scores( grammar_in_mem const& grammar
                    , grammar_in_mem::rule_type rule
                    , iterator_range<ConstituentIterator> const& range 
                    , info_type const& result
                    , ScoreOutputIterator scores_out )
    {
      try {
	// make list of child infos
	boost::python::list children;
	ConstituentIterator ci = begin(range), ce = end(range);
	for (; ci != ce; ++ci)
	  children.append(ci->info()->self);

	object scores = self.attr("component_scores")(ref(grammar), opaque_arg(rule), children, result.self);
	stl_input_iterator<double> begin(scores), end;
	for (stl_input_iterator<double> it=begin; it != end; ++it) {
	  *scores_out = score_t(*it, as_neglog10());
	  ++scores_out;
	}
	return scores_out;
      } catch (error_already_set) {
	PyErr_Print();
	throw;
      }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    
    std::vector<std::string> component_score_names() const
    {
      try {
	object score_names = self.attr("component_score_names")();
	stl_input_iterator<string> begin(score_names), end;
	return std::vector<std::string> (begin, end);
      } catch (error_already_set) {
	PyErr_Print();
	throw;
      }
    }

private:
  object self;
};

////////////////////////////////////////////////////////////////////////////////


class pyinfo_constructor {
public:
  pyinfo_constructor(object constructor) : constructor(constructor) { }
  options_map get_options() { return options_map("You never saw this message"); }
  bool set_option(std::string, std::string) { return false; } // but this could be really handy
  template <class Grammar>
  pyinfo_factory construct(score_combiner const& sc,
			   Grammar& grammar,
			   property_map_type pmap) const {
    try {
      return pyinfo_factory(constructor(sc, ref(grammar), pmap));
    } catch( error_already_set ) {
      PyErr_Print();
      throw;
    }
  }
    
private:
  object constructor;
};

void register_python_info_factory(string name, object c) {
  cerr << "Registering Python info factory " << name << endl;
  register_info_factory_constructor(name, pyinfo_constructor(c));
}

class pyinfo_meta_constructor {
public:
  options_map get_options() { 
    options_map opts( "Python module loader options");
    
    opts.add_option("python-module", 
		    add_notifier(optvar(module_name), bind(&pyinfo_meta_constructor::module_notifier, this)),
		    "name of Python module(s) to load");
    return opts;
  }
  bool set_option(std::string, std::string) { return false; } // but this could be really handy

  void module_notifier() {
    typedef boost::tokenizer<boost::char_separator<char> > tok_t;
    tok_t tok(module_name, boost::char_separator<char>(","));
    for (tok_t::const_iterator it = tok.begin(); it != tok.end(); ++it) {
      cerr << "Loading Python module " << *it << endl;
      try {
	object module = import(it->c_str());
      } catch( error_already_set ) {
	PyErr_Print();
	throw;
      }
    }
  }
    
  template <class Grammar>
  null_info_factory construct( score_combiner const& sc
			    , Grammar& grammar
			    , property_map_type pmap ) const
  {
    return null_info_factory();
  }

private:
  string module_name;
};


////////////////////////////////////////////////////////////////////////////////

class python_reader {
public:
  python_reader(object read) : read(read) { }
  typedef object result_type;
  template <class Dictionary>
  result_type operator()(Dictionary& dict, std::string const& as_string) const {
    try {
      return read(as_string);
    } catch (error_already_set) {
      PyErr_Print();
      throw;
    }
  }
private:
  object read;
};

void register_python_property(string const &info_name, string const &property_name, object read) {
  register_rule_property_constructor(info_name, property_name, python_reader(read));
}

////////////////////////////////////////////////////////////////////////////////

void __attribute__ ((constructor)) register_pyinfo()
{
  if (!Py_IsInitialized()) {
    PyImport_AppendInittab("pysbmt", &initpysbmt);
    Py_Initialize();
    try {
      object builtin = import("__builtin__");
      scope s(builtin);
      def("register_rule_property_constructor", &register_python_property);
      def("register_info_factory_constructor", &register_python_info_factory);

      object pysbmt = import("pysbmt");

    } catch (error_already_set) {
      PyErr_Print();
      throw;
    }
  }

  register_info_factory_constructor("pyinfo", pyinfo_meta_constructor());
}

void __attribute__ ((destructor)) close_pyinfo()
{
    //printf("closing pyinfo library");
}

////////////////////////////////////////////////////////////////////////////////

// is grammar guaranteed not to change between construct() and create_info()?
