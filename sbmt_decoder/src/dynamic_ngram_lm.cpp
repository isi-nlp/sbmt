#include <sstream>

#include <boost/filesystem/operations.hpp>
#include <sbmt/io/log_auto_report.hpp>
#include <graehl/shared/quote.hpp>
#include <graehl/shared/must_eof.hpp>

#include <sbmt/ngram/lw_ngram_lm.hpp>
#include <sbmt/ngram/multi_ngram_lm.hpp>
#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <sbmt/ngram/big_ngram_lm.hpp>
#include <sbmt/ngram/cluster_lm.hpp>
#include <sbmt/ngram/neural_lm.hpp>
#include <sbmt/ngram/null_ngram_lm.hpp>

namespace sbmt {

ngram_lm_factory ngram_lm_factory::s_instance;

namespace impl {
struct lm_filename_special {
    inline bool operator()(char c) const
    {
        switch(c) {
        case '[':case ']':
            return true;
        default:
            return false;
        }
    }
};
}

//defined in base_ngram_lm.hpp
std::string parse_filename(std::istream &i,boost::filesystem::path const& relative_base)
{
    char c;
    if (i >> c && c=='[') {
        std::string ret=graehl::in_quote(i,impl::lm_filename_special());
        if (i >> c && c==']') {
            using namespace boost::filesystem;
            return boost::filesystem::absolute(path(ret),relative_base).native();
        }

    }
    throw std::runtime_error("expected [filename] (yes, in brackets) for lm."
                             "  use [\"quotes[for]brackets\\\" and quotes\"");
}

void write_filename(std::ostream &o,std::string const& filename)
{
    o << '[';
    graehl::out_quote(o,filename,impl::lm_filename_special());
    o << ']';
}

void dynamic_ngram_lm::
set_weights_raw(weight_vector const& w, feature_dictionary& dict)
{}


void dynamic_ngram_lm::describe(std::ostream &o) const {
    Fat::describe(o);
}

void dynamic_ngram_lm::print_further_sequence_details
(std::ostream &o
 ,const_iterator history_start
 ,const_iterator score_start
 ,const_iterator end
 , bool factors
    ) const
{
}


//FIXME: test
dynamic_ngram_lm::const_iterator
dynamic_ngram_lm::longest_prefix_raw(const_iterator i,const_iterator end) const
{
    return Fat::longest_prefix_raw(i,end);
}

//FIXME: test
dynamic_ngram_lm::const_iterator
dynamic_ngram_lm::longest_suffix_raw(const_iterator i,const_iterator end) const
{
    return Fat::longest_suffix_raw(i,end);
}

dynamic_ngram_lm::~dynamic_ngram_lm() {}

bool dynamic_ngram_lm::loaded() const
{
    return true;
}


ngram_ptr ngram_lm_factory::create(std::istream &i,boost::filesystem::path const& relative_base,unsigned cache_size)
{
    char c;
    std::stringstream s;
    std::string name;
    i >> std::ws;
    while (i.get(c)) {
        if (c=='=') {
            name=s.str();
            s.str(""); //s.seekp(0) should be redundant
        } else if (c=='[') {
            i.unget();
            break;
        } else {
            s.put(c);
        }
    }
    ngram_options opt;
    i >> opt;
    std::string const&lm_type=s.str();

    Builder b=maker[lm_type];
    if (!b) throw std::runtime_error("Unknown LM type: " + lm_type);
    SBMT_INFO_STREAM(lm_domain, "Loading dynamic LM of type "<<lm_type<<" with options "<<opt);
    SBMT_LOG_TIME_SPACE(lm_domain, info, boost::str(boost::format("Loaded LM %1%%2% ")% lm_type %opt));

    ngram_ptr ret=b->build(opt,i,relative_base,cache_size);
    if (!name.empty()) ret->name=name;
    ret->set_ids();
    ret->log_describe();

    return ret;
//    throw std::runtime_error("Couldn't parse dynamic ngram LM spec.");
}

ngram_ptr ngram_lm_factory::create(std::string const& spec,boost::filesystem::path const& relative_base,unsigned cache_size)
{
    SBMT_TERSE_STREAM(lm_domain, "Loading dynamic LM from spec: "<<spec);
    std::istringstream in(spec);
    ngram_ptr ret=create(in,relative_base,cache_size);
    graehl::must_eof(in);
    return ret;
}

SBMT_REGISTER_NGRAM_LM(lw_ngram_lm,lw);
SBMT_REGISTER_NGRAM_LM(big_ngram_lm,big);
SBMT_REGISTER_NGRAM_LM(neural_lm,nplm);
SBMT_REGISTER_NGRAM_LM(cluster_lm,cluster);
SBMT_REGISTER_NGRAM_LM(multi_ngram_lm,multi);
SBMT_REGISTER_NGRAM_LM(null_ngram_lm,null);

}
