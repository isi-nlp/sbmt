#include "gusc/lattice/ast.hpp"

#include <iostream>
#include <cstdio>
#include <boost/python.hpp>

using namespace std;
using namespace boost;
using namespace boost::python;
using namespace gusc;

#include "lib/lattice_c.cpp" // I need this for syncbuf. Q: can we separate syncbuf into its own source file so I don't have to do this?

// __str__
string lattice_ast_str(const lattice_ast &lat) {
  ostringstream out;
  out << lat;
  return out.str();
}

void lattice_read(lattice_ast &lat, PyObject *file) {
  FILE *fp = PyFile_AsFile(file);
  syncbuf buf(fp);
  std::istream is(&buf);
  getlattice(is, lat);
  if (is.bad()) {
    PyErr_SetString(PyExc_RuntimeError, "error parsing lattice");
    //PyErr_SetString(PyExc_EOFError, "end of file or error parsing lattice");
    throw_error_already_set();
  }
  if (is.eof() || is.fail()) {
    PyErr_SetString(PyExc_EOFError, "end of file"); // this is not the normal semantics of EOFError
    throw_error_already_set();
  }
}

const string& property_getitem(const property_container_interface::key_value_pair &kv, int i) {
  if (i == 0)
    return kv.key();
  else if (i == 1)
    return kv.value();
  else {
    PyErr_SetString(PyExc_IndexError, "index out of bounds");
    throw_error_already_set();
    return kv.value(); //never executes; avoid warning
  }
}

size_t vertex_pair_getitem(const lattice_vertex_pair &vp, int i) {
  if (i == 0)
    return vp.from();
  else if (i == 1)
    return vp.to();
  else {
    PyErr_SetString(PyExc_IndexError, "index out of bounds");
    throw_error_already_set();
    return vp.to(); //never executes; avoid warning
  }
}

template <typename T>
void add_property(T &c, std::string const& key, std::string const& value) {
  c.insert_property(key, value);
}

lattice_ast::vertex_info& new_vertex_info(lattice_ast &c, size_t id, std::string const& lbl) {
  return *c.insert_vertex_info(id, lbl);
}

template <typename T>
lattice_line& new_edge(T &c, size_t from, size_t to, std::string const& lbl) {
  return *c.insert_edge(lattice_vertex_pair(from, to), lbl);
}

template <typename T>
lattice_line& new_block(T &c) {
  return *c.insert_block();
}

// already defined in lattice_c.cpp:
// typedef pair<lattice_ast::const_vertex_info_iterator,lattice_ast::const_vertex_info_iterator> vertex_info_range;
// etc.

BOOST_PYTHON_MODULE(lattice) {
  //// properties

  class_<property_container_interface::key_value_pair>("Property", no_init)
    .add_property("key", make_function(&property_container_interface::key_value_pair::key, return_value_policy<copy_const_reference>()))
    .add_property("value", make_function(&property_container_interface::key_value_pair::value, return_value_policy<copy_const_reference>()))
    .def("__getitem__", &property_getitem, return_value_policy<copy_const_reference>())
    ;

  class_<property_range>("PropertyRange")
    .def("__iter__", boost::python::range(&property_range::first, &property_range::second))
    ;

  //// vertex info
  //// contains properties

  class_<lattice_ast::vertex_info>("VertexInfo", no_init)
    .def("properties", (property_range (property_container::*)())&property_container::properties)
    .def("add_property", (void (*)(lattice_ast::vertex_info&, std::string const&, std::string const&))&add_property)

    .add_property("label", make_function(&lattice_ast::vertex_info::label, return_value_policy<copy_const_reference>()))
    .add_property("id", &lattice_ast::vertex_info::id)
    ;

  class_<vertex_info_range>("VertexInfoRange")
    .def("__iter__", boost::python::range(&vertex_info_range::first, &vertex_info_range::second))
    ;

  class_<lattice_vertex_pair>("VertexPair", no_init)
    .add_property("frm", &lattice_vertex_pair::from)
    .add_property("to", &lattice_vertex_pair::to)
    .def("__getitem__", &vertex_pair_getitem)
    ;

  //// lattice line
  //// can either be a block or an edge
  //// contains properties
  //// contains lines

  class_<lattice_line>("Line")
    .def(init<>())
    .def("properties", (property_range (lattice_line::*)())&lattice_line::properties)
    .def("add_property", (void (*)(lattice_line&, std::string const&, std::string const&))&add_property)

    .def("lines", (line_range (lattice_line::*)())&lattice_line::lines)
    .def("new_edge", (lattice_line&(*)(lattice_line&, size_t, size_t, std::string const&))&new_edge, return_internal_reference<>())
    .def("new_block", (lattice_line&(*)(lattice_line&))&new_block, return_internal_reference<>())

    .def("is_block", &lattice_line::is_block)
    .add_property("span", &lattice_line::span)
    .add_property("label", make_function(&lattice_line::label, return_value_policy<copy_const_reference>()))
    ;

  class_<line_range>("LineRange")
    .def("__iter__", boost::python::range(&line_range::first, &line_range::second))
    ;

  //// lattice
  //// is a lattice node
  //// contains vertex_infos

  class_<lattice_ast>("AST")
    .def(init<>())
    .def("__str__", &lattice_ast_str)
    .def("read", &lattice_read)

    .def("properties", (property_range (lattice_ast::*)())&lattice_ast::properties)
    .def("add_property", (void (*)(lattice_ast&, std::string const&, std::string const&))&add_property)

    .def("lines", (line_range (lattice_ast::*)())&lattice_ast::lines)
    .def("new_edge", (lattice_line&(*)(lattice_ast&, size_t, size_t, std::string const&))&new_edge, return_internal_reference<>())
    .def("new_block", (lattice_line&(*)(lattice_ast&))&new_block, return_internal_reference<>())

    .def("vertex_infos", (vertex_info_range(lattice_ast::*)())&lattice_ast::vertex_infos)
    .def("new_vertex_info", &new_vertex_info, return_internal_reference<>())
    ;
}
