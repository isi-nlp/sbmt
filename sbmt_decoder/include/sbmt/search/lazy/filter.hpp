# if ! defined(SBMT__SEARCH__LAZY__FILTER_HPP)
# define       SBMT__SEARCH__LAZY__FILTER_HPP
# include <sbmt/search/logging.hpp>
# include <boost/functional/hash.hpp>
# include <sbmt/forest/implicit_xrs_forest.hpp>

namespace sbmt { namespace lazy {

////////////////////////////////////////////////////////////////////////////////

struct route_hash {
    typedef size_t result_type;
    
    template <class Edge>
    size_t 
    operator()(std::pair< edge_equivalence<Edge>, edge_equivalence<Edge> > const& arg) const
    {
        size_t ret = 0;
        boost::hash_combine(ret,arg.first.representative());
        boost::hash_combine(ret,arg.second.representative());
        return ret;
    }
};

struct route_equal {
    typedef bool result_type;
    
    template <class Edge>
    bool operator()( std::pair< edge_equivalence<Edge>, edge_equivalence<Edge> > const& arg1
                   , std::pair< edge_equivalence<Edge>, edge_equivalence<Edge> > const& arg2
                   ) const
    {
        return arg1.first.representative() == arg2.first.representative() and
               arg1.second.representative() == arg2.second.representative();
    }
};

// precondition: the edges gen is building off are represented in cell
template <class Generator, class Filter, class CellConstruct, class Grammar, class ECS>
CellConstruct& single_ply_unary_filter_cell( Generator& gen
                                           , Filter filter
                                           , CellConstruct& cell
                                           , Grammar const& gram
                                           , ECS const& ecs
                                           , span_t span )
{
    io::logging_stream& log = io::registry_log(filter_domain);
    log << io::pedantic_msg << token_label(gram.dict());
    typedef typename boost::result_of<Generator()>::type edge_type;
    typedef edge_equivalence<edge_type> edge_equiv_type;

    typedef typename Filter::pred_type bool_type;

    edge_equivalence_pool<typename boost::result_of<Generator()>::type> epool;
    bool exhausted = true;
    size_t num = 0;
    while (gen) {
        edge_equiv_type child = (*gen).first_children();
        
        SBMT_PEDANTIC_STREAM(
          filter_domain
        , span<<": unary "<<ecs.hash_string(gram,child.representative())<<" -> "<<ecs.hash_string(gram,*gen)
        );
        typename CellConstruct::iterator pos = cell.find((*gen).root());
        if (pos == cell.end()) {
            bool_type b = filter.insert(epool,*gen);
            if (b == false) {
                exhausted = false;
                break;
            }
        } else { 
            typedef typename CellConstruct::iterator::value_type Equivs;
            typename Equivs::iterator p = pos->find(*gen);
            if (p == pos->end()) {
                bool_type b = filter.insert(epool,*gen);
                if (b == false) { 
                    exhausted = false; 
                    break; 
                }
            } else {
                SBMT_PEDANTIC_STREAM(filter_domain,"unary "<<child.representative().root()<<" -> "<<(*gen).root()<< " seen already.");
            }
        }
        ++num;
        ++gen;
    }
    if (exhausted) SBMT_VERBOSE_STREAM(filter_domain, span<<": unary span applications exhausted after "<<num<<" pops");
    else SBMT_VERBOSE_STREAM(filter_domain, span<<": unary span applications aborted by filter after "<<num<<" pops");

    filter.finalize();
    while (not filter.empty()) {
        edge_equivalence<edge_type> eq = filter.top();
        cell.insert(eq);
        filter.pop();
    }
    return cell;
}

template <class Generator, class Filter, class CellConstruct, class G, class E>
CellConstruct& filter_cell(Generator& gen, Filter filter, CellConstruct& cell, G const& gram, E const& ecs)
{
    //std::cout << "filtercell\n\n";
    typedef typename boost::result_of<Generator()>::type etype;
    typedef typename Filter::pred_type bool_type;
    edge_equivalence_pool<typename boost::result_of<Generator()>::type> epool;
    bool exhausted = true;
    while (gen) {
        etype ed = gen();
        bool_type b = filter.insert(epool,ed);
        if (b == false) {
            exhausted = false;
            break;
        } 
    }
    if (exhausted) SBMT_VERBOSE_STREAM(filter_domain, "span exhausted");
    else SBMT_VERBOSE_STREAM(filter_domain, "span aborted by filter");

    filter.finalize();
    while (not filter.empty()) {
        edge_equivalence<etype> eq = filter.top();
        cell.insert(eq);
        filter.pop();
    }
    return cell;
}

////////////////////////////////////////////////////////////////////////////////


} } // namespace sbmt::lazy

# endif //     SBMT__SEARCH__LAZY__FILTER_HPP
