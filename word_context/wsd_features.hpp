#ifndef __wsd_features_hpp__
#define __wsd_features_hpp__

#include <ww/ww_utils.hpp>

using namespace stlext;

namespace word_context {

class wsd_features {
public:
    wsd_features() {}

    typedef tuple<bool, string, string, string> word_context_type;
    typedef hash_map<string, string> vocab_type;

    word_context_type
    transform(const word_context_type& tup) const {
        // get<0>: bool : false -> fprev, true, fafter
        // get<1>: f-1 or f+1
        // get<2>: f
        // get<3>: e
        word_context_type res = tup;
        vocab_type::const_iterator vit;
        // tranform f-1 or f+1.
        transform_helper<1>(res, m_src_list);
        // tranform f.
        transform_helper<2>(res, m_src_list);
        // tranform e.
        transform_helper<3>(res, m_trg_list);

        return res;
    }


    //// load the vocabulary lists.
    void load(string file_src, string file_trg)
    {
        load(file_src, m_src_list);
        load(file_trg, m_trg_list);
    }

private:

    template<int N>
    void transform_helper(word_context_type& wc, const vocab_type& vcb) const {
        vocab_type::const_iterator vit;
        vit=vcb.find(boost::get<N>(wc));
        if(vit ==vcb.end()){ 
            boost::get<N>(wc) = "unk";
            //std::cout<<boost::get<N>(wc)<<" <unk>"<<std::endl;
        } else {
            if(vit->second != ""){
                //std::cout<<boost::get<N>(wc)<<vit->second<<std::endl;
                boost::get<N>(wc) = vit->second;
            } else {
                //std::cout<<boost::get<N>(wc)<<" only first"<<std::endl;
                boost::get<N>(wc) = vit->first;
            }
        }
    }

    void load(string file, vocab_type& vocab)
    {
        ifstream in(file.c_str());
        string line;
        while(getline(in, line)){
            istringstream ist(line);
            string word, mapped;
            ist>>word;
            if(ist>>mapped){
                vocab[word]=mapped;
            } else {
                vocab[word]="";
            }
        }
        in.close();
    }

    vocab_type m_src_list;
    vocab_type m_trg_list;
};


} // word_context


#endif
