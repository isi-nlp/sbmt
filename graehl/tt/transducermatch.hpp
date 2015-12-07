// top-down matching of a tree-trie of patterns(tree prefixes) against a subtree.  see jonathan@graehl.org for explanation.
#ifndef _transducermatch_HPP
#define _transducermatch_HPP

#include <graehl/tt/transducergraph.hpp>
#include <graehl/shared/treetrie.hpp>
#include <graehl/shared/hypergraph.hpp>


#if 0

// from treetrie.hpp - resolve:   //// 1/27/2006: can't remember WHY anyone inserting would want to play with an F callback - not used for anything internally, just exposes impl. detail - commented out with /* */ - insert(t,f)

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>

namespace graehl {


const char *match_rtg=
"{start=0 rules=("
" 0 .01 => f"
" 1 => q(`3 g `3 `2)"
" 3 => a(`2)"
" 3 .1 => f"
" 2 .8=> f"
" 1 .5=> `3"
" 4 => a(`4 f)"
" 4 => b(`5 `1 e `1)"
" 5 => b(`6 `2)"
" 6 => c(`4 `5)"
")}";

template <class R>
struct var_where {
    unsigned max_new_var;
    array<unsigned> new_var;
    var_where(unsigned n) : new_var(n), max_new_var(0) {}
    typedef Tree<typename R::Llabel> *mapped_type;
    typedef unsigned key_type;
    void operator()(key_type where,const mapped_type t) {
//    (*this)(key)=t;
      typename R::Llabel &l=t->label;
      if (l.is_var()) {
          if (where > max_new_var)
              max_new_var = where;
          new_var[l.var] = where;
          l.var = where;
      }
    }
    void operator()(typename R::Rlabel &r) {
      r.var = new_var[r.var];
    }
    void operator()(Source &r) {
      r.var = new_var[r.var];
    }
    void modify_var(R *r) {
//      for (Source *s=r->sources.begin(),e=r->sources.end();s!=e;++s)
      //        s->var = new_var[s->var];
      enumerate(r->sources,ref(*this));
      r->rhs.visit(ref(*this));
    }
};

BOOST_AUTO_UNIT_TEST( TEST_treetrie )
{
  TT tt(match_rtg);
  BOOST_CHECK(tt.valid());

  typedef treetrie<test_counter> Trie;
  Trie trie;
  graph_object<TT,hyperarc_tag>::iterator_pair rp=begin_end(hyperarc_tag,tt);
  var_where<TT::Rule> revar(tt.var_size());
  dynamic_array<dynamic_array<TT::Rule *> > matching_rules;
  for (;rp.first!=rp.second;++rp.first) {
      TT::Rule *rule=*rp.first;
      Trie::V node=trie.insert(&rule->lhs,boost::ref(revar));
      matching_rules(node).push(rule);
  }
//  DBP(tt);
  //BOOST_CHECK(false);

}

}//ns


#endif

#endif
#endif
