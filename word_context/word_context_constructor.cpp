#include "word_context_info.hpp"
#include <sbmt/grammar/rule_feature_constructors.hpp>
#include "wsd_features.hpp"
#include <sbmt/grammar/alignment.hpp>
using namespace sbmt;
using namespace word_context;

class word_context_constructor {
public:

    word_context_constructor() : wsd_src_list(""), wsd_trg_list("") {}

    options_map get_options() {
        options_map opts("wsd model options");
        opts.add_option("wsd-src-list",
                    optvar(wsd_src_list),
                    "the src-word list that the wsd model concerns. "
                    "other src words will be marked by <unk>.");
        opts.add_option("wsd-trg-list",
                    optvar(wsd_trg_list),
                    "the trg-word list that the wsd model concerns. "
                    "other trg words will be marked by <unk>");
        opts.add_option("source-constituents2",
                    optvar(source_constituents),
                    "(optional) the source side consituents. Setting "
                    "this option will generate syntactic WSD features. ");
        return opts;
    }

    void init(sbmt::in_memory_dictionary& dict) {}

    bool set_option(std::string key, std::string value)
    {
        if(key == "source-constituents2"){
            source_constituents=value;
            return true;
        } else {
            return false;
        }
    }

    template<class Grammar>
     any_info_factory construct(Grammar& grammar,
                            const lattice_tree& latree,
                          property_map_type const& pmap )
    {
        m_wsd.load(wsd_src_list, wsd_trg_list);
        return word_context_info_factory(grammar, latree, pmap, m_wsd, source_constituents);
    }


private:
   string wsd_src_list;
   string wsd_trg_list;
   wsd_features m_wsd;
   // the list of NT[a,b] constituents.
   string source_constituents;
};

struct read_align{
  typedef alignment result_type;

  template <class Dictionary>
  result_type operator()(Dictionary& dict, std::string const& as_string) const {
      result_type a;
      istringstream ist(as_string);
      a.read(ist);
      return a;
  }
};

struct word_context_init{
    word_context_init()
    {
        register_info_factory_constructor("wsd", word_context_constructor());
        register_rule_property_constructor("wsd","span",span_constructor());
        register_rule_property_constructor("wsd","align",read_align());
    }
};

static word_context_init wcd;
