# if ! defined(SBMT__EDGE__GREEDY_NGRAM_INFO_HPP)
# define       SBMT__EDGE__GREEDY_NGRAM_INFO_HPP

# include <sbmt/edge/ngram_info.hpp>
# include <gusc/generator/transform_generator.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <unsigned int N, class LMID = unsigned>
struct greedy_ngram_info : ngram_info<N,LMID> {
    typedef ngram_info<N,LMID> ngram_info_;
    bool equal_to(greedy_ngram_info const& other) const
    {
        assert(context == other.context);
        assert(context <= ngram_info_::ctx_len);
        return ngram_info_::equal_to(other,context);
    }

    std::size_t hash_value() const
    {
        return ngram_info_::hash_value(context);
    }
    greedy_ngram_info() : context(1) {}
    void set_context(unsigned short c)
    {
        assert(c <= ngram_info_::ctx_len);
        context = c;
    }
    greedy_ngram_info(ngram_info<N,LMID> const& info, unsigned short context)
      : ngram_info<N,LMID>(info)
      , context(context) {}
private:
    unsigned short context;
};

////////////////////////////////////////////////////////////////////////////////

struct set_context {
    template <class X> struct result {};
    
    template <unsigned int N, class LMID>
    struct result<set_context(boost::tuple<ngram_info<N,LMID>,score_t,score_t>)>
    {
        typedef boost::tuple<greedy_ngram_info<N,LMID>,score_t,score_t> type;
    };
    
    template <unsigned int N, class LMID>
    boost::tuple<greedy_ngram_info<N,LMID>,score_t,score_t>
    operator()(boost::tuple<ngram_info<N,LMID>,score_t,score_t> const& r) const
    {
        return boost::make_tuple( 
                   greedy_ngram_info<N,LMID>(boost::get<0>(r),context)
                 , boost::get<1>(r)
                 , boost::get<2>(r)
               );
    }
    
    explicit set_context(unsigned short context) : context(context) {}
private:
    unsigned short context;
};

////////////////////////////////////////////////////////////////////////////////

template <unsigned int N, class LM>
class greedy_ngram_info_factory {
private:
    ngram_info_factory<N,LM> info_factory;
    unsigned short context;
public:
    typedef greedy_ngram_info<N,typename LM::lm_id_type> info_type;
    typedef gusc::transform_generator<
                typename ngram_info_factory<N,LM>::result_generator 
              , set_context
            > result_generator;
    
    greedy_ngram_info_factory( unsigned short context
                             , boost::shared_ptr<LM> lm
                             , bool shorten
                             , size_t lmid )
      : info_factory(lm,shorten,lmid)
      , context(context) {}
      
    template <class Grammar, class ConstituentIterator>
    result_generator
    create_info( Grammar const& grammar
               , typename Grammar::rule_type rule
               , span_t const& span
               , boost::iterator_range<ConstituentIterator> const& constituents
               )
    {
        return gusc::generate_transform(
                   info_factory.create_info(grammar,rule,constituents)
                 , set_context(context)
               );
    }
    
    template <class Grammar, class ConstituentIterator, class ScoreOutputIterator>
    ScoreOutputIterator
    component_scores( Grammar const& grammar
                    , typename Grammar::rule_type rule
                    , span_t const& span
                    , boost::iterator_range<ConstituentIterator> constituents
                    , info_type const& result
                    , ScoreOutputIterator out ) 
    {
        return info_factory.component_scores(grammar,rule,constituents,result,out);
    }
    
    std::vector<std::string> component_score_names() const
    {
        return info_factory.component_score_names();
    }
    
    template <class Gram>
    bool scoreable_rule(Gram& gram, typename Gram::rule_type rule) const
    {
        return info_factory.scoreable_rule(gram,rule);
    }

    template <class Gram>
    score_t rule_heuristic(Gram& gram, typename Gram::rule_type rule) const
    {
        return info_factory.rule_heuristic(gram,rule);
    }
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__EDGE__GREEDY_NGRAM_INFO_HPP
