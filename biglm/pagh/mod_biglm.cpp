#include "Python.h"

#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>

#include "biglm.hpp"

using namespace std;
using namespace boost::python;


BOOST_PYTHON_MODULE(biglm)
{
  class_<biglm,boost::noncopyable>("BigLM", init<const string &>())
    .def("lookup_word", &biglm::lookup_word)
    .def("get_order", &biglm::get_order)
    .def("lookup_ngram", &biglm::lookup_ngram)
    ;
}
