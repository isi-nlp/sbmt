// to do:
// add safety checks to Grammar.rule_property, etc.

#include <sbmt/grammar/grammar_in_memory.hpp>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/operators.hpp>

using namespace boost;
using namespace boost::python;
using namespace std;
using namespace sbmt;

typedef sbmt::detail::rule_info opaque_rule;
BOOST_PYTHON_OPAQUE_SPECIALIZED_TYPE_ID(opaque_rule)

indexed_syntax_rule const& grammar_in_mem_get_syntax(grammar_in_mem const& g, grammar_in_mem::rule_type r) {
  return g.get_syntax(r);
}

double score_combiner_getitem(score_combiner const &sc, score_combiner::feature_key_type feature) {
  return sc[feature];
}

template <typename NodeType>
class node_range {
public:
  node_range(NodeType const* begin, NodeType const* end) :
    begin(begin), end(end) { }
  size_t size() const { return end-begin; }
  NodeType getitem(size_t i) const { 
    if (i >= 0 && i < this->size())
      return begin[i]; 
    else {
      throw out_of_range("node index out of bounds");
    }
  }

private:
  NodeType const *begin, *end;
};

node_range<indexed_syntax_rule::tree_node> indexed_syntax_rule_lhs(indexed_syntax_rule const &r) {
  return node_range<indexed_syntax_rule::tree_node>(r.lhs_begin(), r.lhs_end());
}

node_range<indexed_syntax_rule::rule_node> indexed_syntax_rule_rhs(indexed_syntax_rule const &r) {
  return node_range<indexed_syntax_rule::rule_node>(r.rhs_begin(), r.rhs_end());
}


class brf_rhs_accessor {
public:
  brf_rhs_accessor(grammar_in_mem const& g, opaque_rule const* r) :
    grammar(g), rule(r) { }
  size_t size() const { return grammar.rule_rhs_size(rule); }
  indexed_token getitem(size_t i) const { 
    if (i >= 0 && i < this->size())
      return grammar.rule_rhs(rule, i); 
    else {
      throw out_of_range("node index out of bounds");
    }
  }
private:
  grammar_in_mem const& grammar;
  opaque_rule const* rule;
};

brf_rhs_accessor make_brf_rhs_accessor(grammar_in_mem const& g, opaque_rule const* r) { return brf_rhs_accessor(g, r); }

bool token_lexical(indexed_token t) { return is_lexical(t); }
bool token_virtual_tag(indexed_token t) { return is_virtual_tag(t); }

BOOST_PYTHON_MODULE(pysbmt) {
  // rules as used in the decoder

  class_<grammar_in_mem,boost::noncopyable>("Grammar")
    .def("rule_has_property", &grammar_in_mem::rule_has_property)
    .def("rule_property", &grammar_in_mem::rule_property<object>, return_value_policy<copy_const_reference>())
    .def("get_syntax", &grammar_in_mem_get_syntax, return_internal_reference<>())
    .def("is_complete_rule", &grammar_in_mem::is_complete_rule)
    .def("label", &grammar_in_mem::label, return_value_policy<copy_const_reference>())
    .def("rule_root", &grammar_in_mem::rule_lhs)
    .def("rule_rhs", &make_brf_rhs_accessor)
    ;

  def("is_lexical", &token_lexical);
  def("is_virtual_tag", &token_virtual_tag);

  opaque<opaque_rule>();

  class_<property_map_type>("PropertyMap")
    .def(map_indexing_suite<property_map_type>())
    ;

  class_<brf_rhs_accessor>("BRFRHS", no_init)
    .def("__len__", &brf_rhs_accessor::size)
    .def("__getitem__", &brf_rhs_accessor::getitem)
    ;

  // xRS rules

  // make rhs and lhs_preorder random-access
  class_<node_range<indexed_syntax_rule::rule_node> >("RuleNodeRange", no_init)
    .def("__len__", &node_range<indexed_syntax_rule::rule_node>::size)
    .def("__getitem__", &node_range<indexed_syntax_rule::rule_node>::getitem)
    ;

  class_<node_range<indexed_syntax_rule::tree_node> >("TreeNodeRange", no_init)
    .def("__len__", &node_range<indexed_syntax_rule::tree_node>::size)
    .def("__getitem__", &node_range<indexed_syntax_rule::tree_node>::getitem)
    ;
  
  class_<indexed_syntax_rule>("SyntaxRule")
    .def(self_ns::str(self))
    //.def("rhs", range<return_internal_reference<> >(&indexed_syntax_rule::rhs_begin, &indexed_syntax_rule::rhs_end))
    .add_property("rhs", &indexed_syntax_rule_rhs)
    //.def("lhs_preorder", range<return_internal_reference<> >(&indexed_syntax_rule::lhs_begin, &indexed_syntax_rule::lhs_end))
    .add_property("lhs_preorder", &indexed_syntax_rule_rhs)
    .add_property("lhs_root", make_function(&indexed_syntax_rule::lhs_root, return_internal_reference<>()))
    ;

  class_<indexed_syntax_rule::rule_node>("RuleNode")
    .def("lexical", &indexed_syntax_rule::rule_node::lexical)
    .def("indexed", &indexed_syntax_rule::rule_node::indexed)
    .add_property("token", make_function(&indexed_syntax_rule::rule_node::get_token, return_value_policy<copy_const_reference>()))
    .def_readonly("lhs_preorder_position", &indexed_syntax_rule::rule_node::lhs_pos)
    ;

  class_<indexed_syntax_rule::tree_node>("TreeNode")
    .def("lexical", &indexed_syntax_rule::tree_node::lexical)
    .def("indexed", &indexed_syntax_rule::tree_node::indexed)
    .add_property("token", make_function(&indexed_syntax_rule::tree_node::get_token, return_value_policy<copy_const_reference>()))
    .def("is_leaf", &indexed_syntax_rule::tree_node::is_leaf)
    .def("children", range<return_internal_reference<> >(&indexed_syntax_rule::tree_node::children_begin,
			   &indexed_syntax_rule::tree_node::children_end))
    .add_property("rhs_position", &indexed_syntax_rule::tree_node::index)
    ;

  class_<indexed_token>("Token")
    ;

  // feature weight vector

  class_<score_combiner>("ScoreCombiner")
    .def("__getitem__", &score_combiner_getitem)
    ;
}

