#include <sbmt/search/sausage.hpp>
#include <sbmt/search/unary_filter_interface.hpp>
#include <sbmt/chart/ordered_cell.hpp>
#include <sbmt/multipass/limit_cells_filter.hpp>
#include <sbmt/multipass/cell_heuristic_compute.hpp>
#include <sbmt/dependency_lm/DLM.hpp>

////////////////////////////////////////////////////////////////////////////////
///
///  reveal_parse_orders and reveal_parse_inits are just dummy classes to force
///  instantiation of all the
///  hide_parse_order and hide_chart_init instantiations we need, since explicit
///  instantiation of a class is expected to instantiate all non-template member
///  functions.
///
////////////////////////////////////////////////////////////////////////////////
//\{
template <class ET>
struct reveal_parse_orders {
    typename parse_traits<ET>::parse_order_f
    reveal(threaded_parse_cky const& po)
    { return hide_parse_order<ET>(po); }

    typename parse_traits<ET>::parse_order_f
    reveal(parse_cky const& po) { return hide_parse_order<ET>(po); }

    typedef block_parse_cky<threaded_parse_cky> block_parse_mt;
    typedef block_parse_cky<parse_cky> block_parse_st;

    typename parse_traits<ET>::parse_order_f
    reveal(block_parse_mt const& po) { return hide_parse_order<ET>(po); }

    typename parse_traits<ET>::parse_order_f
    reveal(block_parse_st const& po) { return hide_parse_order<ET>(po); }
};

// macro alternative, because maybe i am wrong about what in a class gets
// instantiated when explicit instantiation occurs...
# define MINI_REVEAL_PARSE_ORDER(ET,PO) \
template parse_traits< ET >::parse_order_f \
hide_parse_order< ET , PO >(PO const& po)

# define MINI_REVEAL_PARSE_ORDERS(ET) \
MINI_REVEAL_PARSE_ORDER(ET,parse_cky); \
MINI_REVEAL_PARSE_ORDER(ET,threaded_parse_cky); \
MINI_REVEAL_PARSE_ORDER(ET,block_parse_cky<parse_cky>); \
MINI_REVEAL_PARSE_ORDER(ET,block_parse_cky<threaded_parse_cky>)



////////////////////////////////////////////////////////////////////////////////

template <class ET>
struct reveal_chart_inits {
    typedef chart_from_sentence<fat_sentence,gram_type> cfs_init;
    typedef block_lattice_chart<gram_type> blc_init;

    typename parse_traits<ET>::chart_init_f
    reveal(blc_init const& ci) { return hide_chart_init<ET>(ci); }

    typename parse_traits<ET>::chart_init_f
    reveal(cfs_init const& ci) { return hide_chart_init<ET>(ci); }
};

# define MINI_REVEAL_CHART_INIT(ET,CI) \
template parse_traits< ET >::chart_init_f \
hide_chart_init< ET , CI >( CI const& ci)

# define MINI_CHART_FROM_SENTENCE(S,G) chart_from_sentence< S , G >

# define MINI_REVEAL_CHART_INITS(ET) \
MINI_REVEAL_CHART_INIT(ET,MINI_CHART_FROM_SENTENCE(fat_sentence,gram_type)); \
MINI_REVEAL_CHART_INIT(ET,block_lattice_chart<gram_type>)


////////////////////////////////////////////////////////////////////////////////

template <unsigned int N, class LM>
struct instantiate_ngram_order
 : reveal_chart_inits<typename ngram_edge<N>::type>
 , reveal_parse_orders<typename ngram_edge<N>::type> {
    typedef typename ngram_edge<N>::type edge_t;
    void reveal( mini_decoder& md
               , typename parse_traits<edge_t>::chart_init_f ci
               , typename parse_traits<edge_t>::parse_order_f po
               , std::size_t sid
               , span_t ts
               , edge_stats& stats
               , std::string& trans
               , boost::shared_ptr<LM> lm
                 , unsigned pass
        )
    {
        md.decode_ngram_order<N,LM>(ci,po,sid,ts,stats,trans,lm,pass);
    }
};


# define MINI_INSTANTIATE_NGRAM_DLM_ORDER(N,M,LM) \
template  void \
mini_decoder:: \
decode_ngram_order< N,M, LM >( parse_traits<ngram_dlm_edge< N,M >::type>::chart_init_f ci \
  , parse_traits<ngram_dlm_edge< N,M >::type>::parse_order_f po \
  , std::size_t sentid \
  , span_t target_span \
  , edge_stats& stats \
  , std::string& translation \
  , boost::shared_ptr<LM> lm \
  , unsigned pass \
  , boost::shared_ptr<MultiDLM> dlm \
  )

# define MINI_INSTANTIATE_DLM_ORDER(M) \
template void \
mini_decoder:: \
decode_dlm_order<M>( parse_traits<sbmt::edge<edge_info<head_history_dlm_info<M-1> > > >::chart_init_f ci \
                , parse_traits<sbmt::edge<edge_info<head_history_dlm_info<M-1> > > >::parse_order_f po \
                , std::size_t sentid \
                , span_t target_span \
                , edge_stats& stats \
                , std::string& translation \
                , unsigned pass \
                , boost::shared_ptr<MultiDLM> dlm)

# define MINI_INSTANTIATE_NGRAM_ORDER(N,LM) \
template void \
mini_decoder::decode_ngram_order< N , LM >( \
    parse_traits<ngram_edge< N >::type>::chart_init_f ci \
  , parse_traits<ngram_edge< N >::type>::parse_order_f po \
  , std::size_t sid \
  , span_t ts \
  , edge_stats& stats \
  , std::string& trans \
  , boost::shared_ptr< LM > lm \
  , unsigned pass \
  )


////////////////////////////////////////////////////////////////////////////////

template <class ET, class ParseOrder>
typename parse_traits<ET>::parse_order_f
hide_parse_order(ParseOrder const& po)
{
    return typename parse_traits<ET>::parse_order_f(po);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class ChartInit>
typename parse_traits<ET>::chart_init_f
hide_chart_init(ChartInit const& ci)
{
    return typename parse_traits<ET>::chart_init_f(ci);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
void mini_decoder::decode_dispatch(
                      concrete_edge_factory<ET,gram_type> &ef
                    , typename parse_traits<ET>::chart_init_f ci
                    , typename parse_traits<ET>::parse_order_f po
                    , std::size_t sentid
                    , span_t target_span
                    , edge_stats &stats
                    , std::string &translation
                    , unsigned pass
    )
{
    translation=decode(ci,po,ef,sentid,target_span,pass);
    stats=ef.stats();
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned int O,class LM> void
mini_decoder::
decode_ngram_order( typename parse_traits<typename ngram_edge<O>::type>::chart_init_f ci
                  , typename parse_traits<typename ngram_edge<O>::type>::parse_order_f po
                  , std::size_t sentid
                  , span_t target_span
                  , edge_stats& stats
                  , std::string& translation
                  , boost::shared_ptr<LM> lm
                  , unsigned pass
    )
{
    typedef edge_info_factory< ngram_info_factory<O,LM> > IF;
    typedef typename IF::info_type Info;
    typedef sbmt::edge<Info> Edge;
//    na.set_grammar(grammar()); // if you are paranoid, you can perform the mapping here.
    boost::shared_ptr<IF> inf(new IF(boost::in_place(lm,na.ngram_shorten,ga.pc.property_map("")["lm_string"])));
    lm_weight()=lm->set_weights(grammar().get_weights(),grammar().feature_names(),0);
    //edge_factory<IF> efact(inf,weight_tm_heuristic,weight_info_heuristic);
    concrete_edge_factory<Edge,gram_type> ef(boost::in_place< edge_factory<IF> >(inf,weight_tm_heuristic,weight_info_heuristic));
    decode_dispatch(ef,ci,po,sentid,target_span,stats,translation,pass);
    //log_info_stats(*inf,sentid,pass);
}
template <unsigned int O, unsigned int M, class LM> void
mini_decoder::
decode_ngram_order( typename parse_traits<typename ngram_dlm_edge<O,M>::type>::chart_init_f ci
                  , typename parse_traits<typename ngram_dlm_edge<O,M>::type>::parse_order_f po
                  , std::size_t sentid
                  , span_t target_span
                  , edge_stats& stats
                  , std::string& translation
                  , boost::shared_ptr<LM> lm
                  , unsigned pass
                  //, std::vector<boost::shared_ptr<LWNgramLM> > dlm
                  , boost::shared_ptr<MultiDLM> dlm
    )
{
    typedef head_history_dlm_info_factory<M-1,LWNgramLM> DLMF;
    typedef edge_info_factory<DLMF> EDLMF;
    typedef ngram_info_factory<O,LM> LMF;
    typedef edge_info_factory<LMF> ELMF;
    typedef joined_info_factory< ELMF, EDLMF > IF;
    typedef typename IF::info_type Info;
    typedef sbmt::edge<Info> Edge;
//    na.set_grammar(grammar()); // if you are paranoid, you can perform the mapping here.
    dlm->set_weights(grammar().get_weights(),grammar().feature_names());
    boost::shared_ptr<ELMF>
        ngram_if(new ELMF(boost::in_place(lm,na.ngram_shorten,ga.pc.property_map("")["lm_string"])));
    boost::shared_ptr<EDLMF>
        dlm_if(new EDLMF(boost::in_place(dlm, ga.pc.property_map(""), grammar().dict())));
    boost::shared_ptr<IF>
        ngram_dlm_if(new IF(ngram_if, dlm_if));

    // dont care here.
    lm_weight()=lm->set_weights(grammar().get_weights(),grammar().feature_names(),0);
    // dont care the lm_weight here.
    //edge_factory<IF> efact(ngram_dlm_if,weight_tm_heuristic,weight_info_heuristic);
    concrete_edge_factory<Edge,gram_type> ef(boost::in_place< edge_factory<IF> >(ngram_dlm_if,weight_tm_heuristic,weight_info_heuristic));
    decode_dispatch(ef,ci,po,sentid,target_span,stats,translation,pass);
    //log_info_stats(*ngram_dlm_if,sentid,pass);
    //decode_dispatch(ef,ci,po,sentid,target_span,stats,translation,pass);
    //ngram_dlm_if->print_stats(io::registry_log(decoder_app) << io::info_msg);
}

template <unsigned int M>
void
mini_decoder::
decode_dlm_order( typename parse_traits<sbmt::edge< edge_info<head_history_dlm_info<M-1> > > >::chart_init_f ci
                , typename parse_traits<sbmt::edge< edge_info<head_history_dlm_info<M-1> > > >::parse_order_f po
                , std::size_t sentid
                , span_t target_span
                , edge_stats& stats
                , std::string& translation
                , unsigned pass
                , boost::shared_ptr<MultiDLM> dlm
    )
{
    typedef edge_info_factory< head_history_dlm_info_factory<M-1,LWNgramLM> > IF;
    typedef typename IF::info_type Info;
    typedef sbmt::edge<Info> Edge;
    dlm->set_weights(grammar().get_weights(),grammar().feature_names());
    boost::shared_ptr<IF>
            dlm_if(new IF(boost::in_place(dlm, ga.pc.property_map(""), grammar().dict())));
    concrete_edge_factory<Edge,gram_type> ef(boost::in_place< edge_factory<IF> >(dlm_if,weight_tm_heuristic,weight_info_heuristic));
    decode_dispatch(ef,ci,po,sentid,target_span,stats,translation,pass);
}
/*
template <unsigned int M> void
mini_decoder::
decode_dlm_order( typename parse_traits<edge<head_history_dlm_info<M-1> > >::chart_init_f ci
                , typename parse_traits<edge<head_history_dlm_info<M-1> > >::parse_order_f po
                , std::size_t sentid
                , span_t target_span
                , edge_stats& stats
                , std::string& translation
                , unsigned pass
                , MultiDLM& dlm
    )
{
    typedef head_history_dlm_info_factory<M-1,LWNgramLM> IF;
    typedef typename IF::info_type Info;
    typedef edge<Info> Edge;
//    na.set_grammar(grammar()); // if you are paranoid, you can perform the mapping here.
    dlm.set_combiner(ga.combine);
    boost::shared_ptr<head_history_dlm_info_factory<M-1, LWNgramLM> > dlm_if(new head_history_dlm_info_factory<M-1, LWNgramLM>(dlm, grammar().dict()));
    edge_factory<IF> efact(1.0,1.0,dlm_if,weight_tm_heuristic,weight_info_heuristic);
    concrete_edge_factory<Edge,gram_type> ef(efact);
    decode_dispatch(ef,ci,po,sentid,target_span,stats,translation,pass);
    //log_info_stats(*ngram_dlm_if,sentid,pass);
    //decode_dispatch(ef,ci,po,sentid,target_span,stats,translation,pass);
    dlm_if->print_stats(io::registry_log(decoder_app) << io::info_msg);
}
*/

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
std::string
mini_decoder::decode( typename parse_traits<ET>::chart_init_f ci
                    , typename parse_traits<ET>::parse_order_f po
                    , concrete_edge_factory<ET,GT>& ef
                    , std::size_t sentid
                    , span_t target_span
                    , unsigned pass
    )
{
    typedef ET edge_type;
    using namespace std;

    //ostream &nbest_out=this->nbest_out();
    sbmt::io::logging_stream& log = sbmt::io::registry_log(decoder_app);

    status() << "decode-start"
             << " id=" << sentid
        << " pass=" << pass
             << endl;

    typedef edge_equivalence<ET>                              edge_equiv_type;
    typedef basic_chart<edge_type,ordered_chart_edges_policy> chart_type;
    typedef span_filter_factory<ET,gram_type,chart_type>      spff_type;
    typedef unary_filter_factory<ET,gram_type>                uff_type;
    typedef early_exit_from_span_filter_factory<ET,gram_type,chart_type> uff_impl_type;
    typedef filter_bank<edge_type,gram_type,chart_type>       filter_bank_type;
    typedef edge_equivalence_pool<edge_type>                  epool_type;
    typedef concrete_edge_factory<edge_type,gram_type>        ecs_type;

    indexed_token_factory &tf=grammar().dict();

    chart_type chart(0);
    epool_type epool(max_items);
    try {

        boost::shared_ptr<cky_generator> cky_gen = fa.create_cky_generator();

        edge_equiv_type::max_edges_per_equivalence(fa.max_equivalents);

        sbmt::io::log_time_space_report report(log,sbmt::io::lvl_info,"Decoded: ");

        boost::shared_ptr<spff_type> psff=fa.create_filter<chart_type>(target_span,ef,grammar());
        boost::shared_ptr<uff_type> uff(new uff_impl_type(psff,target_span));
        ef.set_cells();
        if (use_kept_cells) {
            cells.check_target_span(target_span);
            psff.reset(new sbmt::limit_cells_factory<ET,gram_type,chart_type>
                       (cells,psff,target_span,grammar()));
            uff.reset(new sbmt::limit_cells_unary_factory<ET,GT>
                      (cells,uff,target_span,grammar()));
            if (weight_cell_outside)
                ef.set_cells(cells,weight_cell_outside,1e-20);
        }


        parse_robust ( psff
                     , uff
                     , *cky_gen
                     , po
                     , grammar()
                     , ef
                     , epool
                     , chart
                     , ci
                     , fa.maximum_retries()
                     , oargs.reserve_nbest
                     );

        edge_equiv_type top=chart.top_equiv(grammar());
        span_t top_span=chart.target_span();
        assert(top_span == target_span);

        log << sbmt::io::pedantic_msg << "\n"
            << print(chart,grammar().dict()) << sbmt::io::endmsg;
        //DONE PARSING - now nbest/output:

        report.report();

        if (sbmt::io::logging_at_level(log,sbmt::io::lvl_verbose)) {
            sausage ssg(chart,top);

            log << sbmt::io::verbose_msg
                << "sausage[" << sentid << "]={{{" << ssg << "}}}"
                << sbmt::io::endmsg;
        }

        log << sbmt::io::info_msg << "Number of equivalences in memory: " << edge_equiv_type::equivalence_count() << sbmt::io::endmsg;
        outside_score outside;
        ef.set_cells(); // don't refer to cell restriction/heuristic at all during nbest output

        cell_heuristic *pcells=0;
        if (compare_kept_cells && cells.has_target_span(target_span))
            pcells=&cells;

        string retval = oargs.output_results( chart.top_equiv(grammar())
                                            , grammar()
                                            , ef
                                            , sentid
                                            , outside
                                            , pass
                                            , pcells
                                            ) ;

        if (keep_cells || print_cells_file) {
            cell_heuristic_compute(cells).compute(top,tf,top_span.right(),outside,keep_cells_beam);
             if (allow_new_glue) {
                 score_t unlikely(10000,as_neglog10()); // supposed glue was used in first pass.  then we don't want to unfairly encourage other glue cells!
                 cells.allow_glue(tf.tag(fa.limit_syntax_glue_label),unlikely);
            }
            if (print_cells_file) {
                oargs.validate_output(print_cells_file,"print-cells-file");
                cells.print(*print_cells_file,tf);
                *print_cells_file << std::endl;
            }
        }

        status() << "decode-finish"
                 << " id="<< sentid
                 << " pass=" << pass
                 << " status=success" << endl;

        return retval ;

    } catch (exception& e) {
        ++parses_failed;

        status() << "decode-finish"
                 << " id=" << sentid
                 << " pass=" << pass
                 << " status=fail" << endl;
        log << sbmt::io::debug_msg
            << "\n" << print(chart,grammar().dict()) << sbmt::io::endmsg;

        return oargs.dummy_output_results<ET>(sentid, grammar(), ef, e.what(),pass);
    } catch (...) {
        ++parses_failed;

        log << sbmt::io::debug_msg
            << "\n" << print(chart,grammar().dict()) << sbmt::io::endmsg;
        status() << "decode-finish"
                 << " id="<< sentid
                 << " pass=" << pass
                 << " status=fail" << endl;
        return oargs.dummy_output_results<ET>(sentid, grammar(), ef, "unknown error",pass);
    }
}

