#ifndef   SBMT__SEARCH__THREADED_PARSE_ORDER_HPP
#define   SBMT__SEARCH__THREADED_PARSE_ORDER_HPP

#include <sbmt/search/parse_order.hpp>
#include <sbmt/hash/thread_pool.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/bind.hpp>
#include <sbmt/search/cky_logging.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

class combine_span_thread_exception : public std::logic_error
{
public:
    combine_span_thread_exception(std::string msg = "")
    : std::logic_error(msg) {}
    
    virtual ~combine_span_thread_exception() throw() {}
};

inline void throw_combine_span_working(span_t target)
{
    std::stringstream sstr;
    sstr << "span " << target << " is already working.";
    combine_span_thread_exception e(sstr.str());
    throw e;
}

////////////////////////////////////////////////////////////////////////////////
///
///  uses a thread to combine all subspans into a given span.
///
////////////////////////////////////////////////////////////////////////////////
class combine_span
{
public:
    typedef void result_type;

    template <class FBT>
    void operator()(FBT& filter_bank
                   , span_t target
                   , cky_generator const& cky_gen ) const
    {
        std::stringstream sstr;
        graehl::time_change spent;
        
        io::logging_stream& log = io::registry_log(cky_domain);

        partition_iterator pitr, pend;
        boost::tie(pitr,pend) = cky_gen.partitions(target);
        for (;pitr != pend; ++pitr) {
            graehl::time_space_change apply_spent;
            filter_bank.apply_rules(pitr->first, pitr->second.right());
            log << io::verbose_msg << target << ": applied "
                << pitr->first <<" , "<< pitr->second <<" in "<<spent<<"." << io::endmsg;
        }

        log << io::info_msg << target << ": rules-applied in " 
            << spent << "." << io::endmsg;
        filter_bank.finalize(target);
        log << io::info_msg << target << ": finalized in " 
            << spent << io::endmsg;
    }
};

template <class FBT> 
void threaded_parse_cky_impl( thread_pool& pool
                            , FBT& filter_bank
                            , span_t target_span
                            , cky_generator const& cky_gen
                            )
{
    using namespace std;
    using namespace boost;
    graehl::time_space_change total_spent;
    edge_stats s=filter_bank.ecs_stats();
    io::logging_stream& log = io::registry_log(cky_domain);

    unsigned chart_size=target_span.size();
    double total_work=cky_progress(chart_size);
    
    for ( span_index_t i=target_span.left(),e=target_span.right()
        ; i < e
        ; ++i ) {
        span_t spn(i,i+1);
        filter_bank.finalize(spn);
    }

    for ( span_index_t len=2,max=length(target_span)
        ; len<=max
        ; ++len ) {
        span_iterator sitr, send;
        boost::tie(sitr,send) = cky_gen.shifts(target_span,len);
        graehl::time_space_change len_spent;
        edge_stats s_span=filter_bank.ecs_stats();
        for (; sitr != send; ++sitr) {
            pool.add( bind( combine_span()
                          , ref(filter_bank)
                          , *sitr
                          , cref(cky_gen)
                          )
                    );
        }
        
        pool.wait();       
        if (logging_at_level(log,io::lvl_terse)) {
            log << io::terse_msg << "threads finished for lengths " << len
                << ": " << (filter_bank.ecs_stats()-s_span) 
                << " in " << len_spent << io::endmsg;
                
            double work_done_post=cky_progress(chart_size,len);
            
            log << io::terse_msg << setw(2) << 100*work_done_post/total_work 
                << "% done after span length " << len<<"/"<<chart_size 
                << io::endmsg;
        }
    }

    filter_bank.finalize(target_span);    
    log << io::terse_msg << "In total, " << (filter_bank.ecs_stats()-s)
        <<" " << total_spent << endl;
}

class threaded_parse_cky {
public:
threaded_parse_cky(std::size_t num_threads):num_threads(num_threads){}
template <class FBT> 
void operator()( FBT& filter_bank
               , span_t target_span
               , cky_generator const& cky_gen = full_cky_generator() ) const
{
    thread_pool pool(num_threads);

    threaded_parse_cky_impl(pool, filter_bank, target_span, cky_gen);
    
    pool.join();  
}
private:
  std::size_t num_threads;
};

class shared_thread_pool_parse_cky {
public:
    shared_thread_pool_parse_cky(thread_pool* pool) : pool(pool) {}
    template <class FBT> 
    void operator()( FBT& filter_bank
                   , span_t target_span
                   , cky_generator const& cky_gen = full_cky_generator() ) const
    {
        threaded_parse_cky_impl(*pool, filter_bank, target_span, cky_gen);
    }
private:
    thread_pool* pool;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT__SEARCH__THREADED_PARSE_ORDER_HPP
