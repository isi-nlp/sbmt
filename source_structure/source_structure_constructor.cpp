#include "source_structure_info.hpp"
#include <sbmt/grammar/rule_feature_constructors.hpp>
using namespace sbmt;
using namespace source_structure;

class source_structure_constructor {
public:

    source_structure_constructor() : src_nt_list_file(""), trg_nt_list_file("") {}

    options_map get_options() {
        options_map opts("source structure model options");
        opts.add_option("src-nt-list",
                    optvar(src_nt_list_file),
                    "the nonterminal list of the source language "
                    "for which we are compute the match/cross features");
        opts.add_option("trg-nt-list",
                    optvar(trg_nt_list_file),
                    "the nonterminal list of the target language "
                    "for which we are compute the match/cross features");
        opts.add_option("source-constituents",
                    optvar(source_constituents),
                    "the source constituent list. ");
        return opts;
    }
    void init(sbmt::in_memory_dictionary& dict) {}
    bool set_option(std::string key, std::string value) { 
        if(key == "source-constituents"){
            source_constituents=value;
            return true;
        } else {
            return false; 
        }
    }

    template<class Grammar>
     any_info_factory construct(
                            Grammar& grammar,
                            const lattice_tree& latree,
                          property_map_type const& pmap )
    {
        return source_structure_info_factory(grammar, pmap, src_nt_list_file, trg_nt_list_file, source_constituents);
    }


private:
   string src_nt_list_file;
   string trg_nt_list_file;
   string source_constituents;
};


struct source_structure_init{
    source_structure_init()
    {
        register_info_factory_constructor("source-structure", source_structure_constructor());
        sbmt::register_rule_property_constructor("source-structure","span",sbmt::span_constructor());
    }
};

static source_structure_init d;
