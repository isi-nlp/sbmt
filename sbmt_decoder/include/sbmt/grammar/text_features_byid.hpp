#ifndef SBMT__GRAMMAR__TEXT_FEATURE_VECTOR_BYID_HPP
#define SBMT__GRAMMAR__TEXT_FEATURE_VECTOR_BYID_HPP

#include <sbmt/grammar/features_byid.hpp>
#include <graehl/shared/word_spacer.hpp>

namespace sbmt {

//FIXME: similar enough to features_byid.hpp that we should template (only value type differs)
typedef std::pair<feature_id_type,std::string> text_byid_type;
typedef std::vector<text_byid_type > texts_byid_type;

struct text_feature_vector_byid : public texts_byid_type
{
    void add_byid(feature_id_type id,std::string const& text)
    {
       // if (!text.empty()) // FIXME: may be redundant if BRF_ARCHIVE_CLEAR_MAP defined in brf_archive_io.cpp (that is: contract is that syntax reader callbacks are given sparse maps ...
            ((texts_byid_type*)this)->push_back(text_byid_type(id,text));
    }
    void insert(text_byid_type const& p, std::vector<text_byid_type>::iterator m)
    {
        add_byid(p.first,p.second);
    }
    void insert(text_byid_type const& p)
    {
        add_byid(p.first,p.second);
    }
    
    void add(std::string const& name,std::string const& text,feature_names_type &dict)
    {
        add_byid(dict.get_index(name),text);
    }
    
    template <class features_type>
    void text_features_byid(features_type const& scores,feature_names_type &dict) 
    {
        set(scores,dict);
    }

    void prep(std::size_t sz) 
    {
        this->clear();
        this->reserve(sz);
    }
    
    template <class features_type>
    void set(features_type const& scores,feature_names_type &dict) 
    {
        prep(scores.size());
        for (typename features_type::const_iterator i=scores.begin(),e=scores.end();
             i!=e;++i) {
            add(i->first,i->second,dict);
        }
    }
    
    template <class O,class Dict>
    void print(O&o,Dict const& dict) const
    {
        graehl::word_spacer sp;
        for (const_iterator i=begin(),e=end();i!=e;++i)
            o << sp << dict.get_token(i->first)<<"={{{"<<i->second<<"}}}";
    }
};

template <class O>
void print(O& o,text_feature_vector_byid const& fv, feature_names_type const& dict)
{
    fv.print(o,dict);
    /*
    for (text_feature_vector_byid::const_iterator i=fv.begin(),e=fv.end();i!=e;++i)
        out << sp << dict.get_token(i->first) << "=" << i->second;
    */
}
    
}//sbmt


#endif
