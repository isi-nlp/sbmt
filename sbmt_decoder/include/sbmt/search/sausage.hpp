# if ! defined(SBMT__SEARCH__SAUSAGE_HPP)
# define       SBMT__SEARCH__SAUSAGE_HPP

# include <sbmt/span.hpp>
# include <sbmt/logmath.hpp> // score_t
# include <sbmt/edge/edge.hpp>
# include <sbmt/token/indexed_token.hpp>
# include <boost/multi_index_container.hpp>
# include <boost/multi_index/hashed_index.hpp>
# include <boost/multi_index/indexed_by.hpp>
# include <boost/multi_index/member.hpp>
# include <algorithm>

namespace sbmt {
    
struct delta;
class sausage;

std::ostream& operator<<(std::ostream&, delta const&);
std::istream& operator>>(std::istream&, delta&);
std::ostream& operator<<(std::ostream&, sausage const&);
std::istream& operator>>(std::istream&, sausage&);

////////////////////////////////////////////////////////////////////////////////

struct delta {
    enum rule_class_t { top = 1
                      , tag = 2
                      , virt = 4
                      , quasi = 8
                      , terminal = 16
                      , all = top | tag | virt | quasi | terminal
                      };
    rule_class_t rule_class;
    span_t span;
    score_t beam;
    score_t beam_rule_class;
    size_t top_n;
    size_t top_n_rule_class;
    template <class Chart>
    delta( Chart const& chart
         , typename Chart::edge_type e );
    delta();
private:
    template <class Chart>
    void calc_deltas( Chart const& chart
                    , typename Chart::cell_range cells
                    , score_t scr );
    template <class ET>
    rule_class_t calc_rule_class(ET const& eq) const;
    friend std::ostream& operator<<(std::ostream&, delta const&);
    friend std::istream& operator>>(std::istream&, delta&);
};

////////////////////////////////////////////////////////////////////////////////

class sausage {
public:
    template <class Chart>
    sausage( Chart const& chart
           , typename Chart::edge_equiv_type eq );
    
    // they could be auto-generated, but id rather not have the compiler
    // generate multi-index-container operations in every translation unit.
    sausage(sausage const& other);
    sausage& operator=(sausage const& other);
    sausage();
    
    score_t beam(size_t select_types) const;
    score_t beam_rule_class(delta::rule_class_t select_type) const;
    size_t  top_n(size_t select_types) const;
    size_t  top_n_rule_class(delta::rule_class_t select_type) const;
    
    void insert(delta const& d);
private:
    typedef boost::multi_index_container<
                delta
              , boost::multi_index::indexed_by<
                    boost::multi_index::hashed_non_unique<
                        boost::multi_index::member<delta,span_t, &delta::span>
                    >
                >
            > delta_container_t;
    delta_container_t delta_container;
    template <class Chart>
    void fill(Chart const& chart, typename Chart::edge_type e);
    friend std::ostream& operator<<(std::ostream&, sausage const&);
    friend std::istream& operator>>(std::istream&, sausage&);
};

////////////////////////////////////////////////////////////////////////////////

template <class Chart>
delta::delta( Chart const& chart
            , typename Chart::edge_type e )
: beam(1.0)
, beam_rule_class(1.0)
, top_n(0)
, top_n_rule_class(0)
{
    rule_class = calc_rule_class(e);
    span = e.span();
    calc_deltas(chart, chart.cells(span), e.score());
}

////////////////////////////////////////////////////////////////////////////////

template <class Chart> void
delta::calc_deltas( Chart const& chart
                  , typename Chart::cell_range cells
                  , score_t score )
{
    typename Chart::cell_iterator citr = cells.begin(), cend = cells.end();
    for (; citr != cend; ++citr) {
        typename Chart::edge_range edges = 
                    chart.edges(citr->root(), citr->span());
        typename Chart::edge_iterator eitr = edges.begin(), eend = edges.end();
        for (; eitr != eend; ++eitr) {
            score_t edge_score = eitr->representative().score();
            if (edge_score >= score) {
                ++top_n;
                beam = std::min(beam,score / edge_score);
                if (calc_rule_class(eitr->representative()) == rule_class) {
                    ++top_n_rule_class;
                    beam_rule_class = std::min( beam_rule_class
                                              , score / edge_score );
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
delta::rule_class_t delta::calc_rule_class(ET const& e) const
{
    indexed_token root = e.root();
    if (is_lexical(root)) return terminal;
    else if (root.type() == top_token) return top;
    else if (root.type() == tag_token) return tag;
    else {
        for (size_t x = 0; x != e.child_count(); ++x) {
            if (is_lexical(e.get_child(x).root()))
                return quasi;
        }
        return virt;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class Chart>
sausage::sausage(Chart const& chart, typename Chart::edge_equiv_type eq)
{
    fill(chart,eq.representative());
}

////////////////////////////////////////////////////////////////////////////////

template <class Chart>
void sausage::fill(Chart const& chart, typename Chart::edge_type e)
{
    insert(delta(chart,e));
    for (size_t x = 0; x != e.child_count(); ++x) {
        fill(chart,e.get_child(x));
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__SEARCH__SAUSAGE_HPP
