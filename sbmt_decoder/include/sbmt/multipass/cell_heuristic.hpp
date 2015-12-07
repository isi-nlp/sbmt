#ifndef SBMT__MULTIPASS_CELL_HEURISTIC_HPP
#define SBMT__MULTIPASS_CELL_HEURISTIC_HPP

#include <sbmt/span.hpp>
#include <sbmt/logmath.hpp>
#include <sbmt/token.hpp>
#include <sbmt/hash/oa_hashtable.hpp>
#include <graehl/shared/triangular_array.hpp>
#include <graehl/shared/maybe_update_bound.hpp>
#include <graehl/shared/intrusive_refcount.hpp>

namespace sbmt {

struct wrong_cell_heuristic : public std::exception
{
  std::string err;

  char const* what() const throw()
  {
    return err.c_str();
  }
  ~wrong_cell_heuristic() throw() {}
  wrong_cell_heuristic(unsigned cell_max,unsigned should_max)
  {
    std::stringstream s;
    s << "Cell heuristic had max span end of " << cell_max << "; current parse expects "<<should_max<<". This suggests you used the cell heuristic from a previous sentence (of a different length).";
    err=s.str();
  }
  wrong_cell_heuristic()
  {
    err="You used the cell heuristic from a previous sentence (of a different length).";
  }
};



struct cell_heuristic
  : public graehl::intrusive_refcount<cell_heuristic>
{
  typedef oa_hash_map<indexed_token,score_t> seen_nts;
  typedef graehl::triangular_array<seen_nts> spans_cont;

  typedef std::pair<span_t,indexed_token> cell_id;

  spans_cont spans;

  cell_heuristic(unsigned max_span=0) { reset(max_span); }
  cell_heuristic(span_t const& target_span) {
    reset(target_span);
// assert(target_span.left() >= 0);
  }

  bool has_target_span(span_t const& target_span) const
  {
    return target_span.right() == max_span_end();
  }

  void check_target_span(span_t const& target_span) const
  {
    unsigned t=target_span.right(),c=max_span_end();
    if (c!=t)
      throw wrong_cell_heuristic(c,t);
  }

  unsigned max_span_end() const
  {
    return spans.max_end();
  }

  span_t max_span() const
  {
    return span_t(0,max_span_end());
  }


  void allow_glue(indexed_token glue_tag,score_t outside=as_one())
  {
    for (unsigned i=1,e=max_span_end();i<=e;++i) {
      seen_nts &n=spans(0,i);
      n.insert(std::pair<indexed_token,score_t>(glue_tag,outside));
    }
  }

  template <class O>
  struct print_span
  {
    O &o;
    indexed_token_factory const& tf;
    print_span(O &o,indexed_token_factory const& tf) : o(o),tf(tf) {}
    void operator()(unsigned a,unsigned b,seen_nts &nts) const
    {
      span_t s(a,b);
      for (seen_nts::iterator i=nts.begin(),e=nts.end();
           i!=e;++i)
        o << tf.label(i->first) << s << '=' << i->second<<'\n';
    }
  };

  template <class O>
  void print (O &o,indexed_token_factory const& tf) const
  {
    spans.visit_indexed(print_span<O>(o,tf));
  }

  void reset(unsigned max_span)
  {
    spans.reset(max_span);
  }

  void reset(span_t const& target_span)
  {
// assert(target_span.left() >= 0);
    reset(target_span.right());
  }

  seen_nts & operator[](span_t const& s)
  {
    assert(s.right()<=spans.max_end() && s.right()>s.left()); //s.left()>=0 &&
    return spans(s.left(),s.right());
  }

  bool any_cells(span_t const& s)
  {
    return !(*this)[s].empty();
  }

  bool have_cell(cell_id const& c) const
  {
    return have_cell(c.first,c.second);
  }

  bool have_cell(span_t const& s,indexed_token const& t) const
  {
    return for_span(s).end() != for_span(s).find(t);
    //return NULL!=for_span(s).find_second(t);
  }

  seen_nts const& for_span(span_t const& s) const
  {
    assert(s.right()<=spans.max_end() && s.right()>s.left()); //s.left()>=0 &&
    return spans(s.left(),s.right());
  }

  typedef std::vector<cell_id> missing_cells_t;

  // note: equivs after outside_score computed (post decode, setting representative().score() to outside of that item)
  template <class Edge>
  void maybe_improve_cell(Edge const& e)
  {
    score_t s=e.outside_score();
    graehl::maybe_increase_max((*this)[e.span()].at_default(e.root(),s),s);
  }

  void maybe_improve_cell(indexed_token const& root,span_t const& span,score_t const& score)
  {
    graehl::maybe_increase_max((*this)[span].at_default(root,score),score);
  }

  typedef std::size_t size_type;


  size_type nonempty_spans() const
  {
    size_type s=0;
    for (spans_cont::iterator i=spans.begin(),e=spans.end();i!=e;++i)
      if (!i->empty())
        ++s;
    return s;
  }

  double portion_nonempty_spans() const
  {
    return (double)nonempty_spans()/(double)possible_spans();
  }

  size_type possible_spans() const
  {
    return spans.size();
  }


  size_type present_cells() const
  {
    size_type c=0;
    for (spans_cont::iterator i=spans.begin(),e=spans.end();i!=e;++i)
      c+=i->size();
    return c;
  }


private:
  cell_heuristic(cell_heuristic const& o);
  void operator=(cell_heuristic const& o);
};

typedef boost::intrusive_ptr<cell_heuristic> cell_heuristic_ptr;


}


#endif
