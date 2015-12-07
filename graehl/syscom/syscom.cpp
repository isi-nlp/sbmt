#define GRAEHL__SINGLE_MAIN

#include <boost/multi_array.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <graehl/shared/input_error.hpp>
#include <graehl/shared/cmdline_main.hpp>
#include <graehl/shared/io.hpp>
#include <graehl/shared/dynarray.h>
#include <sbmt/token/in_memory_token_storage.hpp>
#include <sbmt/hash/oa_hashtable.hpp>
#include <omp.h>
#include <cmath>

char const *usage_str="system combination\n"
    ;

namespace graehl {
namespace sc {
using namespace std;
using namespace boost;
using namespace sbmt;

USE_PRINT_SEQUENCE(std::vector<istream_arg>)

typedef double TT_prob;
typedef double Score;
typedef double Scale;

typedef unsigned Rank;
typedef in_memory_token_storage dict_t;
typedef dict_t::index_type index;
typedef dict_t::token_type token;
typedef unsigned Sysid;

typedef dynamic_array<index> Sent;


typedef pair<TT_prob,TT_prob> Norm_inv; // normal p(t|s) and inverse p(s|t) probs
typedef pair<index,index> S_t; // src,trg word pair
typedef oa_hash_map<S_t,Norm_inv> TTable;

dict_t sym; //contains both src and trg lang words (and NULL). FIXME: global/unsynchronized

void append_words(Sent &s,string const& line)
{
//FIXME: could make this faster by scanning line chars for ws vs. copying to stringstream
    istringstream i(line);
    token t;
    while (i >> t)
        s.push_back(sym.get_index(t));
}

TTable tt;

inline TT_prob s2t(index s,index t)
{
    return tt[S_t(s,t)].first;
}
inline TT_prob s2tinv(index s,index t)
{
    return tt[S_t(s,t)].second;
}

index null=sym.get_index("NULL");

Score p_null=.2; // prob generating NULL english (backbone) word to explain hyp words

Scale alpha_sem=.3; // surf = 1-sem

template <class S>
inline unsigned common_prefix(S const& a,S const& b)
{
    unsigned m=0;
    for (typename S::const_iterator ia=a.begin(),ea=a.end(),ib=b.begin(),eb=b.end();
         ia!=ea && ib!=eb;
         ++ib,++eb,++m)
        if (*ia!=*ib) break;
    return m;
}

template <class S>
inline double prefix_similarity(S const& a,S const& b)
{
    double s=std::max(a.size(),b.size());
    return common_prefix(a,b)/s;
}

Scale rho_surface_scale=10; // infty -> exact match only

template <class S>
inline double surface_similarity(S const& a,S const& b,Scale rho)
{
    return std::exp(rho*(prefix_similarity(a,b)-1));
}

typedef tuple<Sysid,Rank,Score,Sent> Nbest;

struct Sentence_xltns
{
    Sent foreign,backbone;
    typedef dynamic_array<Nbest> Nbests;
    Nbests nbests;
    typedef fixed_array<TT_prob> P;
    typedef multi_array<TT_prob,2> Ps;
    void compute_all()
    {
//#pragma omp parallel for shared(nbests)
        unsigned N=nbests.size();
        for (unsigned n=0;n<N;++n) {
            Sent const& h=nbests[n].get<3>();
            unsigned J=h.size();
            Ps h_bb(extents[J][I]);
            similarity(h,h_bb,rho_surface_scale);
        }
    }

    void similarity(Sent const& h,Ps &h_bb,Scale rho,Scale alpha_sem=.3)
    {
        semantic(h,h_bb);
        surface(h,h_bb,rho,alpha_sem)
    }

    // call after semantic - interpolates surface against it
    void surface(Sent const& h,Ps &h_bb,Scale rho,Scale alpha_sem=1.)
    {
        unsigned J=h.size();
        for (unsigned j=0;j<J;++j) {
            token const& eh=sym[h[j]];
            for (unsigned i=0;i<I;++i) {
                token const& e=sym[backbone[i]];
                Score surf=surface_similarity(eh,e,rho);
                TT_prob &psem=h_bb[j][i];
                psem*=alpha_sem;
                psem+=(1-alpha_sem)*surf;
                DBP3(eh,e,surf);
            }
        }

    }

    void semantic(Sent const& h,Ps &h_bb)
    {
        // prob of hyp e'[j] given backbone e[i]
        unsigned I=backbone.size(),K=foreign.size(),J=h.size();
        for (unsigned j=0;j<J;++j) {
            TT_prob h_from_null=p_null*s2t(null,h[j]);
            for (unsigned i=0;i<I;++i) {
                TT_prob &psem=h_bb[j][i];
                psem=h_from_null;
                for (unsigned k=0;k<K;++k) {
                    index f=foreign[k];
                    psem+=s2tinv(f,backbone[i])*s2t(f,h[i]);
                }
            }
        }
        /*
          P pfs(K);
          for (unsigned k=0;k<K;++k)
          for (unsigned i=0;i<I;++i)
          pfs[k] += tt[S_t(foreign[k],backbone[i])].second; // inv
        */
    }

};

struct Corpus
{

    typedef printable_options_description<std::ostream> OD;
    OD opts;
    Corpus() : opts("Corpus options"),have_sent(false)
    {
        add_options();
    }
    istream_arg foreign,backbone;
    vector<istream_arg> sys_nbests;
    vector<unsigned> sys_nbests_lines;
//    vector<string> sys_nextlines;


    istream_arg nbests;

    void add_options() {
        opts.add_options()
            ("foreign,f",defaulted_value(&foreign),"foreign sents")
            ("backbone,b",defaulted_value(&backbone),"backbone translation")
            ("nbests,n",defaulted_value(&sys_nbests),"SENT n HYP n cost hyp... (multiple input files allowed)")
            ;
    }

    dynamic_array<Sentence_xltns> parsed;
    bool have_last;
    unsigned last_sent;
    void read()
    {
        //        null=sym.get_index("NULL");
        parsed.push_back();
        Sentence_xltns &xltns=parsed.back();
        std::string l;
        unsigned sentno=0;
        while (getline(*foreign,l)) {
            ++sentno;
            append_words(xltns.foreign,l);
            if (!getline(*backbone,l))
                fail();
            append_words(xltns.backbone,l);
            unsigned sent;
            for (unsigned i=0,e=sys_nbests.size();i!=e;++i) {
                std::istream &n=*sys_nbests[i];
                unsigned &lineno=sys_nbests_lines[i];
                ++lineno;
                while (n) {
#define ASSERT_NBEST(read,desc) if (!(read)) {throw_input_error(n,desc,sys_nbests[i].desc(),lineno);}
                    if (!have_last) {
                        ASSERT_NBEST(expect_consuming(n,"SENT"),"missing SENT");
                        ASSERT_NBEST(n>>sent,"SENT number bad");
                    }
                    ASSERT_NBEST(expect_consuming(n,"HYP"),"missing HYP");
                    unsigned hyp;
                    ASSERT_NBEST((n>>hyp && hyp>0),"HYP rank should be >= 1");
                    xltns.nbests.push_back();
                    Nbest &nb=xltns.nbests.back();
                    nb.get<0>()=i;
                    nb.get<1>()=hyp;
                    Score s;
                    ASSERT_NBEST(n>>s,"expected nbest cost e.g. -90.5");
                    nb.get<2>()=s;
                    append_words(nb.get<3>(),n);
                    if (have_last) {
                        ASSERT_NBEST(expect_consuming(n,"SENT"),"missing SENT");
                        ASSERT_NBEST(n>>sent,"SENT number bad");
                        if (sent!=last_sent)
                            break;
                    }
                }
            }
            last_sent=sent;
            have_last=true;
        }

};


struct syscom : public main
{
    syscom() : main("syscom",usage_str,"v1")
    {

    }
    void set_defaults()
    {
        set_defaults_base();

    }
    Corpus corpus;

    void add_options_extra(OD &all) {
        all.add(corpus.opts);
    }
};

}

}

INT_MAIN(graehl::sc::syscom)


