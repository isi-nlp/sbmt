# if ! defined(XRSDB__LATTICE_READER_HPP)
# define       XRSDB__LATTICE_READER_HPP

# include <boost/lexical_cast.hpp>
# include <boost/function.hpp>
# include <boost/graph/adjacency_list.hpp>
# include <boost/graph/properties.hpp>
# include <sbmt/token.hpp>
# include <sbmt/span.hpp>
# include <iosfwd>
# include <gusc/generator/lazy_sequence.hpp>
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/token/wildcard.hpp>


namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

struct graph_info {
    std::set<sbmt::span_t> brackets;
    size_t id;
};

typedef boost::adjacency_list<
          boost::vecS
        , boost::vecS
        , boost::bidirectionalS
        , size_t
        , sbmt::indexed_token
        , graph_info
        > graph_t;

////////////////////////////////////////////////////////////////////////////////

typedef boost::function<void (graph_t const&)> lattice_callback;

////////////////////////////////////////////////////////////////////////////////

void 
lattice_reader(std::istream&, lattice_callback, sbmt::indexed_token_factory&);

////////////////////////////////////////////////////////////////////////////////
//
//  create the transitive closure graph, with "x" as the label for each new edge,
//  where x is the longest distance between vertex pairs for the edge
//
////////////////////////////////////////////////////////////////////////////////
graph_t skip_lattice(graph_t const& g, sbmt::indexed_token_factory& dict);


////////////////////////////////////////////////////////////////////////////////
graph_t to_dag(lattice_tree const& ltree, indexed_token_factory& dict);
graph_t to_dag(gusc::lattice_ast const& lat, indexed_token_factory& dict);

typedef std::map<indexed_token, std::set<int> > lex_outside_distances;
typedef std::map<
          int
        , boost::tuple<lex_outside_distances,lex_outside_distances>
        > left_right_distance_map;

left_right_distance_map
make_left_right_distance_map( graph_t const& skip_lattice
                            , indexed_token_factory& tf );

} // namespace xrsdb

# endif //     XRSDB__LATTICE_READER_HPP
