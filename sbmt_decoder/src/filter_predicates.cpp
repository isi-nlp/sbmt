# include <sbmt/search/filter_predicates.hpp>

namespace sbmt {
    
////////////////////////////////////////////////////////////////////////////////

ratio_predicate::ratio_predicate(score_t threshold)
: threshold(threshold)
{
    assert(threshold <= 1.0);
}

////////////////////////////////////////////////////////////////////////////////

ratio_predicate::ratio_predicate(beam_retry const& retry_threshold)
: retry_threshold(retry_threshold)
{
    adjust_for_retry(0);
}

////////////////////////////////////////////////////////////////////////////////

void ratio_predicate::print(std::ostream& o) const 
{
    o << "ratio_predicate{threshold=" << threshold << "}";
}

////////////////////////////////////////////////////////////////////////////////

bool ratio_predicate::adjust_for_retry(unsigned i) 
{                                                         
    return retry_threshold(i,threshold);
}

////////////////////////////////////////////////////////////////////////////////

fuzzy_ratio_predicate::fuzzy_ratio_predicate(score_t t, score_t f)
: threshold(t)
, fuzz(f)
{
    using namespace boost::logic;
    assert(threshold <= 1.0);

}

////////////////////////////////////////////////////////////////////////////////

fuzzy_ratio_predicate::fuzzy_ratio_predicate( beam_retry const& retry_threshold
                                            , beam_retry const& retry_fuzz )
: retry_threshold(retry_threshold)
, retry_fuzz(retry_fuzz)
{
    adjust_for_retry(0);
}

////////////////////////////////////////////////////////////////////////////////

void fuzzy_ratio_predicate::print(std::ostream& o) const 
{
    o << "fuzzy_ratio_predicate{"
      << "threshold=" <<threshold<< "; "
      << "fuzz=" << fuzz << "}";
}

////////////////////////////////////////////////////////////////////////////////

bool fuzzy_ratio_predicate::adjust_for_retry(unsigned i) 
{
    return any_change(retry_threshold(i,threshold), retry_fuzz(i,fuzz));
}

////////////////////////////////////////////////////////////////////////////////

histogram_predicate::histogram_predicate(std::size_t top_n)
: top_n(top_n) {}

////////////////////////////////////////////////////////////////////////////////

histogram_predicate::histogram_predicate(hist_retry const& retry_top_n)
: retry_top_n(retry_top_n)
{
    adjust_for_retry(0);
}

////////////////////////////////////////////////////////////////////////////////

void histogram_predicate::print(std::ostream& o) const 
{
    o << "histogram_predicate{top_n=" << top_n << "}";
}

////////////////////////////////////////////////////////////////////////////////

bool histogram_predicate::adjust_for_retry(unsigned i) 
{
    return retry_top_n(i,top_n);
}

////////////////////////////////////////////////////////////////////////////////

fuzzy_histogram_predicate::fuzzy_histogram_predicate(std::size_t n, score_t f)
: top_n(n)
, fuzz(f) {}

////////////////////////////////////////////////////////////////////////////////

fuzzy_histogram_predicate::fuzzy_histogram_predicate( 
                               hist_retry const& retry_top_n
                             , beam_retry const& retry_fuzz 
                           )
: retry_top_n(retry_top_n)
, retry_fuzz(retry_fuzz)
{
    adjust_for_retry(0);
}

////////////////////////////////////////////////////////////////////////////////

void fuzzy_histogram_predicate::print(std::ostream& o) const 
{
    o << "fuzzy_histogram_predicate{"
      << "top_n=" << top_n << "; "
      << "fuzz=" << fuzz
      << "}";
}

////////////////////////////////////////////////////////////////////////////////

bool fuzzy_histogram_predicate::adjust_for_retry(unsigned i) 
{
    return any_change(retry_top_n(i,top_n),retry_fuzz(i,fuzz));
}

////////////////////////////////////////////////////////////////////////////////

poplimit_histogram_predicate::poplimit_histogram_predicate(
                                  std::size_t top_n
                                , std::size_t poplimit
                                , std::size_t softlimit
                              )
: top_n(top_n)
, poplimit(poplimit)
, softlimit(softlimit)
, num_keep_queries(0)
{}

////////////////////////////////////////////////////////////////////////////////

poplimit_histogram_predicate::poplimit_histogram_predicate( 
                                  hist_retry const& retry_top_n
                                , hist_retry const& retry_poplimit 
                                , hist_retry const& retry_softlimit
                              )
: num_keep_queries(0)
, retry_top_n(retry_top_n)
, retry_poplimit(retry_poplimit)
, retry_softlimit(retry_softlimit)
{
    adjust_for_retry(0);
}

////////////////////////////////////////////////////////////////////////////////

void poplimit_histogram_predicate::print(std::ostream& o) const
{
    o << "poplimit_histogram_predicate{"
      << "top_n=" << top_n << "; "
      << "poplimit=" << poplimit << "; "
      << "softlimit=" << softlimit << "}";
}

////////////////////////////////////////////////////////////////////////////////

bool poplimit_histogram_predicate::adjust_for_retry(unsigned i)
{
    num_keep_queries = 0;
    bool retry = false;
    // note: side-effects needed.  dont rewrite with short-circuiting 
    // operations.
    retry = retry_top_n(i,top_n) || retry;
    retry = retry_poplimit(i,poplimit) || retry;
    retry = retry_softlimit(i,softlimit) || retry;
    return retry;
}

////////////////////////////////////////////////////////////////////////////////

void pass_thru_predicate::print(std::ostream& o) const 
{
    o << "pass-thru-predicate"; 
}

////////////////////////////////////////////////////////////////////////////////

bool pass_thru_predicate::adjust_for_retry(unsigned i) 
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
