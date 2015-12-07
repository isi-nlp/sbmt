// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include <Common/stldefs.h>
#include "LangModelSA.h"

#define DEBUG_LANGMODEL_SA

#include <assert.h>

#include "Common/ErrorCode.h"
#include "LMVersion.h"
#ifndef LM_NO_COMMON_LIB
#include "Common/Crypto/CryptoStreamFactory.h"
#endif
#include "Common/Serializer/Serializer.h"

using namespace std;

namespace LW {



        template <class NodeEntryType>
        void
        LangModelSANode<NodeEntryType>::init(unsigned int nNodeCount)
        {
                // Make sure we have something in the node
                if (0 == nNodeCount)
                        throw Exception(ERR_IO, "Cannot initialize SA language model with 0 entries");

////
//              // MAX_ENTRIES_PER_NODE must be a power of 2 !!!
//              m_nMask = MAX_ENTRIES_PER_NODE - 1;
//
//              m_nMask = 1;
//              while (0 != nCounter) {
//                      m_nMask |= (m_nMask << 1);
//                      m_nMaskBitCount++;
//                      nCounter >>= 1;
//              }

//              m_nOrder = nOrder;
                m_nNodeCount = nNodeCount;

                // Determine whether the node is terminal by the size of the node entry
                /*
                if (sizeof(NodeEntryType) == sizeof(highest_entry)) {
                        m_bIsTerminal = true;
                }
                else {
                        m_bIsTerminal = false;
                }
                */

                // Allocate in increments of max MAX_ENTRIES_PER_NODE
                // Calculate the number of subnodes (add 1 for the extra node at the end) 
                // i.e. nNodeCount + 1 - 1
                m_nSubnodeCount = (nNodeCount) / MAX_ENTRIES_PER_NODE + 1;
                // Allocate the subnodes
                m_pNodes = new NodeEntryType*[m_nSubnodeCount];

                // Add 1 for the extra node at the end
                unsigned int nRemainingCount = nNodeCount + 1;
                for (unsigned int i = 0; i < m_nSubnodeCount; i++) {
                        unsigned int nAllocCount;
                        if (nRemainingCount >= MAX_ENTRIES_PER_NODE) {
                                nAllocCount = MAX_ENTRIES_PER_NODE;
                        }
                        else {
                                nAllocCount = nRemainingCount;
                        }
                        
                        // Allocate subnode
                        m_pNodes[i] = (NodeEntryType*) malloc((nAllocCount + 1) * sizeof(NodeEntryType));

                        nRemainingCount -= nAllocCount;
                }

                // Fill in the last (fake) entry
                getEntry(m_nNodeCount)->init_fake();


//              // Allocate one entry more than necessary. This is a "fake" entry so we don't have to test
//              // the end of the buffer when finding how many children a node has
////            m_pNodes = (lower_entry*) pMemAlloc->alloc((nNodeCount + 1) * sizeof(lower_entry));
//              m_pNodes = (lower_entry*) malloc((nNodeCount + 1) * sizeof(NodeEntryType));
//              if (NULL == m_pNodes) {
//                      throw Exception(ERR_IO, "Failed to allocate memory in the language model.");
//              }
//
//              // Fill in the last (fake) entry
//              lower_entry* pLast = (lower_entry*) (m_pNodes + nNodeCount);
//              pLast->word = 0;
//              pLast->prob = 0;
//              if (!isTerminal()) {
//                      pLast->bow = 0;
//                      pLast->children = 0xFFFFFFFF;
//              }
        }

        template <class NodeEntryType>
        LangModelSANode<NodeEntryType>::~LangModelSANode() 
        {
                if (m_pNodes) {
                        for (unsigned int i = 0; i < m_nSubnodeCount; i++) {
                                free(m_pNodes[i]);
                        }
                        delete[] m_pNodes ;
                }
        }

        template <class NodeEntryType>
        void LangModelSANode<NodeEntryType>::loadCompleted()
        {

        }


template <class NodeEntryType>
bool
LangModelSANode<NodeEntryType>::find(word_type v, index_t start,index_t end,NodeEntryType const*&node_found,index_t &i_found) const
{
    // precondition: node is sorted
    
    //FIXME: can that really happen?  we set the last parent's succesor to a sentinel.  init_fake when uninit, and actual value when read.
//            if (end > m_nNodeCount) end=m_nNodeCount;
    //assert(end>start);
    NodeEntryType const* p;
    while (start<end) { // invariant: j<start:key[j]<v.  j>=end:key[j]>=v
        index_t mid=(start+end)/2;
        p=getEntry(mid);
        if (p->word < v) // j<mid+1:key[j]<v
            start=mid+1;
        else // j>=mid:key[j]>=v
#ifndef BSEARCH_2WAY
        if (v < p->word)
#endif 
            end=mid;
#ifndef BSEARCH_2WAY
        else {
            i_found=mid;
            node_found=p;
            return true;
        }
#endif        
    }
#ifdef BSEARCH_2WAY
    if (start==end || (p=getEntry(start))->word!=v)
        return false;
    i_found=start;
    node_found=p;
    return true;
#else
    return false;
#endif 
}




/*
inline int compare(LWVocab::WordID a,LWVocab::WordID b)
{
    return (a>b)?1:
        ((a<b)?-1:0);
}

        int compareNodes(const void* p1, const void* p2)
        {
            return compare(
                ((highest_entry*) p1)->word,
                ((highest_entry*) p2)->word);
        }
*/
        //template <class NodeEntryType>
        //void LangModelSANode<NodeEntryType>::sort(int nStartIndex, int nEndIndex)
        //{
        //      //assert(nStartIndex < static_cast<int>(m_nNodeCount));
        //      //assert(nEndIndex < static_cast<int>(m_nNodeCount));

        //      // Perform a quicksort
        //      qsort(m_pNodes + nStartIndex, nEndIndex - nStartIndex, sizeof(NodeEntryType), compareNodes);
        //}

        //template <class NodeEntryType>
        //NodeEntryType* LangModelSANode<NodeEntryType>::getEntries() 
        //{
        //      return m_pNodes;
        //}

        //class LangModelSANodeIterator 
        //{
        //public:
        //      LangModelSANodeIterator(const vector<LangModelSANode*>& nodes, unsigned int nOrder) {
        //              m_nodes = nodes;
        //              m_nOrder = nOrder;
        //      }
        //      bool reset() {
        //              for (unsigned int i = 0; i < m_nOrder; i++) {
        //                      currentIndex[i] = 0;
        //              }
        //              // Return true only if we have some n-grams for the maximum order
        //              return m_nodes[nOrder - 1]->getCount() > 0;
        //      }
        //      bool next() {
        //              unsigned int nIndex = nOrder - 1;
        //              if (nIndex >= 1) && currentIndex[nIndex-1] < 
        //      }
        //private:
        //      unsigned int m_nOrder;
        //      unsigned int currentIndex[MAX_SUPPORTED_ORDER];
        //};

LangModelSA::LangModelSA(LWVocab* pVocab, const LangModelParams& params) : m_params(params)
        {
//              m_pMemAlloc = new MemAlloc();
                m_pVocab = pVocab;
        }

        LangModelSA::~LangModelSA()
        {
                // Do not delete m_pVocab, this class does not own it!
        }

        //void LangModelSA::sort()
        //{
        //      for (int i = 0; i < m_nodes.size(); i++) {
        //              LangModelSANode* pNode = m_nodes[i];
        //      }
        //}


void LangModelSA::read(std::istream& inStream, LangModel::ReadOptions nOptions)
{

// The values below are for reading FORMAT_BINARY_SA
//      [C2C1] [version]
        //  [C2C4] [max order] - this is only for version > CURRENT_VERSION2
//      [C2C2] [Vocabulary Count]
//      [Vocab ID] [Vocab String]
//      ...
//      [C2C3] [N-Gram Order] [N-Gram Count]
//  [WORD ID] [Prob] [BOW] [FirstChildOffset]
//      ...
//      [C2C3] [0] // 0 means end

        // We use default max order of 3
        // If version > CURRENT_VERSION2, this number is read from the header
        // If not WE ASSUME IT IS 3 for now. In the next release all language models
        // will have the max order in the header
        unsigned want_max_order = m_params.m_nMaxOrder;
        unsigned nMaxOrder=0;
        

        // The auto_pointer will take care of automatically destructing the stream in case somebody throws
#ifndef LM_NO_COMMON_LIB
        auto_ptr<istream> pCryptoStream(CryptoStreamFactory::getInstance()->createIStream(inStream, true));
        ISerializer in(pCryptoStream.get());
#else
        istream* pCryptoStream = &inStream;
        ISerializer in(pCryptoStream);
#endif


        // Read magic number
        unsigned int nMagic;
        in.read(nMagic);
        if (nMagic != VERSION_MAGIC_NUMBER2) {
                throw InvalidLangModelVersionException(ERR_IO, "Invalid Language Model file or version not supported.");
        }

        // Read version
        unsigned int nVersion;
        in.read(nVersion);
        if (nVersion != CURRENT_VERSION2 && nVersion != CURRENT_VERSION2_1) {
                throw InvalidLangModelVersionException(ERR_IO, "Unsupported version number in Language Model file.");
        }

        // For CURRENT_VERSION2_1 we have the max order in the header
        // For previous version, we assume the default (requested) for now
        if (nVersion == CURRENT_VERSION2_1) {
            in.read(nMaxOrder);
#ifdef DEBUG_LANGMODEL_SA
//                std::cerr << "\nLM header says order="<<nMaxOrder<<" and we want (max) order="<<want_max_order<<std::endl;
#endif 
                if (nMaxOrder > want_max_order)
                    nMaxOrder=want_max_order;
        }

        // Read vocabulary header
        in.read(nMagic);
        if (nMagic != VOCAB_MAGIC_NUMBER2) {
                throw LW::Exception(ERR_IO, "Invalid Language Model file: Bad Vocabulary magic number.");
        }

        // Read Vocabulary count
        unsigned int nVocabCount = 0;
        in.read(nVocabCount);

        LWVocab::WordID nWordID;
        string sWord;
        for (unsigned int i = 0; i < nVocabCount; i++) {
                in.read(nWordID);
                in.read(sWord);

                m_pVocab->insertWord(sWord, nWordID);
        }

        // Test if we wanted to extract the vocabulary only. In that case stop here
        if (LangModel::READ_VOCAB_ONLY != nOptions) {
            unsigned prev_order=0;
                for(;;) {
                        unsigned int nGramOrder;

                        // Read magic number for probabilities
        //              cerr << "Reading magic number for probs..." << endl;
                        in.read(nMagic);
                        if (nMagic != PROB_MAGIC_NUMBER2) {
                                char szError[500];
                                sprintf_s(szError, 500, 
                    "Invalid Language Model file: Bad magic number in probability header. Expected %d. Found %d", 
                    (unsigned int) PROB_MAGIC_NUMBER2, nMagic);
                                throw LW::Exception(ERR_IO, szError);
                        }

        //              cerr << "Reading n-gram order for probs..." << endl;
                        in.read(nGramOrder);
        //              cerr << "Order: " << nGramOrder << endl;
                        if (nGramOrder > MAX_SUPPORTED_ORDER) {
                                char buffer[500];
                                sprintf_s(buffer, 500, 
                    "n-gram order (%d) found in the Language Model file is higher than maximum supported order (%d)", 
                    nGramOrder, (unsigned int) MAX_SUPPORTED_ORDER);
                                throw LW::Exception(ERR_IO, buffer);
                        }
                        
                        if (0 == nGramOrder) { // end of ngrams marker (doesn't exist in some file revisions?)
                            if (nMaxOrder && prev_order !=nMaxOrder) // nMaxOrder is ONLY set for v2.1
                                throw LW::Exception(ERR_IO, "Language Model file didn't contain all the ngram orders promised in header");
                            break;
                        }
                        
                        if (nGramOrder > nMaxOrder)
                            break;
                        
                        // The n-gram order must come in order, starting with one, no gaps
                        if (nGramOrder != prev_order + 1) {
                            throw LW::Exception(ERR_IO, "Invalid Language Model: n-gram order not successive.");
                        }
                        prev_order=nGramOrder;

                        if (nGramOrder == nMaxOrder)
                            highest.init_read(in);
                        else
                            lower[nGramOrder-1].init_read(in);
                }
                if (prev_order==0)
                    throw LW::Exception(ERR_IO,"LM file had no ngrams (max order = 0)");
                n_lower=prev_order-1;
                max_order=prev_order;

                // Update the last "fake" entry that is used to tell the number of children in the previous entry
                // We only update the non-terminal nodes. A terminal node does not have this value
                for (size_t i = 0; i < n_lower; i++) {
                    lower_node& n = lower[i];
                    n.getEntry(n.getCount())->children=
                        i+1==n_lower ? highest.getCount() : lower[i+1].getCount();
                }
        } else
            n_lower=0;
}

void LangModelSA::write(std::ostream& out, LangModel::SerializeFormat nFormat) 
{
        switch (nFormat) {
                case LangModel::FORMAT_TEXT:
                        writeText(out);
                        break;
                //case LangModel::FORMAT_BINARY_TRIE:
                //      writeBinary(out);
                //      break;
                //case LangModel::FORMAT_BINARY_SA:
                //      writeBinary2(out);
                //      break;
                default:
                        throw Exception(ERR_IO, "Unsupported file format for serialization.");
        }
}

void LangModelSA::writeTextNGram(
        ostream& out,
        unsigned int nOrder,
        word_type *pCurrentNGram,
        double dProb,
        double dBOW)
{
        // Text LM conventions -99 means minus infinity (0 prob)
        if (dProb <= -98) {
                dProb = -99;
        }
        if (dBOW <= -98) {
                dBOW = -99;
        }
        out << dProb;
        for (unsigned int i = 0; i < nOrder; i++) {
                out << "\t";
                out << m_pVocab->getWord(pCurrentNGram[i]);
        }
        if (dBOW != 0) {
                out << "\t";
                out << dBOW;
        }

        out << "\n";
}

void LangModelSA::writeTextNGrams(
        ostream& out, 
        unsigned int nOrder,
        unsigned int nCurrentOrder,
        word_type *pCurrentNGram, 
        int nStartIndex,
        int nEndIndex // This value is included
        )
{
        // Terminal node?
    if (nCurrentOrder==max_order) {
        highest_node* pNode = &highest;
        highest_entry* pEntry;

                for (int i = nStartIndex; i < nEndIndex; i++) {
                        // Add the current word ID to the NGram
                        pEntry = pNode->getEntry(i);
                        pCurrentNGram[nCurrentOrder - 1] = pEntry->word;
                        // Did we reach the desired depth?
                        if (nOrder == nCurrentOrder) {
                                // Yes, just write the ngram
                                writeTextNGram(out, nOrder, pCurrentNGram, pEntry->prob, 0);
                        }
                        else {
                                // No, this must be an error, as this is a terminal node
                                throw Exception(ERR_IO, "Inconsistent Language Model (SA type).");
                        }
                }
        }
        else {
            lower_node* pNode = &lower[nCurrentOrder-1];
            int i = nStartIndex;
            lower_entry *pEntry,*pNextEntry;
            for (pNextEntry=pNode->getEntry(i); i < nEndIndex; i++) {
                        // Add the current word ID to the NGram
                    pEntry = pNextEntry;
                    pNextEntry = pNode->getEntry(i + 1);
                    
                        pCurrentNGram[nCurrentOrder - 1] = pEntry->word;
                        // Did we reach the desired depth?
                        if (nOrder == nCurrentOrder) {
                                // Yes, just write the ngram
                                writeTextNGram(out, nOrder, pCurrentNGram, pEntry->prob, pEntry->bow);
                        }
                        else {
                                // No, keep going
                                writeTextNGrams(
                                        out, 
                                        nOrder, 
                                        nCurrentOrder + 1, 
                                        pCurrentNGram, 
                                        pEntry->children,
                                        pNextEntry->children - 1);
                        }
                }
        }
}


void LangModelSA::writeText(std::ostream& out)
{
        out << endl;
        out << "\\data\\" << endl;
        out << endl;

        // Write out the header (total counts by order)
        for (size_t nOrder = 1; nOrder <= getMaxOrder(); nOrder++)
            out << "ngram " << nOrder << "=" << n_ngrams(nOrder) << endl;

        out << endl;

        LWVocab::WordID currentNGram[MAX_SUPPORTED_ORDER];

        // Write out the actual n-grams
        for (unsigned int nOrder = 1; nOrder <= getMaxOrder(); nOrder++) {
                out << "\\" << nOrder << "-grams:" << endl;

                writeTextNGrams(out, nOrder, 1, currentNGram, 0, n_ngrams(nOrder));
                
                out << endl;
        }

        out << "\\end\\";
}

LangModelImplBase::logprob LangModelSA::find_bow(iterator b,iterator end) const
{
    if (b>=end) return BOW_NONE;
    index_t lb=0,ub=whole_upper(),i; //FIXME: this same pattern of traversing the trie is repeated a few times.  iterator object would be heavy, though, and closures/visitors would be even more code
    for (lower_node const*n=lower,*nend=lower+n_lower;n!=nend;++n) {
        lower_entry const*e;
        if (!n->find(*b++,lb,ub,e,i))
            return BOW_NONE;
        else if (b==end)
            return e->bow;
        lb=e->children;
        ub=n->getEntry(i+1)->children;
    }
    // I decided that I don't want find_bow to give prob, but just bow (i.e. suitable for finding longest equivalent history)
    /*
    if (b==end) return BOW_NONE; // not a redundant check.
    highest_entry const*h;
    if (highest.find(*b,lb,ub,h,i))
        return h->prob;
    */
    return BOW_NONE;
}

LangModelImplBase::logprob LangModelSA::find_prob(iterator b,iterator end) const
{
    if (b>=end) return BOW_NONE;
    index_t lb=0,ub=whole_upper(),i; //FIXME: this same pattern of traversing the trie is repeated a few times.  iterator object would be heavy, though, and closures/visitors would be even more code
    for (lower_node const*n=lower,*nend=lower+n_lower;n!=nend;++n) {
        lower_entry const*e;
        if (!n->find(*b++,lb,ub,e,i))
            return BOW_NONE;
        else if (b==end)
            return e->prob;
        lb=e->children;
        ub=n->getEntry(i+1)->children;
    }
    if (b==end) return BOW_NONE; // not a redundant check.
    highest_entry const*h;
    if (highest.find(*b,lb,ub,h,i))
        return h->prob;
    return BOW_NONE;
}


LangModelImplBase::iterator LangModelSA::longest_prefix(iterator b,iterator end) const
{
    index_t lb=0,ub=whole_upper(),i;
    for (lower_node const*n=lower,*nend=lower+n_lower;;++n,++b) {
        if (b==end) return b;
        if (n==nend) break;
        lower_entry const*e;
        if (!n->find(*b,lb,ub,e,i))
            return b;
        lb=e->children;
        ub=n->getEntry(i+1)->children;
    }
/* we want prefix/suffix to only work on contexts now, just like find_bow.  but if you cared about full length prob answer (e.g. find out what ngram was used, reenable this */
#if 0
    highest_entry const*h;
    if (highest.find(*b,lb,ub,h,i))
    ++b;
#endif 
    return b;
}

bool LangModelSA::prob_starting(iterator begin,iterator end,log_prob &p_accum)
{
    //assert(end-b)<=getMaxOrder() && end>b);
    if (begin>=end) {
        p_accum=PROB_LOG_ZERO;
        return true;
    }   
    log_prob bo=0;
    index_t lb=0,ub=whole_upper(),i;
    iterator b=begin;
    for (lower_node const*n=lower,*nend=lower+n_lower;n!=nend;++n) {
        lower_entry const*e;
        word_type w=*b++;
        if (n->find(w,lb,ub,e,i)) { 
            if (b==end) {
                p_accum+=e->prob;
                return true;
            } else
                bo=e->bow;
            lb=e->children;
            ub=n->getEntry(i+1)->children;
        } else {
            if (b==end)
                p_accum+=bo;
            return false;
        }
    }
    word_type w=*b;
    highest_entry const*h;
    if (highest.find(w,lb,ub,h,i)) {
        p_accum+=h->prob;
        return true;
    } else {
        p_accum +=bo;
        return false;
    }
}


LangModel::ProbLog LangModelSA::getContextProb(iterator pContext, unsigned int nOrder) 
{
    iterator end=pContext+nOrder;
    if (nOrder > getMaxOrder()) {
        nOrder = getMaxOrder();
        pContext=end-nOrder;
    }
    log_prob p=0;
    while (!prob_starting(pContext++,end,p)) ;
    return p;
}

} // namespace LW
