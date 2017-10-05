/** LWNgramLM.cpp -- implementation of the LWNgramLM.
 * $(Id).
 */
#pragma GCC diagnostic ignore "-Wunused-local-typedefs"
#include <memory>
#include <fstream>
#include <sbmt/logmath.hpp>
#include <sbmt/logmath/lognumber.hpp>
#include <sbmt/ngram/lw_ngram_lm.hpp>
#include <boost/scoped_array.hpp>
#include "Shared/Core/LangModel/LangModelFactory.h"
#include "Shared/Core/LangModel/LangModel.h"
#include "Shared/Core/LangModel/impl/LangModelImplBase.h"
#include "Shared/Core/LangModel/impl/LangModelImpl.h"
#include <cstring>
#include <string>


using namespace LW;
using namespace sbmt::logmath;

namespace sbmt {

LWNgramLM_impl::~LWNgramLM_impl()  {
    clear();
}

void LWNgramLM_impl::clear()
{
    delete m_langModel;
    m_langModel=0;
    m_imp=0;
    last_vocab=vocab=0;
}

unsigned int LWNgramLM_impl::max_order_raw() const
{
    assert(m_langModel);
    return m_langModel->getMaxOrder();
}

// convert string to lw lm id.
unsigned int LWNgramLM_impl:: id_raw(const std::string& tok,bool add_unk)
{
//    assert(m_langModel);
//FIXME: does vocab return the special ids for unk, s, /s tags?  if not, intercept.
    return add_unk ? vocab->insertWord(tok) : vocab->getID(tok);
}

score_t LWNgramLM_impl::
open_prob_len_raw(lm_id_type const* ctx,unsigned len) const
{
    assert(len>0);
#ifdef DEBUG_LM
    DEBUG_LM_OUT << " p(";
    print(DEBUG_LM_OUT,ctx,len-1,len);
    score_t ret=
#else
    return
#endif
    score_t(
        m_imp->
        getContextProb(const_cast<lm_id_type*>(ctx),len)
        ,as_log10());
#ifdef DEBUG_LM
    DEBUG_LM_OUT<<")="<<ret<<' ';
    return ret;
#endif
}

// Input. Returns 0 if no failure.
void LWNgramLM_impl::read(std::string const& filename)
{
    SBMT_INFO_STREAM(lm_domain, "Loading lw LM from file "<<filename);
    LW::XMLConfig config;
    LW::LangModelFactory::initInstance(&config);
    LW::LangModelParams langModelParams;
    langModelParams.m_nMaxOrder=5;//opt.want_max_order;

    m_langModel = LW::LangModelFactory::getInstance()->create(langModelParams);
    if(!m_langModel) goto fail;
    {
        std::ifstream lmf(filename.c_str(), std::ios::binary | std::ios::in);
        if(!lmf) goto fail;
        m_langModel->read(lmf);
    }
    last_vocab=vocab=const_cast<LW::LWVocab *>(m_langModel->getVocab()); //FIXME: make sure it's ok to modify vocab by adding more words, once lm is already loaded.
    m_imp=static_cast<LW::LangModelImpl*>(m_langModel)->getImplementation();
/* cast cannot fail: lm factory does:
    LangModel* pInstance = new LangModelImpl(pImpl, pVocab);
    and class LangModelImpl : public LangModel.
    I'm taking the implementation pointer so I can use an API that doesn't cause unnecessary copying to concatenate the word scored to its history (I already have it thus).
*/
    return;
fail:
    clear();
    throw std::runtime_error("Couldn't load LW LM");
}

static std::string no_vocab="LWNgramLM::MISSING-VOCAB";
static std::string bo_word="#";
static std::string unk_word="<unk>";
static std::string null_word="*";

inline std::string const& vocab_word(LW::LWVocab const* vocab,LWNgramLM_impl::lm_id_type id)
{
    switch (id) {
                // handled in fat_ngram_lm already:
//    case LWNgramLM_impl::bo_id: return bo_word;
//    case LWNgramLM_impl::null_id: return null_word;
    case LWNgramLM_impl::unk_id: return unk_word;

    default:
        return vocab->getWord(id);
    }
}

std::string const& LWNgramLM_impl::word_raw(lm_id_type id) const
{
    return vocab_word(vocab,id);
//    return vocab->getWord(id);
}

LW::LWVocab const* LWNgramLM_impl::last_vocab=NULL;

/*
std::string const& LWNgramLM_impl::word_recent(lm_id_type id)
{
    if (last_vocab)
        return vocab_word(last_vocab,id);
    return no_vocab;
}
*/

unsigned
LWNgramLM_impl::max_supported_order()
{
    return MAX_SUPPORTED_ORDER;
}


LWNgramLM_impl::const_iterator LWNgramLM_impl::longest_prefix_raw(const_iterator i,const_iterator end) const
{
    return m_imp->longest_prefix(i,end);
}

#if 0
LWNgramLM_impl::const_iterator LWNgramLM_impl::longest_suffix_raw(const_iterator i,const_iterator end) const
{
    return m_imp->longest_suffix(i,end);
}
#endif

score_t LWNgramLM_impl::find_bow_raw(const_iterator i,const_iterator end) const
{
#ifdef DEBUG_LM
    DEBUG_LM_OUT << " p_bo(";
    print(DEBUG_LM_OUT,i,end-i);
    score_t ret=
#else
return
#endif
        score_t(m_imp->find_bow(i,end),as_log10());
#ifdef DEBUG_LM
    DEBUG_LM_OUT<<")="<<ret<<' ';
    return ret;
#endif
}

score_t LWNgramLM_impl::find_prob_raw(const_iterator i,const_iterator end) const
{
#ifdef DEBUG_LM
    DEBUG_LM_OUT << " prob(";
    print(DEBUG_LM_OUT,i,end-i);
    score_t ret=
#else
return
#endif
        score_t(m_imp->find_prob(i,end),as_log10());
#ifdef DEBUG_LM
    DEBUG_LM_OUT<<")="<<ret<<' ';
    return ret;
#endif
}

namespace impl {
  struct vocab_gen {
    typedef dynamic_ngram_lm::vocab_generator::result_type result_type;
    operator bool() const {
      return last!=LWVocab::INVALID_WORD;
    }
    result_type operator()() {
      result_type r=last;
      next();
      return r;
    }
    void next() {
      last=i.next();
    }

    VocabIterator i;
    LWVocab::WordID last;
    vocab_gen(LWVocab *v) : i(v) {
      next();
    }
    vocab_gen(vocab_gen const &o) : i(o.i),last(o.last) {}
  };
}

dynamic_ngram_lm::vocab_generator lw_ngram_lm::vocab() const
{
  return dynamic_ngram_lm::vocab_generator(impl::vocab_gen(LWNgramLM_impl::vocab));
}

}

