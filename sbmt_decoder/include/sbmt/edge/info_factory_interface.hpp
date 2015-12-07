# if ! defined(SBMT__EDGE__INFO_FACTORY_INTERFACE_HPP)
# define       SBMT__EDGE__INFO_FACTORY_INTERFACE_HPP

# include <boost/range.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/grammar/property_construct.hpp>

namespace sbmt {

// these are not valid class.  just pseudocode contract. also,
//FIXME: out of date!

class info_factory_interface {
public:
    typedef struct {}; info_type;

    // info, inside-score, heuristic
    typedef boost::tuple<info_type,score_t,score_t> result_type;

    // whether applying this rule will result in a legitimate info
    // for some types this may only be true for syntax rules.  for others its
    // all rules.  we assume that it can be determined just by looking at the
    // rule, not by resorting to looking at constituents.
    //
    // edge-factory will use this method to avoid making infos that are
    // unscoreable.  you wont have to define is_scoreable().  it will be determined
    // by this method.
    template <class Grammar>
    bool scoreable_rule( Grammar const& grammar
                       , typename Grammar::rule_type rule );


    // a constituent is a lightweight object containing methods
    //   root()  -- root non-terminal
    //   info()  -- matches your factories info -- no need to call cast_info.
    //           -- for composite info types, edge-factory will be responsible
    //              for making sure each component info is processed by its
    //              matching info_factory
    //   span()  -- foreign span
    // though this interface allows for multiple results, in the first iteration
    // of this change i think edge_factory will only ever use the first
    // value...
    // also, right now the interface suggests that if there are tonnes of
    // potential outputs, they cant be lazily pruned by edge-factory.  this
    // may require revisiting.
    template <class Grammar, class ConstituentIterator, ResultOutputIterator>
    void create_info( Grammar const& grammar
                    , typename Grammar::rule_type rule
                    , boost::iterator_range<ConstituentIterator> constituents
                    , ResultOutputIterator results );

    // recompute component inside scores for a single result.  used in
    // nbest reporting.
    template <class Grammar, class ConstituentRange, class ScoreOutputIterator>
    void component_scores( Grammar const& grammar
                         , typename Grammar::rule_type rule
                         , ConstituentRange range
                         , info_type const& result
                         , ScoreOutputIterator out );

  //FIXME: componenet scores are now pairs of fid,score - so this is no longer used
    // names of component scores.  these should be in the same order as the
    // component scores that come out of component_scores()
    std::vector<std::string> component_score_names() const;
};

////////////////////////////////////////////////////////////////////////////////

class info_factory_construct_interface {
    boost::program_options::options_description get_options();

    template <class Dictionary>
    info_factory_interface construct( score_map& features
                                    , property_construct<Dictionary>& prop_constructors
                                    );
};

} // namespace sbmt

# endif //     SBMT__EDGE__INFO_FACTORY_INTERFACE_HPP
