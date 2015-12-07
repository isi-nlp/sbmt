# if ! defined(XRSDB__LATTICE_HPP)
# define       XRSDB__LATTICE_HPP

# include <boost/tuple/tuple.hpp>

namespace xrsdb {

template <class Lattice>
struct lattice_traits {
    typedef typename boost::tuple::element<1,Lattice>::type graph_type;
    typedef typename boost::graph_traits<graph_type>
};
/// for now i dont want to commit to writing an actual lattic class, so a 
/// lattice will just be a tuple (graph, start, finish), with the possibly
/// unenforced attribute that all topological sorts of graph satisfy
/// start <= nodes(graph) <= finish
template <class Lattice>
boost::tuple::element<1,Lattice>::type const& 
start(Lattice const& lat) { return lat.template get<1>(); }

template <class Lattice>
boost::tuple::element<1,Lattice>::type const& 
finish(Lattice const& lat) { return lat.template get<1>(); }

template <class Lattice>
boost::tuple::element<1,Lattice>::type const& 
finish(Lattice const& lat) { return lat.template get<1>(); }

} // namespace xrsdb

# endif //     XRSDB__LATTICE_HPP
