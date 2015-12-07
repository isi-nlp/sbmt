// tree-to-tree and tree-to-string transducer - no algorithms except I/O
#ifndef _TRANSDUCER_HPP
#define _TRANSDUCER_HPP

#include <graehl/shared/symbol.hpp>
#include <graehl/shared/tree.hpp>
#include <graehl/shared/string.hpp>
#include <graehl/shared/weight.h>
#include <graehl/shared/strhash.h>
#include <graehl/shared/dynarray.h>
#include <graehl/shared/genio.h>
#include <iostream>
#include <graehl/tt/ttconfig.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <graehl/shared/hypergraph.hpp>
#include <graehl/shared/byref.hpp>
#include <graehl/shared/property.hpp>
#include <graehl/shared/treetrie.hpp>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

/*
  template <class R>
  struct Treexdcr;
  template <class S,class R>
  struct Xrule;
*/
// Treexdcr is an ordered list of rules (with a state name alphabet that translates state symbol->index)
// rule: Xrule<InputAlph, Rhs>
// where Rhs = Trhs<OutputAlph> or Srhs<OutputAlph>
// (lhs is always a tree of patterns on symbol/rank and sometimes capturing a subtree into a variable index): Tree<Llab>
// for Trhs, you have a tree of Rlab, output alphabet symbols mixed with some (leaves only) pairs of variable index/state
// for Srhs, you have a string of Rlab
// derivation grammars have a Tree RHS (always) of pointers to rules in the transducer they come from.
// we intend to treat grammars as transducers taking any tree input to their (CFG|RTG) outputs
// we intend to do forward application by restricting the derivation transducer to a particular input tree
// a function can transform a derivation tree to an output tree/string if we ever want to; until we cascade, there is no reason to except to make users happy
// Input/Output: Llab and Rlab can be printed without requiring the transducer holding them (Symbols are stored independently, although Derivation Tree RHS do need their source xdcr to still be allocated
// Llab can be read independently, but Rlab requires the state name dictionary to cache the state index.  this could be postponed, though, since other types of indexing will probably be simplest to do as a second pass rather than on read.

// INDEXING: it's intended that indices be built from the list of rules as
// needed (different indices required for forward/backward application, etc
// ... training (restricting to input,output pair) probably could use a joint
// index but will likely just use input index and scan over matches to check rhs
// applicability

// TT typedef = tree-tree (Symbol in/out).  TT::Deriv = derivation-output-tree for TT
// TS typedef = tree-string (Symbol in/out).  TS::Deriv = derivation-output-tree for TS

// OBSOLETE a template typedef struct Xtype allows short naming of the
// transducer/derivation transducer for a given input/output alphabet.  TT and
// TS are aliases for tree-tree and tree-string on Symbols.  e.g. TT::Xdcr,
// TT::Rule, TT::Deriv
namespace graehl {

typedef Symbol StateName;
typedef Alphabet<StateName> StateNames;

template <class S=Symbol>
struct Llab {

  typedef S Symbol;
private:
  Symbol s;

  enum named { ANYTREE=-2, ANYRANK=-1 };
  rank_type r; // ignored if var==0.  if r==ANYRANK, any rank allowed, otherwise
               // rank must ==r (meaningful only for leaves but could be
               // extended to not care about additional children to right for
               // interior nodes.  if ANYTREE, also allow any symbol.  if var==0
               // then r=0
public:
  var_type var; // 0 = no capture, 1,...,n = consecutive variable indicies
  Symbol & symbol() {
    Assert(!(is_var() && any_tree()));
    return s;
  }
  Symbol symbol() const {
    Assert(!(is_var() && any_tree()));
    return s;
  }
  template<class T>
  bool operator==(const Llab<T> &o) const {
    return o.var==var &&
      (var ? (o.r==r&&(r==ANYTREE||o.s==s))
       : o.s==s);

  }
  // LHS symbol stuff:
  var_type is_var() const {
    return var;
  }
  var_type var_index() const {
    return var-1;
  }
  void make_any_rank() {
    rank()=ANYRANK;
  }

  void make_any_tree(){
    rank()=ANYTREE;
  }

  bool any_rank() const {
    return r<0;
  }
  bool any_tree() const {
    return rank()==ANYTREE;
  }
  bool any_symbol() const {
    return any_tree();
  }
  bool is_trivial() const {
    return var==1 && any_tree();
  }
  void make_trivial() {
    var=1;rank()=ANYTREE;
  }

  rank_type &rank() {
    return r;
  }
  rank_type rank() const {
    return r;
  }

  bool rank_match(unsigned rank) const {
    //!FIXME: is this used or even right?  think it's used by treetrie
    return any_rank() || rank == this->rank();
  }
  bool match(Symbol sym) const {
    return s==sym;
  }
  struct labelrank_matcher{
    bool operator()(const shared_tree<Symbol> &tree,const shared_tree<Llab> &pat)  const {
      return tree.any_tree() || (tree.any_rank()||tree.size()==pat.label.rank()) &&
        pat.label.symbol() = tree.label;
    }
  };
  /*  typedef void has_print;
  template <class charT, class Traits>
  std::ios_base::iostate
  print(std::basic_ostream<charT,Traits>& o=std::cerr) const*/
  GENIO_print
  {
    if (is_var()) {
      if (var > 1)
        o << '$' << var;
      if (!any_tree()) {
        o << ':' << symbol();
        if (!any_rank())
          o << '^' << rank();
      }
    } else {
      o << symbol();
    }
    return GENIOGOOD;
  }
  GENIO_read
  {
    var=r=0;
    char c;
    EXPECTI_COMMENT_FIRST(in >> c);
    if (c==':') {
      var=1;
      goto lookahead;
    } else if (c=='$') {
      EXPECTI(in >> var);
      EXPECTI(in.get(c));
      if (c==':') {
      lookahead:
        EXPECTI(in >> symbol());
        EXPECTI(in.get(c));
        make_any_rank();
        if (c=='^') {
          EXPECTI(in.get(c));
          if (c!='*') {
            in.unget();
            EXPECTI(in >> rank());
          }
        } else {
          in.unget();
        }
      } else {
        in.unget();
        make_any_tree();
      }
    } else {
      in.unget();
      EXPECTI(in >> symbol());
    }
    return GENIOGOOD;
  fail:
    return GENIOBAD;
  }
};

template <class charT, class Traits,class S>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, Llab<S> &arg)
{
  return gen_extractor(is,arg);
}

template <class charT, class Traits,class S>
std::basic_ostream<charT,Traits>&
operator <<
  (std::basic_ostream<charT,Traits>& os, const Llab<S> &arg)
{
  return gen_inserter(os,arg);
}

/*
  CREATE_INSERTER(Llab<Symbol>)

  CREATE_EXTRACTOR(Llab<Symbol>)


  template <class S>
  struct LhsLabReader {
  typedef S value_type;
  template <class charT, class Traits>
  std::basic_istream<charT,Traits>&
  operator()(std::basic_istream<charT,Traits>& in,Xlab<S> &x) const {
  in >> x.label();
  return in;
  }
  };
*/

template <class S,class charT, class Traits>
inline   std::ios_base::iostate
print_source(unsigned var, S state, std::basic_ostream<charT,Traits>& o=std::cerr) {
  if (var > 1)
    o << '$' << var;
  o << '`' << state;
  return GENIOGOOD;
}

struct Source {
  var_type var;
  unsigned state;
  Source(var_type var_,unsigned state_):var(var_),state(state_) {}
  /*        template <class R>
            Source(R &l) : var(l.var),state(l.state) {
            Assert(var>0);
            }*/
  GENIO_print
  {
    return print_source(var,state,o);
  }
};
typedef dynamic_array<Source> Sources;
// Sources are needed to efficiently enumerate hyperarcs.


CREATE_INSERTER(Source);

struct RhsReaderContext {
  StateNames &stnames;
  var_type &max_var;
  unsigned &total_sources;
  RhsReaderContext(StateNames &s,var_type &v,unsigned &t) : stnames(s),max_var(v),total_sources(t) {}
};

template <class S=Symbol>
struct Rlab {
  typedef S Symbol;
  Symbol s;
  var_type var; // 0 = non-variable leaf, 1,...,n = consecutive variable indicies.  if >0 then state is defined
  unsigned state; // redundant with s but faster than a hash lookup

  template<class T>
  bool operator==(const Rlab<T> &o) const {
    return var==o.var &&
      s==o.s;
  }

  // RHS symbol stuff:
  Symbol & symbol() { return s; }
  Symbol symbol() const { return s; }
  bool is_unknown_state() {
    return state==(unsigned)-1;
  }
  void make_unknown_state() {
    state=(unsigned)-1;
  }
  unsigned state_index() const { return state; }
  Symbol state_name() const { return s; }
  var_type is_var() const {
    return var;
  }
  var_type any_rank() const {
    return is_var();
  }
  var_type rank() const {
    return 0; // for non any_rank() leaves only, really
  }
  var_type any_symbol() const {
    return is_var();
  }
  unsigned var_index() const {
    return var-1;
  }
  bool match(Symbol sym) const {
    return var || s==sym;
  }
  GENIO_print
  {
    if (is_var()) {
      print_source(var,s,o);
    } else {
      o << s;
    }
    return GENIOGOOD;
  }
  GENIO_read
  {
    char c;
    EXPECTI_FIRST(in >> c);
    var=0;
    if (c=='$') { // var
      EXPECTI(in >> var);
      if (var < 1)
        goto fail;
      EXPECTCH('`'); // expect state next
      goto state;
    } else if (c=='`') { // state
    state:
      if (!is_var())
        var=1; // default if omitted, e.g. RTG
      EXPECTI(in >> s);
      make_unknown_state();
    } else {
      in.unget();
      EXPECTI(in >> s);
    }
    return GENIOGOOD;
  fail:
    return GENIOBAD;
  }
  struct Reader {
    typedef Rlab<S> value_type;
    RhsReaderContext &context;
    /*
      StateNames &stnames;
      var_type &max_var;
      unsigned &total_sources;
    */

    Sources &sources;
    Reader(RhsReaderContext &context_,Sources &s) : context(context_), sources(s) {}

    template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
    operator()(std::basic_istream<charT,Traits>& in,value_type &x) const {

      /*         char c;
                 EXPECTI(in >> c);
                 x.var=0;
                 if (c=='$') { // var
                 EXPECTI(in >> x.var);
                 EXPECTCH('`'); // expect state next
                 } else if (c=='`') { // state
                 if (!x.var)
                 x.var=1; // default if omitted, e.g. RTG
                 state:
                 EXPECTI(in >> x.s);
                 x.state=stnames.index_of(x.s);
                 } else {
                 in.unget();
                 EXPECTI(in >> x.s);
                 }
                 return in;
                 fail:
                 GENIOSETBAD(in);
                 return in;
      */

      if ((in >> x).good()
          //&& !x.s.isDefault() // don't allow empty symbol
          ) {
        if (x.is_var()) {
          if(x.var > context.max_var )
            context.max_var = x.var;
          x.state=context.stnames.index_of(x.s);
          /*                 sources.push_back_raw();
                             PLACEMENT_NEW(&sources.back()) Source(*this);*/
          sources.push_back(Source(x.var,x.state));
          ++context.total_sources;
        }
      } else {
        GENIOSETBAD(in);
      }
      return in;
    }
  };

};


template <class charT, class Traits,class S>
std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, Rlab<S> &arg)
{
  return gen_extractor(is,arg);
}

template <class charT, class Traits,class S>
std::basic_ostream<charT,Traits>&
operator <<
  (std::basic_ostream<charT,Traits>& os, const Rlab<S> &arg)
{
  return gen_inserter(os,arg);
}


// XXX: ignore this micro-optimization crap: stuff for leaves only:

/*
// explicit state member in Rlab now; since strings don't have that extra leaf_data (although could give it to them)
template <class S>
inline unsigned &rhs_state(const shared_tree<Rlab<S> > *t) {
Assert(t->label.var!=0);
return t->leaf_data<unsigned>();
}

template <class S>
inline unsigned &rhs_state(const String<Rlab<S> > *t) {
Assert(t->label.var!=0);
return t->leaf_data<unsigned>();
}

*/

/*
  template <class S>
  inline unsigned &rhs_state(const shared_tree<Rlab<S> > *t) {
  Assert(t->label.var!=0);
  return t->label.state;
  }
*/


/*
  also explicit state member in Llab since don't want to provide interface for writing leafdata on input through Reader
  // <0 = any rank allowed, >=0: only that rank (#children) allowed
  template <class S>
  inline int &lhs_allowed_rank(const shared_tree<Llab<S> > *t) {
  return t->leaf_data<int>();
  }
  template <class S>
  inline bool lhs_rank_match(const shared_tree<Llab<S> > *t,unsigned rank) {
  if (t->size() && t->size()!=rank)
  return false;
  int a=lhs_allowed_rank(t);
  return a<0 || a==rank;
  }

*/


/*
  template <class F>
  struct is_var_functor : public F {
  void operator()(typename F::argument_type arg) {
  if (arg.is_var())
  F::operator()(arg);
  }
  };
*/

//rhs of tree-tree transducers
template <class S>
struct Trhs : public shared_tree<Rlab<S> > {
  typedef S Symbol;
  typedef Rlab<Symbol> Label;
  typedef shared_tree<Label> Parent;
  // using inheritance ... //  shared_tree<Label> tree;
  // private member = tree or string of Xlab
  // get list of (unique state, count) pairs (for pruning/k-best)
  // get list of states for a given variable index
  // substitute input-subtree-array (or RTG-state-X-lookahead) into variables, returning resulting rhs
  // substitute result of a functor(state,index) into tree or string, returning new tree/string
  // match output (against tree or string, depending) - different operations entirely!  (string match is ambiguous)
  template <class F>
  void visit(F f)
  {
    //      for (iterator i=begin(),e=end();i!=e;++i) {
    if (this->size()) {
        //FOREACH(Parent *child,*this) {
        for (typename Parent::iterator i=this->begin(),e=this->end();i!=e;++i) // each child
            reinterpret_cast<Trhs<S> *>(*i)->visit(f);
    } else {
      deref(f)(this->label);
    }

  }
  ~Trhs() {
    this->dealloc_recursive();
  }
};


//rhs of tree-string transducers
template <class S>
struct Srhs : public String<Rlab<S> > {
  typedef S Symbol;
  typedef Rlab<Symbol> Label;
  typedef String<Label> Parent;
  // using inheritance ... //Array<Label> string;
  template <class F>
  void visit(F f) {
      //FOREACH(Label &l,*this) {
      for (typename Parent::iterator i=this->begin(),e=this->end();i!=e;++i) // each label
          deref(f)(*i);
  }
  ~Srhs() {
    this->destroy();
    this->dealloc();
  }
};


template <class S, class R = Trhs<S> >
struct Xrule {
  typedef Xrule Rule;
  typedef S Symbol;
  typedef Llab<S> Llabel;
  typedef R Rhs;
  typedef typename Rhs::Symbol OutSymbol;
  typedef typename Rhs::Label Rlabel;
  typedef shared_tree<Llabel> Lhs;

  unsigned state;

  Lhs lhs;
  Rhs rhs;
  Weight w;
  Sources sources; // rhs variable nodes

  typedef typename Weight::cost_type cost_type;
  cost_type getCost() {
    return w.getCost();
  }
  void clear_index() {
    sources.clear();
  }
  void build_index() {
    clear_index();
    rhs.visit(*this);
  }
  void operator()(const Rlabel &l) {
    if (l.is_var())
      sources.push_back(l);
  }
  template<class S2,class R2>
  bool operator==(const Xrule<S2,R2> &o) const {
    return state==o.state &&
      w==o.w &&
      lhs==o.lhs &&
      rhs==o.rhs;
  }
  ~Xrule() {
    lhs.dealloc_recursive();
  }

  Weight weight() const{ return w;}
  bool is_lhs_trivial() const {
    return lhs.rank==0 && lhs.label.is_trivial();
  }
  void make_lhs_trivial() {
    lhs.label.make_trivial();
    lhs.set_rank(0);
  }
  bool is_weight_trivial() const {
    return w.isOne(); // could use a margin rather than exact compare
  }

  // typedef XRuleReader<S,R> Reader;
  struct Reader : public RhsReaderContext {
    typedef Xrule<S,R> value_type;
    //typedef Xrule<S,R> value_type;


    Reader(StateNames &s,var_type &v,unsigned &t) : RhsReaderContext(s,v,t) {}


    template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
    operator()(std::basic_istream<charT,Traits>& in,value_type &x)  {
      //XXX: warning: had copying of stnames until changed template parameter for 3-arg gen_ to reference rather than value!
      GEN_EXTRACTOR(in,x.read(in,*this));
      //return gen_extractor(in,x,boost::ref(stnames),boost::ref(max_var),boost::ref(total_sources));
      x.sources.compact();
      return in;
    }
  };   //Reader

  struct Writer {
    typedef Xrule<S,R> value_type;
    const StateNames &stnames;
    Writer(const StateNames &s) : stnames(s) {}

    template <class charT, class Traits>
    std::basic_ostream<charT,Traits>&
    operator()(std::basic_ostream<charT,Traits>& o,const value_type &x) const {
      //XXX: warning: had copying of stnames until changed template parameter for 3-arg gen_ to reference rather than value!
      GEN_INSERTER(o,x.print(o,stnames));
      return o;
      //return gen_inserter(o,x,stnames);
    }
  };


  /*  template <class charT, class Traits>
  std::ios_base::iostate
  print(std::basic_ostream<charT,Traits>& o=std::cerr) const
  {
    o << state;
    return print_rest(o);
    }*/
  GENIO_print {
    o << state;
    return print_rest(o);
  }
  template <class charT, class Traits>
  std::ios_base::iostate print(std::basic_ostream<charT,Traits>& o,const StateNames &stnames) const
  {
    //Assert(stnames.is_index(state));
    o << stnames[state];
    return print_rest(o);
  }
  template <class charT, class Traits>
  std::ios_base::iostate print_rest(std::basic_ostream<charT,Traits>& o) const
  {
    if (!is_lhs_trivial()) {
      o << " ; ";
      o << lhs;
    }
    if (!is_weight_trivial())
      o << ' ' << weight();
    o << " => ";
    o << rhs;
    return GENIOGOOD;
  }

  template <class charT, class Traits>
  std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader &read)
    //StateNames &stnames,var_type &max_used_var,unsigned &total_sources)
  {
    clear_index();
    char c;
    Symbol statename;
    // (LHS) state
    EXPECTI_FIRST(in >> statename);
    state=read.stnames.index_of(statename);

    // optional lhs (omit for Grammar)
    EXPECTI(in >> c);
    if (c==','||c==';') {
      EXPECTI(in>>lhs);
    } else {
      in.unget();
      make_lhs_trivial();
    }

    // optional weight
    EXPECTI_COMMENT(in >> c);
    in.unget();
    if (c!= '=') {
      EXPECTI(in >> w);
    } else {
      w.setOne();
    }

    // => (rhs follows)
    EXPECTCH_SPACE_COMMENT('=');
    EXPECTCH('>');

    // (rhs)
    //      EXPECTI(in >> rhs);
    {
      typename Rlab<OutSymbol>::Reader rlabr(read,sources);
      return
        rhs.read(in,boost::ref(rlabr));
    }
    //build_index(); // could integrate that into Reader // just did!

  fail:
    return GENIOBAD;
  }

  bool match_input(const shared_tree<Symbol> &tree) {
    return tree_contains(lhs,tree,Lhs::labelrank_matcher);
  }

};

template <class charT, class Traits,class S,class R>
inline std::basic_ostream<charT,Traits>&
operator <<
  (std::basic_ostream<charT,Traits>& o, const Xrule<S,R> &arg)
{
  return gen_inserter(o,arg);
}


// default = tree-tree transducer
// a tree grammar is a tree-tree transducer with default lhs (input-epsilon) and variable nodes in rhs all using variable 1
// same for context free grammar but tree-string transducer
// (any input tree transformed to/generates the grammar)
template <class R=Xrule<Symbol> >
struct Treexdcr {
  bool operator ==(const Treexdcr<R> &r) const {
    return r.rules == rules && r.start == start && r.stnames == stnames;
  }
  Treexdcr()
    //: itail(NULL),ihead(NULL)
  {
    make_invalid();
  }
  explicit Treexdcr(const char *c)
  {
    make_invalid();
    std::istringstream ic(c); ic >> *this;
  }
  ~Treexdcr() {
    make_invalid();
  }
  typedef Treexdcr<R> Self;
  typedef R Rule;
  typedef typename Rule::Symbol Symbol;
  typedef typename Rule::Lhs Lhs;
  typedef typename Rule::Rhs Rhs;
  typedef typename Rule::OutSymbol OutSymbol;
  typedef Treexdcr<Xrule<Symbol,Trhs<Rule *> > >  Deriv; // derivation wRTG

  typedef dynamic_array<Rule> Rules;

  var_type max_used_var; // indexed in rhs of rules by $1,...,$max_used_var
    unsigned var_size() const { return max_used_var+1; }
  Rules rules;
  StateNames stnames;
  unsigned total_sources;
  unsigned start; // start state index, (unsigned)-1 if none
  unsigned n_states() const {
    return stnames.size();
  }
  unsigned n_rules() const {
    return rules.size();
  }
  bool valid() const {
    return start != (unsigned)-1;
  }
  void make_invalid() {
    free_tail_index();
    free_head_index();
    start = (unsigned)-1;
  }
  // invalidates indexes, and derivation trees (pointers to rules change) ... could use linked list instead but then would need a hashtable or an indirect array to associate values with rules, e.g. training (or a void * member instead)
  void remove_marked_states(bool remove[]) {
    //FIXME:
  }
  void clear() {
    make_invalid();
    rules.clear();
    stnames.clear();
    max_used_var=0;
    total_sources=0;
  }
  /*  template <class charT, class Traits>
  std::ios_base::iostate
  print(std::basic_ostream<charT,Traits>& o=std::cerr) const
  */
    GENIO_print
  {
    //Assert(valid());
    if (!valid()) {
      o << "{()}\n";
    }
    //begin
    o << "{\n";

    o << "nstates = " << n_states() << "\n";
    o << "nrules = " << n_rules() << "\n";
    //optional list of (other) states
    o << "states = ";
    //Array<StateName> sn(stnames.symbols().begin()+1,stnames.symbols().end()); // the other states
    o << stnames.symbols() << "\n";

    //initial(root) state
    o << "start = " << stnames[start] << "\n";

    // rules
    o << "rules = ";
    gen_inserter(o,rules,typename Rule::Writer(stnames),(unsigned)Rules::MULTILINE);

    //end
    o << '}' << std::endl;

    return GENIOGOOD;
  }

  // use boost::make_function_output_iterator(StateNamePrimer(stnames)) instead
  struct StateNamePrimer {
    StateNames &stnames;
    explicit StateNamePrimer(StateNames &s_) : stnames(s_) {}
    //        StateNamePrimer(StateNamePrimer &s) : stnames(s.stnames) {}

    void operator()(StateName s) {
      (void)stnames.index_of(s);
    }
  };
  GENIO_read
  {
    char c;
    unsigned n;
    StateName stn;
    clear();

    //begin
    EXPECTCH_SPACE_COMMENT('{');

    //optional header items in form key = val (e.g. states = ( list of (some) other states )

    for(;;) {
      EXPECTI_COMMENT_FIRST(in >> c);
      switch(c) {
      case 'n':
        EXPECTI(in >> c);
        if (c=='s') { //nstates=
          EXPECTCH('t');
          EXPECTCH('a');
          EXPECTCH('t');
          EXPECTCH('e');
          EXPECTCH('s');
          EXPECTCH_SPACE_COMMENT('=');
          EXPECTI(in >> n);
          stnames.reserve(n);
        } else if  (c=='r') { //nrules=
          EXPECTCH('u');
          EXPECTCH('l');
          EXPECTCH('e');
          EXPECTCH('s');
          EXPECTCH_SPACE_COMMENT('=');
          EXPECTI(in >> n);
          rules.reserve(n);
        } else {
          Config::warn() << "Unknown attribute; expected nstates= or nrules=\n";
          goto fail;
        }
        break;
      case 's':
        EXPECTCH('t');
        EXPECTCH('a');
        EXPECTI(in >> c);
        if (c=='t') { // states=
          EXPECTCH('e');
          EXPECTCH('s');
          EXPECTCH_SPACE_COMMENT('=');
          //
          StateNamePrimer sn(stnames);
          typename boost::function_output_iterator<StateNamePrimer> namep(sn);
          if (range_read(in,
                             namep,
                             //boost::make_function_output_iterator
                             //boost::function_output_iterator<StateNamePrimer>(StateNamePrimer(stnames)),

                             DefaultReader<StateName>())==GENIOBAD)
            return GENIOBAD;
        } else if (c=='r') { // start=
          EXPECTCH('t');
          EXPECTCH_SPACE_COMMENT('=');
          //initial(root) state
          EXPECTI(in >> stn);
          start=stnames.index_of(stn);
          Assert(stnames.find(stn)&&*stnames.find(stn)==start);
        } else {
          Config::warn() << "Unknown attribute; expected states= or start=\n";
          goto fail;
        }

        break;
      case 'r':
        EXPECTCH('u');
        EXPECTCH('l');
        EXPECTCH('e');
        EXPECTCH('s');
        EXPECTCH_SPACE_COMMENT('='); // rules=
        //      in >> rules; // need to map using statenames.
        {
          typename Rule::Reader rulerd(stnames,max_used_var,total_sources);
          if(rules.read(in,
                            boost::ref(rulerd),
                            Rules::APPEND)
             != GENIOGOOD)
            goto fail;
        }
        break;
      case '}':
        if (!valid()) { // no start state
          Config::warn() << "Didn't read a valid transducer (did you specify the initial state with start= ?)\n";
          goto fail;
        }
        //          DBP("before compact");DBP(*this);
        //          if (rules.size()==0) BREAKPOINT;
        rules.compact();
        stnames.compact();
        //          DBP("after");          DBP(*this);
        return GENIOGOOD;
      default:
        //Config::warn() << "Unrecognized attribute name\n";
        goto fail;
      }
    }


    //end
  fail:
    return GENIOBAD;
  }

  void free_tail_index() {
  }
  void free_head_index() {
  }
  /*
  //FIXME:need to declare graphtraits before this!
  typedef TailsUpHypergraph<Self> TailIndex;
  TailIndex *itail;
  void free_tail_index() {
  if (itail) {
  delete itail;
  itail=NULL;
  }
  }
  void build_tail_index() {
  if (itail)
  return;
  unsigned n_r=n_rules();
  itail=new RevIndex(n_states(),n_r);
  for (int i=0;i!=n_r;++i) {
  itail->edge[i].edge=i; = rules[i].weight();
  itail->edge[i].ntails = 0;
  }
  }
  typedef R * hyperarc_descriptor;
  typedef SourceEdges<Self,hyperarc_descriptor> HeadIndex;
  HeadIndex *ihead;
  void free_head_index() {
  if (ihead) {
  delete ihead;
  ihead=NULL;
  }
  }
  void build_head_index() {
  if (ihead)
  return;
  ihead=new HeadIndex(*this);
  }
  */


#ifdef TT_TRAITS
  ///GRAPH_TRAITS STUFF
  typedef Treexdcr<R> graph;
  typedef unsigned vertex_descriptor;
  //typedef unsigned hyperarc_descriptor;
  //typedef typename graph::hyperarc_descriptor hyperarc_descriptor;
  typedef typename graph::Rule * hyperarc_descriptor;
  typedef Source *tail_descriptor;
  //  typedef OffsetMap<hyperarc_descriptor) offset_map;
  //template<class E> class edge_offset_map<E>;
  struct hyperarc_index_map : public OffsetMap<hyperarc_descriptor> {
    //typedef hyperarc_descriptor edge_descriptor;
    hyperarc_index_map(graph &g) : OffsetMap<hyperarc_descriptor>(g.rules.begin()) {}
    hyperarc_index_map(const hyperarc_index_map &o) : OffsetMap<hyperarc_descriptor>(o) {}
  };
  //typedef OffsetMap<hyperarc_descriptor> hyperarc_index_map;
  typedef boost::identity_property_map vertex_index_map;
  struct edge_descriptor {
    hyperarc_descriptor arc;
    tail_descriptor tail; // pointer into rules[rule].sources
    bool operator==(edge_descriptor r) {
      return arc==r.arc && tail==r.tail;
    }
    edge_descriptor & operator *() { return *this; }
    /*    template <class charT, class Traits>
    std::ios_base::iostate
    print(std::basic_ostream<charT,Traits>& o) const
    */
    GENIO_print
    {
      o << '"' << *arc << " :: " << *tail << '"';
      return GENIOGOOD;
    }


    //    operator hyperarc_descriptor() { return arc; }
  };
  //typedef unsigned edge_descriptor;
    typedef boost::directed_tag directed_category;
  typedef boost::allow_parallel_edge_tag edge_parallel_category;
  //      typedef edge_list_graph_tag traversal_category;
  typedef unsigned vertices_size_type;
  typedef unsigned edges_size_type;
  typedef unsigned degree_size_type;
  static vertex_descriptor null_vertex() {
    return (unsigned)-1;
  }
  typedef boost::counting_iterator<tail_descriptor> tail_iterator;
  typedef boost::counting_iterator<hyperarc_descriptor> hyperarc_iterator;
  typedef boost::counting_iterator<vertex_descriptor> vertex_iterator;
  typedef hyperarc_iterator out_hyperarc_iterator;
  /*struct traversal_category {
    operator edge_list_graph_tag() const { return edge_list_graph_tag(); }
    operator vertex_list_graph_tag() const { return vertex_list_graph_tag(); }
    };
  */
    struct traversal_category: public boost::edge_list_graph_tag, boost::vertex_list_graph_tag {};
  //      typedef typename graph::edge_iterator edge_iterator;
  struct edge_iterator {
    edge_descriptor ed;
    hyperarc_descriptor hend; // hend would almost be unnecessary if we could defer initializing t<-begin

    //          tail_descriptor t;

    //! only works for i!=end()
    bool operator !=(const edge_iterator i2) const { // only works for detecting end - sorry
      return ed.arc!=i2.ed.arc;
    }
    bool operator ==(const edge_iterator i2) const {
      return !(*this!=i2);
    }
    void operator ++() {
      ++ed.tail;
      if (!goodTail()) {
        ++ed.arc;
        find_tail();
      }
    }
    bool goodTail() {
      return ed.tail != ed.arc->sources.end();
    }
    void find_tail() {
      for (; ed.arc != hend; ++ed.arc) {
        ed.tail=ed.arc->sources.begin();
        if (goodTail())
          return;
      }
    }
    edge_descriptor operator *() const {
      return ed;
    }
    /*
      edge_iterator(hyperarc_descriptor h,hyperarc_descriptor end) :hend(end){
      ed.head=h;
      --ed.head(); advance_head();
      }*/
  };
  typedef std::pair<
    vertex_iterator,
    vertex_iterator> pair_vertex_it;

  typedef std::pair<
    tail_iterator,
    tail_iterator> pair_tail_it;
  typedef std::pair<
    hyperarc_iterator,
    hyperarc_iterator> pair_hyperarc_it;
  typedef std::pair<
    out_hyperarc_iterator,
    out_hyperarc_iterator> pair_out_hyperarc_it;
  typedef std::pair<
    edge_iterator,
    edge_iterator> pair_edge_it;
  typedef FLOAT_TYPE cost_type;
  typedef void adjacency_iterator;
  typedef void out_edge_iterator;
  typedef void in_edge_iterator;
  /*  struct hyperarc_cost_map {
      typedef boost::read_property_map_tag category;
      typedef cost_type value_type;
      typedef hyperarc_descriptor key_type;
      cost_type operator [](key_type p) {
      return p->getCost();
      }
      }*/

  ///GRAPH_TRAITS
#endif
};


typedef Treexdcr<Xrule<Symbol, Trhs<Symbol> > > TT;
typedef Treexdcr<Xrule<Symbol, Srhs<Symbol> > > TS;

template <class charT, class Traits,class S>
inline std::basic_istream<charT,Traits>&
operator >>
(std::basic_istream<charT,Traits>& is, Treexdcr<S> &arg)
{
  return gen_extractor(is,arg);
}

template <class charT, class Traits,class S>
inline std::basic_ostream<charT,Traits>&
operator <<
  (std::basic_ostream<charT,Traits>& os, const Treexdcr<S> &arg)
{
  return gen_inserter(os,arg);
}

/*
  template <class R>
  ostream & operator <<(ostream &o,const typename Treexdcr<R>::edge_descriptor &arg) {
  return gen_inserter(o,arg);
  }
*/

template <class charT, class Traits,class S>
inline std::basic_ostream<charT,Traits>&
operator <<
  (std::basic_ostream<charT,Traits>& os, typename Treexdcr<S>::edge_descriptor &arg)
{
  return gen_inserter(os,arg);
}

template <class R>
std::ostream & dbgout(std::ostream &o, typename Treexdcr<R>::edge_descriptor &arg) {
  return gen_inserter(o,arg);
}


#ifdef GRAEHL_TEST

struct Rlabcheck {
  std::ostringstream o;
  void clear() {
    o.str(std::string());
  }
  void operator()(Rlab<> &lab) {
    if (lab.is_var())
      o << lab.state_name() << ' ';
  }
};

const char *empty_rtg="{start=S rules=()}";

BOOST_AUTO_UNIT_TEST( transducer)
{
  //  TreeXdcr a,b;
  //  stringstream x;

    using namespace std;

  // made two copies of this (one for TT, one for TS) because I'm dumb (only the types and rhs strings differ, code is the same)
  {
    Rlabcheck rl;
    TT a,b,e,f;
    /*
      string sa=" xR { } ";
      string sb=" RTG { } ";
      string fa=" R TG { } ";
      string fb="1 RTG { } ";
    */
    string sa="%asdf\n{%asdf\nstart=Q %asdf\nstates=(R) rules=%asdf\n(%asdf\nQ %asdf\n.5%asdf\n=>%asdf\n T%asdf\n)%asdf\n}%asdf\n";
    //  string sa="{start=Q states=(R) rules=(Q .5=> T)}";
    string sb="{start=Q states=(R) rules=(Q .5=>$1`S   Q => V($1`R)  )} ";
    string sc="{start=Q rules=()} ";
    string sd="{start=Q states=(R) rules=()} ";
    string se="{start=Q states=(Q) rules=(Q .5=>$1`S   Q,A($2:B^*,:Q^2) => V($1`R  , B(A,$1`R)$2`Q)  )} ";
    string ee="{\nnstates = 3\nnrules = 2\nstates = (Q S R)\nstart = Q\nrules = (\n Q 0.5 => `S\n Q ; A($2:B :Q^2) => V(`R B(A `R) $2`Q)\n)\n}\n";
    string fa="  ";
    string fb=" a ";
    string fc=" {Q sate=(Q)()} ";

    BOOST_CHECK(test_extract(sc,f));
    // Config::debug() << sc << f << endl;
    BOOST_CHECK(test_extract(sd,f));
    // Config::debug() << sd << f << endl;
    f.clear();

    BOOST_CHECK(test_extract(sa,a));
//      DBP(a);
    BOOST_CHECK(a.n_states() == 2);
    BOOST_CHECK(a.n_rules() == 1);
    BOOST_CHECK(a.max_used_var == 0);
    // Config::debug() << sa<< a << endl;
    BOOST_CHECK(test_extract(sb,b));
    BOOST_CHECK(b.n_states() == 3);
    BOOST_CHECK(b.n_rules() == 2 && b.max_used_var == 1);
    // Config::debug() <<sb<< b << endl;

    BOOST_REQUIRE(test_extract(se,e));

    BOOST_CHECK(test_extract(ee,f));
    BOOST_CHECK(e==f);
    if (e.n_rules()>=2) {
              e.rules[1].rhs.visit(boost::ref(rl));
              BOOST_CHECK(rl.o.str() == "R R Q ");
    }
    //Config::debug() <<se<< e << ee;
    stringstream o;
    o << Weight::out_never_log;
    o << e;
    BOOST_CHECK(o.str() == ee);
//    DBP(o.str());  DBP(ee); DBP(f);

    BOOST_CHECK(test_extract(sb,b));
    CHECK_EXTRACT(se,e);
    FAIL_EXTRACT(fa,f);
    FAIL_EXTRACT(fb,f);
    FAIL_EXTRACT(fc,f);
  }


  // COPY 2
  {
    Rlabcheck rl;
    TS a,b,e,f;
    /*
      string sa=" xR { } ";
      string sb=" RTG { } ";
      string fa=" R TG { } ";
      string fb="1 RTG { } ";
    */
    string sa="{start=Q states=(R) rules=(Q .5=> (T))}";
    string sb="{start=Q states=(R) rules=(Q .5=>()   Q => ($1`R `S)  )} ";
    string sc="{start=Q rules=()} ";
    string sd="{start=Q states=(R) rules=()} ";
    string se="{start=Q states=() rules=(Q .5=>($1`S))rules=(   Q;A($2:B^*,:Q^2) => (V $1`R  S $2`Q  T)  )} ";
    string ee="{\nnstates = 3\nnrules = 2\nstates = (Q S R)\nstart = Q\nrules = (\n Q 0.5 => (`S)\n Q ; A($2:B :Q^2) => (V `R S $2`Q T)\n)\n}\n"
      ;
    string fa="  ";
    string fb=" a ";
    string fc=" {start=Q sate=(Q)() ";

    BOOST_CHECK(test_extract(sc,f));
    //  dbp(sc);dbp(f);
    // Config::debug() << sc << f << endl;
    BOOST_CHECK(test_extract(sd,f));
    //dbp(sc);dbp(f);
    // Config::debug() << sd << f << endl;
    f.clear();


    BOOST_CHECK(test_extract(sa,a));
    //  DBP(a);
    BOOST_CHECK(a.n_states() == 2);
    BOOST_CHECK(a.n_rules() == 1);
    BOOST_CHECK(a.max_used_var == 0);
    // Config::debug() << sa<< a << endl;
    BOOST_CHECK(test_extract(sb,b));
    BOOST_CHECK(b.n_states() == 3);
    BOOST_CHECK(b.n_rules() == 2 && b.max_used_var == 1);
    // Config::debug() <<sb<< b << endl;

    BOOST_CHECK(test_extract(se,e));
    BOOST_CHECK(test_extract(ee,f));
    BOOST_CHECK(e==f);
    if (e.n_rules()>=2) {
                //FIXME: segfault
              e.rules[1].rhs.visit(boost::ref(rl));
              BOOST_CHECK(rl.o.str() == "R Q ");
    }
    //Config::debug() <<se<< e << ee;
    stringstream o;
    o << Weight::out_never_log;
    o << e;
    BOOST_CHECK(o.str() == ee);
    //DBP(o.str());  DBP(ee);


    BOOST_CHECK(test_extract(sb,b));
    CHECK_EXTRACT(se,e);
    FAIL_EXTRACT(fa,f);
    FAIL_EXTRACT(fb,f);
    FAIL_EXTRACT(fc,f);


  }
}
#endif

}//ns
#endif
