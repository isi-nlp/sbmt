#ifndef SBMT_SEARCH_PARSE_ORDER_HPP
#define SBMT_SEARCH_PARSE_ORDER_HPP

#include <sbmt/span.hpp>
#include <ostream>
#include <iomanip>
#include <graehl/shared/time_space_report.hpp>
#include <sbmt/search/cky_logging.hpp>

namespace sbmt {

//TODO: make virtual function object based on ET,GT,CT

struct parse_cky {
    template <class FB>
    void summarize_cells_for_span(FB &filter_bank,span_t const& span) const
    {
        SBMT_VERBOSE_EXPR(cky_domain,
        {
          if (filter_bank.get_chart().empty_span(span))
              continue_log(str)<<"Nothing kept for span "<<span;
          else {
              filter_bank.get_chart().contents(span).print(continue_log(str));
              continue_log(str)<<" kept for span "<<span<<": ";
              filter_bank.get_chart().print_cells_for_span(
                  continue_log(str),span,filter_bank.get_grammar().dict());
          }
//                              continue_log(str) << sbmt::io::endmsg;
        });
    }
    
        
template <class filter_bank_type>
void operator()( filter_bank_type &filter_bank
               , span_t target_span
               , cky_generator const& cky_gen = full_cky_generator() ) const
{
    using namespace std;
    edge_stats s=filter_bank.ecs_stats();
    graehl::time_space_change total_spent;
    unsigned chart_size=target_span.size();
    double total_work=cky_progress(chart_size);
    
    io::logging_stream& log = io::registry_log(cky_domain);
    log << io::terse_msg << "Starting parse (" << chart_size << " words) with " << s << io::endmsg;
    
    for (span_index_t len=1,max=length(target_span);
         len<=max; ++len) {
        edge_stats s_spanlen=filter_bank.ecs_stats();
        if ( logging_at_level(log,io::lvl_terse) ) {
            double work_done=cky_progress(chart_size,len,0);
            log << io::terse_msg << setw(2) << 100*work_done/total_work 
                << "% done.  Starting span length "<< len << "/" 
                << chart_size << io::endmsg;
        }
    
        graehl::time_space_change len_spent;

        //todo: use shift_generate for all len; just make sure partition generator does nothing
        if (len==1) {
            for (span_index_t i=target_span.left(),e=target_span.right()
                     ;i<e;++i) {
                span_t span(i,i+1);
                log << io::info_msg << span <<": ";
                graehl::time_space_change spent;
                filter_bank.finalize(span);
                log << io::info_msg <<"  finalized. "<<(filter_bank.ecs_stats()-s_spanlen)
                    << " in " << spent << io::endmsg;
                summarize_cells_for_span(filter_bank,span);
            }
        } else {            
            span_iterator sitr, send;
            boost::tie(sitr,send) = cky_gen.shifts(target_span,len);
            for (; sitr != send; ++sitr) {
                span_t const&span=*sitr;
                partition_iterator pitr, pend;
                boost::tie(pitr,pend) = cky_gen.partitions(span);
                edge_stats s_span=filter_bank.ecs_stats();
                graehl::time_space_change spent;
                for (;pitr != pend; ++pitr) { 
                    graehl::time_space_change apply_spent;
                    filter_bank.apply_rules(pitr->first, pitr->second.right());
                    log << io::verbose_msg << span << ": applied "
                        << pitr->first <<" , "<< pitr->second <<" in "<<spent<<"." << io::endmsg;
                }
                log << io::info_msg << span << " applied rules in "<< spent <<"." << io::endmsg;
                filter_bank.finalize(span);
                log << io::info_msg << span << "  finalized. "<<(filter_bank.ecs_stats()-s_span)
                    << " in " << spent << io::endmsg;
                summarize_cells_for_span(filter_bank,span);                
            }
            double work_done_post=cky_progress(chart_size,len);
            log << io::terse_msg << setw(2) 
                << 100*work_done_post/total_work << "% done after span length "
                << len << "/" <<chart_size << "; " << (filter_bank.ecs_stats()-s_spanlen) << " in " << len_spent << io::endmsg;
        }
        if(false) { //FIXME: why is this disabled?  what do we do on the chart after?  print pruning proximity?
        //if (chart_size - len < len) {
            span_t s(chart_size - len, len);
            log << io::info_msg << "destroy sub-chart " << s << endl;
            partitions_generator pg(s);
            partitions_generator::iterator pitr = pg.begin(), 
                                           pend = pg.end();
            for (;pitr != pend; ++pitr) {
                filter_bank.get_chart().clear(pitr->first);
                filter_bank.get_chart().clear(pitr->second);
            }
            if (s != target_span) filter_bank.get_chart().clear(s);
        }
    }
    filter_bank.finalize(target_span);
    log << io::terse_msg << "In total, " << (filter_bank.ecs_stats()-s);
    continue_log(log) << " " << total_spent << io::endmsg;
}
};

}//sbmt

#endif
