# ifndef DISTORTION_MODEL__DISTORTION_MODEL_HPP
# define DISTORTION_MODEL__DISTORTION_MODEL_HPP
# include "cross_distortion_model.hpp"
# include "disc_distortion_model.hpp"
# include <boost/enum.hpp>

////////////////////////////////////////////////////////////////////////////////

BOOST_ENUM_VALUES(
  distortion_model_type
, const char*
, (generative)("generative")
  (discriminative)("distriminative")
)
;

struct read_cross {
    typedef gusc::varray<bool> result_type;

    template <class Dictionary>
    result_type operator()(Dictionary& dict, std::string const& as_string) const 
    {
        std::vector<bool> r;
        
        typedef boost::tokenizer<boost::char_separator<char> > tok_t;
        tok_t tok(as_string, boost::char_separator<char>(", "));
        for (tok_t::const_iterator it = tok.begin(); it != tok.end(); ++it) {
            if (*it == "10^-") // workaround for ndwf run amok
                continue;
            r.push_back(boost::lexical_cast<bool>(*it));
        }
        return gusc::varray<bool>(r.begin(),r.end());
    }
};

template < class cross_type = gusc::varray<bool> >
class distortion_constructor {
public:
    distortion_constructor()
    : binarized(false)
    , model_type(distortion_model_type::generative)
    {}
    void init(sbmt::in_memory_dictionary& dict) {}
    sbmt::options_map get_options() 
    {
        using namespace sbmt;
        options_map opts("distortion model options");
        opts.add_option("binarized-distortion-feature",
                        optvar(binarized),
                        "distortion rule featue has been binarized across brf rules.  "
                        "only for generative model");
        opts.add_option("distortion-prob-file",
                        optvar(distortion_prob_file),
                        "file containing distortion probabilities.  "
                        "only for generative model");
        opts.add_option("distortion-model-type",
                        optvar(model_type),
                        "legal values are discriminative or generative");
        return opts;
    }
    // options can be set through options_map
    bool set_option(std::string key, std::string value) { return false; }

    template <class Grammar>
    sbmt::any_type_info_factory<Grammar> construct( Grammar& grammar
                                                  , sbmt::lattice_tree const& lattice
                                                  , sbmt::property_map_type pmap ) const
  {
      if (model_type == distortion_model_type::generative) {
          return generative_model(grammar, pmap);
      } else {
          return discriminative_model(grammar, pmap);
      }
  }

private:
    template <class Grammar>
    distortion::cross::distortion_info_factory<cross_type>
    generative_model(Grammar& grammar, sbmt::property_map_type pmap) const
    {
        using namespace distortion::cross;
        using namespace sbmt;
        using namespace boost;
        using namespace std;
        
        int max_len = 0;
        boost::uint32_t distortion_id = grammar.feature_names().get_index("distortion");
        double distortion_weight = grammar.get_weights()[distortion_id];

        ifstream f(distortion_prob_file.c_str());
        string line;
        distortion_table m;
        int line_count = 0;
        while (getline(f,line)) {
            bool cross;
            int len;
            string label;
            double prob;
            istringstream ls(line);
            ls >> len >> label >> cross >> prob;
            max_len = std::max(max_len,len);
            indexed_token nt = grammar.dict().create_token(label,tag_token);
            m[distortion_key(len,nt,cross)] = score_t(prob);
            SBMT_DEBUG_MSG( distortion::cross::gen_distortion_log
                          , "distortion-table entry: (%s,%s,%s) -> %s"
                          , len % print(nt,grammar.dict()) % cross % prob
                          );
            ++line_count;
        }

        SBMT_INFO_MSG( distortion::cross::gen_distortion_log
                , "%s lines in distortion-model-file %s"
                , line_count % distortion_prob_file );

        SBMT_INFO_MSG( distortion::cross::gen_distortion_log
               , "distortion model using %s cross features"
               , (binarized ? "brf" : "xrs") );

        SBMT_INFO_MSG( distortion::cross::gen_distortion_log
               , "distortion weight=%s"
               , distortion_weight );

        indexed_token null_token = grammar.dict().create_token("None",tag_token);
        if (m.find(distortion_key(0,null_token,0)) == m.end() or
            m.find(distortion_key(0,null_token,1)) == m.end()) {
            SBMT_ERROR_MSG( distortion::cross::gen_distortion_log
                          , "%s"
                          , "unseen values must be present in distortion prob file"
                          );
        }

        return distortion_info_factory<cross_type>( 
                 distortion_weight // feature weight
               , distortion_id
               , pmap["cross"]  // name of feature on rules
               , m
               , null_token
               , binarized
               , max_len 
               );
    }

    template <class Grammar>
    distortion::disc::distortion_info_factory<cross_type>
    discriminative_model(Grammar& grammar, sbmt::property_map_type pmap) const
    {
        using namespace distortion::disc;
        gusc::sparse_vector<distortion_key,double> weights;
        BOOST_FOREACH(range_value<weight_vector const>::type v, grammar.get_weights())
        {
            string fname = grammar.feature_names().get_token(v.first);
            auto_ptr<distortion_key>
                pk = distortion_key::from_feature_name(fname);
            if (pk.get()) weights[*pk] = v.second;
        }

        return distortion_info_factory<cross_type>(weights, pmap["cross"]);
    }

    std::string distortion_prob_file;
    bool binarized;
    distortion_model_type model_type;
};

# endif // DISTORTION_MODEL__DISTORTION_MODEL_HPP
