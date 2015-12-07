// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

/**

\defgroup LangModelSA LW Sorted Array ngram trie

Comments from Jonathan Graehl - graehl@isi.edu

While the choice of data structure for a trie is pretty clever/compact (leaving
out only bit-shaving techniques that sacrifice # of words or precision in
logprobs), the order of the ngrams is all wrong (for the common p(word|history)
- the only method really used so far)

Suppose you have phrase:

w0 w1 w2 w3

and you're going to compute p(w3|w0...w2)

What's done now (wrong): is to store in the trie (left->right) "w0 w1 w2 w3", and if that's not found, look up "w0 w1 w2" for the backoff cost, and "w1 w2 w3" for the backed off prob (recursing).

I have improved this somewhat so that instead of 2*n trie lookups, only n are used (if we get to "w0 w1 w2", we save the backoff then, just in case we fail to see "w3" next.

However, the ideal trie organization would be:

right->left context, followed by (max order) word.  e.g.:

w2,w1,w0.w3 (.w3 means we're scoring p(w3|preceding context)

Here's what we'd do:

.w3 (unigram prob)
BO:w2 (accumulate backoff)
w2.w3 (bigram prob - if found, clear backoff)
BO:w2,w1 (accum backoff)
w2,w1.w3 (trigram - if found, clear backoff)
BO:w2,w1,w0 (accum backoff)
w2,w1,w0.w3 (max order)

Note: early exit if extending the history with ,wn gives a failed lookup.  use the most recent found prob and all the accumulated backoff

Advantage: trie node lookups are as few as possible.

Disadvantage: look up two words per level.  If every max-n-gram is known, then simple right->left (or left->right, doesn't matter) would do only N words lookup, whereas 2N-1 w/ this approach.

Alternative: strict right->left

w3,w2,w1,w0

w3 (unigram)
w3,w2 (bigram)
w3,w2,w1 (3gram - now, suppose this isn't found)
BO:w2,w1 (start backing off at w1 - the word we just failed to find above)
BO:w2,w1,w0

The right->leftness is best when we're interested in scoring a particular word.

Note that in all cases the data structures can be the same.  In the w2,w1,w0.w3 variant, the prob attached to a node in the trie "a b" is p(b|a), while the bow is for backing off from "b a" (confusing? not really).  In the strict left->right and right->left, the bow and prob are for the same sequence of words.

Finally: if we're building on 64bit, we should do away with the segmented array (better performance).  Is there a #define that's always available on 64 bit?  in templates, we can test sizeof(long).  or we can create our own #define so 32-bit users who know their OS allocator isn't lame can use flat arrays too.

*/

#ifndef _LANG_MODEL_SA_H
#define _LANG_MODEL_SA_H

#include <vector>
#include <assert.h>

//#include "Common/MemAlloc.h"
#include "Common/ErrorCode.h"
#include "LangModel/LangModel.h"
#include "LangModelImplBase.h"
#include "Common/Serializer/Serializer.h"

// Extracts the lower bits of the number the represent the index
#define INDEX(index) (index & MASK)
// Extracts the higher bits of the number that represent the subnode
#define SUBNODE(index) ((index & (~MASK)) >> MASK_BITS)

// Two is these values are redundant, but they enable the compiler
// to generate really efficient code

//#define TEST_EXTREME

#ifdef TEST_EXTREME
// THIS NUMBER MUST BE A POWER OF 2
#define MAX_ENTRIES_PER_NODE (4u)
// The number of bits in the mask
#define MASK_BITS (2u)
// The mask
#define MASK (MAX_ENTRIES_PER_NODE - 1)

#else

// THIS NUMBER MUST BE A POWER OF 2 - if you modify it make sure you know what you are doing
#define MAX_ENTRIES_PER_NODE (4 * 1024 * 1024)
// The number of bits in the mask
#define MASK_BITS (22u)
// The mask
#define MASK (MAX_ENTRIES_PER_NODE - 1)
#endif


namespace LW {
            typedef unsigned index_t;

class LangModelSANodeEntryShort {
        public:
                /// Word, as an ID
                LWVocab::WordID word;
                /// Probability as a lon
                LangModel::ProbLog prob;
            void init_fake() 
            {
                word=0;
                prob=0;
            }
            void read_base(ISerializer &in) 
            {
                in.read(word);
                in.read(prob);
                // By convention -98 or less means PROB_LOG_ZERO
                if (prob <= -98)
                    prob = PROB_LOG_ZERO;
            }
            void read(ISerializer &in) 
            {
                read_base(in);
                LangModel::ProbLog d1;
                index_t d2;
                in.read(d1);
                in.read(d2);

            }
            
        };

        class LangModelSANodeEntryLong : public LangModelSANodeEntryShort {
        public:
            /// Back off weight as a log
                LangModel::ProbLog bow;
                /// Offset of the first child. The last child offset is defined by 
                /// by the next sibling of this node (it's first child - 1)
                index_t children;
            void init_fake()
            {
                LangModelSANodeEntryShort::init_fake();
                bow=0;
                children=0x7FFFFFFF;
            }
            void read(ISerializer &in) 
            {
                read_base(in);
                in.read(bow);
                in.read(children);
            }
        };



        template <class NodeEntryType>
        class LangModelSANode{
                friend class LangModelSA;
            typedef LangModelSANode<NodeEntryType> self_type;
        public:
            static const index_t NOT_FOUND =(index_t)-1;
                /**
                        @arg nOrder n-gram order this node is for
                        @arg nNodeCount the total number of n-grams in this node
//                      @arg pMemAlloc memory allocator used to allocate the array of nodes. 
                */
            LangModelSANode()  : m_pNodes(0) {}
            void init(unsigned int nNodeCount);
            void init_read(ISerializer &in) 
            {
                unsigned nGramCount;
        //              cerr << "Reading n-gram count for probs..." << endl;
                in.read(nGramCount);
        //              cerr << "Order: " << nGramOrder << " Count: " << nGramCount << endl;                
                init(nGramCount);
                for (unsigned i=0;i<nGramCount;++i)
                    getEntry(i)->read(in);
                loadCompleted();
            }
                /**
                */
                ~LangModelSANode();
        public:
                void loadCompleted();
        public:
            typedef LWVocab::WordID word_type;
                /// Performs a binary search on [start,end), returning index of node and the node.  true if found, false otherwise
//            index_t find(word_type nWord, index_t start,index_t end) const;
            bool find(word_type nWord, index_t start,index_t end,NodeEntryType const*& p, index_t &i) const;
            /*
            index_t find(word_type nWord, index_t start,index_t end,NodeEntryType *& p) const 
            {
                return find(nWord,start,end,reinterpret_cast<NodeEntryType const*&>(p));
            }
            */


                /// This method performs a "regional" sort in between the 2 indexes
//              void sort(int nStartIndex, int nEndIndex);
                /// Returns whether this node is terminal node
                bool isTerminal() const {
                    //   return m_bIsTerminal;
                    return sizeof(NodeEntryType)==sizeof(LangModelSANodeEntryShort);
                }
            
        public:
                ///// Returns a pointer to the internal buffer where the entries are stored
                ///// Called by LangModelSA (fried) to increase loading performance
                //NodeEntryType* getEntries();
                /// Returns an entry for the specified index
            inline NodeEntryType const* getEntry(unsigned i) const
            {
                return const_cast<self_type *>(this)
                    ->getEntry(i);
            }
            
                inline NodeEntryType* getEntry(unsigned int nIndex) {
                        //assert(nIndex <= m_nNodeCount);

                        unsigned int nSubnode = SUBNODE(nIndex);
                        unsigned int nLocalIndex = INDEX(nIndex);

                        return m_pNodes[nSubnode] + nLocalIndex;
                }

        public:
            
                /// Returns the number of entries
            unsigned getCount() const {return m_nNodeCount;};
        private:
            
                /// Is this node a terminal node
//              bool m_bIsTerminal;
                /// Order of the node
            //          unsigned int m_nOrder;
                /// How many chidren it contains
                unsigned int m_nNodeCount;
                /// The subnodes are grouped in continous areas. How many?
                unsigned int m_nSubnodeCount;
                /// Children nodes. Each node is represented by a LangModelSANodeEntryLong
                NodeEntryType** m_pNodes;
//              /// This is a mask for the number of bits that are the actual index
//              /// in the subnodes (lower bits). The higher bits are an index in the subnode
//              unsigned int m_nMask;
//              /// The number of significant bits in the index mask
//              unsigned int m_nMaskBitCount;
        };

class LangModelSA : public LangModelImplBase
{
 public:
    typedef LangModel::ProbLog log_prob;
    typedef LangModelSANodeEntryShort highest_entry;
    typedef LangModelSANode<highest_entry> highest_node;
    typedef LangModelSANodeEntryLong lower_entry;
    typedef LangModelSANode<lower_entry> lower_node;
            
 public:
    LangModelSA(LWVocab* pVocab, const LangModelParams& params);
    virtual ~LangModelSA();
 public: // Virtual pure methods we dont' implement. They all throw if called
    /// Clears all counts and probabilities and frees memory associated with them
    virtual void clear() 
    {throw Exception(ERR_NOT_IMPLEMENTED, "Method clear()not implemented by class LangModelSA");};
    /// Performs training o a sentence
    virtual void learnSentence(iterator pSentence, unsigned int nSentenceSize)
    {throw Exception(ERR_NOT_IMPLEMENTED, "Method learnSentence()not implemented by class LangModelSA");};
    /// Called after learning is finishted to prepare counts, etc.
    virtual void prepare()
    {throw Exception(ERR_NOT_IMPLEMENTED, "Method prepare()not implemented by class LangModelSA");};
    /// Loads the language model counts from a file (previously saved with writeCounts())
    virtual void readCounts(std::istream& in)
    {throw Exception(ERR_NOT_IMPLEMENTED, "Method readCounts()not implemented by class LangModelSA");};
    /// Writes the language model counts to a file
    virtual void writeCounts(std::ostream& out)
    {throw Exception(ERR_NOT_IMPLEMENTED, "Method writeCounts()not implemented by class LangModelSA");};
    /// Writes the language model in human readable format
    virtual void dump(std::ostream& out)
    {throw Exception(ERR_NOT_IMPLEMENTED, "Method dump()not implemented by class LangModelSA");};
 public: // Virtual pure methods we implement
    /// This method can only read binary data created for this type of LM (FORMAT_BINARY_SA)
    /// It will throw exceptions for any other kind of file format
    virtual void read(std::istream& in, LangModel::ReadOptions nOptions = LangModel::READ_NORMAL);
    /// Writes the language model to a file
    virtual void write(std::ostream& out, LangModel::SerializeFormat nFormat);
    /// Returns the probability of a sequence of words of size order
    virtual LangModel::ProbLog getContextProb(iterator pNGram, unsigned int order);

    virtual logprob find_prob(iterator b,iterator end) const;
    virtual logprob find_bow(iterator b,iterator end) const;
    virtual iterator longest_prefix(iterator b,iterator end) const;

 protected:
    unsigned n_ngrams(unsigned order) const 
    {
        if (order < max_order)
            return lower[order-1].getCount();
        else
            return highest.getCount();
    }
    
    void writeTextNGram(
        std::ostream& out,
        unsigned int nOrder,
        word_type *pCurrentNGram,
        double dProb,
        double dBOW);
    void writeTextNGrams(
        std::ostream& out, 
        unsigned int nOrder,
        unsigned int nCurrentOrder,
        word_type *pCurrentNGram, 
        int nStartIndex,
        int nEndIndex // This value is included
        );
    void writeText(std::ostream& out);
 public:
//              /// This particular langauge model can only be used if the nodes are sorted
//              /// This method will check if they are and, if they are not, will perform the sort
//              void sort();
    /// Returns the max order supported by this LM
    unsigned int getMaxOrder()const {return max_order;};
            
    /// lp+=log(p(end|b...end-1) and returns true, or p_bo(b...end-1) and returns false (need to call prob_starting(b+1,end,p_accum)
    bool prob_starting(iterator begin,iterator end,log_prob &p_accum);
            
 private:            
    unsigned n_lower; // max_order-1
    unsigned max_order;
    unsigned whole_upper() const 
    {
        return n_lower ? lower[0].getCount() : highest.getCount();
    }
    /// This vector has one element per n-gram order level except the highest
    /// For a LM that supports 3-grams, we have 2 elements
    highest_node highest; /// holds highest order
    lower_node lower[MAX_SUPPORTED_ORDER-1];
    /// The memory allocator used for large chunks of memory
    //              MemAlloc* m_pMemAlloc;
    /// Vocabulary instance
    LWVocab* m_pVocab;
    LangModelParams m_params;
};

} // namespace

#endif // #ifdef _LANG_MODEL_SA_H
