#ifndef SBMT_SEARCH_PARSE_ROBUST_HPP
#define SBMT_SEARCH_PARSE_ROBUST_HPP

#include <new>
#include <sbmt/span.hpp>
#include <sbmt/search/parse_order.hpp>
#include <graehl/shared/reserved_memory.hpp>
#include <stdexcept>
#include <ostream>
#include <sbmt/search/filter_bank.hpp>

namespace sbmt {

/*
struct parse_failure : public std::runtime_error
{
    parse_failure() : std::runtime_error("parse_failure (gave up)") {}
};
*/

template <class Sentence, class GT>
class chart_from_sentence
{
    Sentence const&s;
    GT & gram;
 public:
    unsigned max_span_rt() const
    {
        return s.length();
    }

    chart_from_sentence(Sentence const& s, GT& gram) : s(s), gram(gram) {}
    template <class CT, class ET>
    void operator()( CT &chart
                   , concrete_edge_factory<ET,GT>& ef
                   , edge_equivalence_pool<ET>& epool ) const
    {
        typename Sentence::iterator itr = s.begin(),
                                    end = s.end();
        chart.reset(max_span_rt());
        unsigned short i = 0;
        for (;itr != end; ++itr,++i) {
            ET e = ef.create_edge(index(*itr,gram.dict()),span_t(i,i+1));
            edge_equivalence<ET> eq = epool.create(e);
            chart.insert_edge(eq);

        }
    }
};

template <class Sentence,class GT>
inline chart_from_sentence<Sentence,GT>
make_chart_from_sentence(Sentence const& s,GT& gram)
{
    return chart_from_sentence<Sentence,GT>(s,gram);
}


template <class ET, class GT, class CT, class ChartInit>
bool parse_robust(
      typename filter_bank<ET,GT,CT>::span_filter_factory_p span_filt_factory
    , typename filter_bank<ET,GT,CT>::unary_filter_factory_p unary_filt_factory
    , cky_generator const& cky_gen
    , boost::function<void (filter_bank<ET,GT,CT>&, span_t, cky_generator const&)> parse
    , GT& gram
    , concrete_edge_factory<ET,GT> &ecs
    , edge_equivalence_pool<ET>& epool
    , CT& chart
    , ChartInit const& chart_init
    , unsigned max_retries=7
    , std::size_t reserve_bytes=5*1024*1024
    )
{
    typedef filter_bank<ET,GT,CT> filter_bank_type;
    using namespace std;

    io::logging_stream& log = io::registry_log(cky_domain);

    graehl::reserved_memory spare(reserve_bytes);
    epool.reset();
    span_filt_factory->adjust_for_retry(0);
	unsigned retry_i=0;
    while (true) {
        chart_init(chart,ecs,epool);
        span_t const& target_span=chart.target_span();
        spare.restore();
        std::string except;
        bool badalloc=false;
        try {
            filter_bank_type filter_bank( span_filt_factory
                                        , unary_filt_factory
                                        , gram
                                        , ecs
                                        , epool
                                        , chart
                                        , target_span );
            log << io::terse_msg << "Filter settings for retry #"<<retry_i<<": ";
            span_filt_factory->print_settings(continue_log(log));
            continue_log(log) << endl;
            parse(filter_bank,target_span,cky_gen);
            return retry_i == 0;
        } catch (std::bad_alloc &e) {           // *only* catches bad_alloc and mempool exception
          spare.use();
          except=e.what();
          badalloc=true;
        } catch (boost::thread_resource_error &e) {
          spare.use();
          except=e.what();
        }
        if (retry_i < max_retries) {
          log << io::warning_msg
              << "\nRETRYING: ran out of memory on retry #" << retry_i
              << " - trying again with tighter beams. "
              << "(CAUGHT exception: " << except << ")" << endl;
          epool.reset();
          ++retry_i;
          continue;
          if (!span_filt_factory->adjust_for_retry(retry_i)) {
            log << io::error_msg
                << "GIVING UP: "
                << "no more span filter factory adjustments are available."
                << endl;
          }
        } else {
          log << io::error_msg
              << "GIVING UP: out of memory still after "
              << retry_i << " retries "
              << "(ERROR: " << except << ")" << endl;
        }
        if (badalloc)
          throw std::bad_alloc();
        else
          throw boost::thread_resource_error();
    }
	return retry_i == 0;
}


}//sbmt

#endif
