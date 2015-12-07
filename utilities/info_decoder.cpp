# include "mini_decoder_filters.hpp"
# include "output_args.hpp"
# include <sbmt/edge/composite_info.hpp>
# include <boost/program_options.hpp>
# include <string>
# include <iostream>
# include <sbmt/grammar/brf_file_reader.hpp>
# include <sbmt/search/parse_robust.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/sentence.hpp>
# include <sbmt/chart/chart.hpp>
# include <sbmt/chart/ordered_cell.hpp>
# include <sbmt/chart/ordered_span.hpp>

filter_args decode_filters;
sbmt::output_args oargs;

namespace po = boost::program_options;

struct options {
    std::vector<std::string> info_names;
    std::string grammar;
    std::string sentence;
    std::string weights;
};

options parse_options(int argc, char** argv)
{
    options opts;
    po::options_description desc;
    desc.add_options()
        ( "help,h"
        , "display options"
        )
        ( "use-info"
        , po::value(&opts.info_names)->composing()
        , "list which info types to use in decoding"
        )
        ( "grammar"
        , po::value(&opts.grammar)
        , "a grammar file in brf format"
        )
        ( "sentence"
        , po::value(&opts.sentence)
        , "a sentence to decode"
        )
        ( "weights"
        , po::value(&opts.weights)
        , "a weight string to score with"
        )
        ;
    desc.add(sbmt::info_registry_options());
    desc.add(decode_filters.options());
    po::variables_map vm;
    store(parse_command_line(argc,argv,desc), vm);
    notify(vm);
    
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }
    
    if (opts.info_names.size() == 0) opts.info_names.push_back("null");
    
    return opts;
}

sbmt::grammar_in_mem* 
create_grammar( std::string file
              , sbmt::property_constructors_type const& pc
              , sbmt::score_combiner const& sc )
{
    std::ifstream str(file.c_str());
    sbmt::brf_stream_reader reader(str);
    std::auto_ptr<sbmt::grammar_in_mem> grammar(new sbmt::grammar_in_mem());
    grammar->load(reader,sc,pc);
    return grammar.release();
}

int main(int argc, char** argv)
{
    using namespace sbmt;
    typedef sbmt::composite_info                              info_type;
    typedef edge<info_type> ET;
    typedef grammar_in_mem gram_type;
    typedef ET edge_type;
    typedef sbmt::edge_equivalence<ET>                        edge_equiv_type;
    typedef basic_chart<edge_type,ordered_chart_edges_policy> chart_type;
    typedef span_filter_factory<ET,gram_type,chart_type>      spff_type;
    typedef unary_filter_factory<ET,gram_type>                uff_type;
    typedef early_exit_from_span_filter_factory<ET,gram_type,chart_type> uff_impl_type;
    typedef filter_bank<edge_type,gram_type,chart_type>       filter_bank_type;
    typedef edge_equivalence_pool<edge_type>                  epool_type;
    typedef concrete_edge_factory<edge_type,gram_type>        ecs_type;
    
    options opts = parse_options(argc,argv);
    
    sbmt::property_constructors_type
        pc = sbmt::info_registry_create_property_constructors(opts.info_names);
    
    sbmt::score_combiner sc(opts.weights);
    
    boost::shared_ptr<sbmt::grammar_in_mem>
        grammar(create_grammar(opts.grammar,pc,sc));
        
    sbmt::fat_sentence fs = sbmt::foreign_sentence(opts.sentence);
    sbmt::chart_from_sentence<sbmt::fat_sentence,sbmt::grammar_in_mem> ci(fs, *grammar);
    sbmt::span_t tgt(0,fs.size());
        
    sbmt::property_map_type pmap = pc.property_map();
    
    sbmt::composite_info_factory 
        info_fctry = sbmt::info_registry_create_composite_info_factory(opts.info_names,sc,*grammar,pmap);
    
    sbmt::edge_factory<sbmt::composite_info_factory> edge_fctry(info_fctry);
    sbmt::concrete_edge_factory<edge_type,gram_type> ecs(edge_fctry);
    
    boost::shared_ptr<spff_type> psff=decode_filters.create_filter<ET,gram_type,chart_type>(tgt,ecs,*grammar);
    boost::shared_ptr<uff_type> uff(new uff_impl_type(psff,tgt));
    chart_type chart(0);
    epool_type epool;
    full_cky_generator gen;
    parse_cky p;
    boost::function<void (filter_bank<edge_type,gram_type,chart_type>&, span_t, cky_generator const&)>
        parser(p);
    sbmt::parse_robust<edge_type,gram_type,chart_type>( psff
                      , uff
                      , gen
                      , parser
                      , *grammar
                      , ecs
                      , epool
                      , chart
                      , ci
                      );
    
    oargs.output_results( chart
                        , *grammar
                        , ecs
                        , epool
                        , 1
                        );
    return 0;
}