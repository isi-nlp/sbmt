#include "sbmt/dependency_lm/DLM.hpp"
#include <graehl/shared/word_spacer.hpp>
#include <boost/tokenizer.hpp>
#include <sbmt/ngram/lw_ngram_lm.hpp>
#include <sbmt/ngram/big_ngram_lm.hpp>
#include <sbmt/ngram/cached_ngram_lm.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <boost/function_output_iterator.hpp>

namespace {
struct nullout {
    template <class X> void operator()(X const& x) const { return; }
};

boost::function_output_iterator<nullout> nulloutitr;
} 

using namespace std;

namespace sbmt{

// i.e., name: lw=file1,lw=file2,big=file3
SwitchDLM::SwitchDLM(const string filename)
{
    cerr<<"DLM file name "<<filename<<endl;
    boost::char_delimiters_separator<char> sep(false, "", ",");
    boost::tokenizer<> toker(filename, sep);
    boost::tokenizer<>::iterator itr ;
    for(itr = toker.begin(); itr != toker.end(); ++itr){
        boost::char_delimiters_separator<char> sep1(false, "", "=");
        boost::tokenizer<> toker1(*itr, sep1);
        boost::tokenizer<>::iterator itr1 ;
        vector<string> ss;
        for(itr1 = toker1.begin(); itr1 != toker1.end(); ++itr1){
            ss.push_back(*itr1);
        }
        assert(ss.size() == 2);
        cerr<<"SwitchDLM: type "<<ss[0]<<" filename "<<ss[1]<<endl;

        if(ss[0] == "lw"){
            m_dlms.push_back(new lw_ngram_lm(ngram_options()));
            cerr<<"Reading DLM in "<<ss[0]<<" format from file "<<ss[1]<<" ";
            ((lw_ngram_lm*)m_dlms.back())->read(ss[1].c_str());
            cerr<<" done.\n";
        } else if(ss[0] == "big"){
            ss[1] = "["+ss[1]+"]";
            istringstream ist(ss[1]);
            cerr<<"Reading DLM in "<<ss[0]<<" format from file "<<ss[1]<<" ";
             m_dlms.push_back(new cached_ngram_lm<big_ngram_lm>(ngram_options(), ist, boost::filesystem::initial_path(), 5000000));
            cerr<<" done.\n";
        } else {
            std::stringstream sstr;
            sstr<<"SwitchDLM: unrecognized LM type: "<<ss[0]<<endl;
            throw std::runtime_error(sstr.str());
            //exit(1); // exits are bad for c++ code.  they dont allow destructors
                       // to be called --michael
        }
        m_dlms.back()->own_weight = 1.0;
    }
}


SwitchDLM::~SwitchDLM()
{
    for(size_t i = 0; i < m_dlms.size(); ++i){
        delete m_dlms[i];
    }
}

void SwitchDLM::clear()
{
    for(size_t i = 0; i < m_dlms.size(); ++i){
        delete m_dlms[i];
    }
    m_dlms.clear();
}

// if there are more than one submodel in this DLM,
// the first element v[0] of the input v is the index
// of the submodel.
score_t SwitchDLM::prob(const vector<string>& v, int inx, string memo, score_output_iterator out) const
{
    /// \todo we need to get rid of string representations in all dlm calculations.
    std::vector<lm_id_type> ar(v.size()+1);
    for(unsigned j = 0; j < v.size(); ++j){
        ar[j] = m_dlms[inx]->id(v[j]);
    }
    return m_dlms[inx]->prob(&(ar[0]), v.size());
}

std::string const& SwitchDLM::word(lm_id_type id) const
{
    return m_dlms[0]->word(id);
}

// i.e., name: file1;file2
MultiDLM::MultiDLM(const string filename, weight_vector const& weights, feature_dictionary& dict)
{
    boost::char_delimiters_separator<char> sep(false, "", ";:");
    boost::tokenizer<> toker(filename, sep);
    boost::tokenizer<>::iterator itr ;
    for(itr = toker.begin(); itr != toker.end(); ++itr){
        m_dlms.push_back(new SwitchDLM(*itr));
    }
    set_weights(weights,dict);
}

void MultiDLM::create(const string filename)
{
    boost::char_delimiters_separator<char> sep(false, "", ";:");
    boost::tokenizer<> toker(filename, sep);
    boost::tokenizer<>::iterator itr ;
    for(itr = toker.begin(); itr != toker.end(); ++itr){
        m_dlms.push_back(new SwitchDLM(*itr));
    }
}

MultiDLM::~MultiDLM()
{
    for(size_t i = 0; i < m_dlms.size(); ++i){
        delete m_dlms[i];
    }
}

score_t MultiDLM::prob( const vector<string>& v
                      , int inx
                      , string memo
                      , score_output_iterator out) const
{
     //cout<<"DEPENDENCY: "<<memo<<" ";
     //for(int p = 0; p < v.size(); ++p){
     // cout<<" "<<v[p];
     //}
    size_t i;

    score_t retval(1.0);
    for(i = 0; i < m_dlms.size(); ++i){
        //vector<score_t> tmp;
        score_t s = m_dlms[i]->prob(v, inx, memo, score_output_iterator(nulloutitr));
        ostringstream ost;
        ost<<"deplm"<<i+1;
        double w = m_weights[i].second;
        *out = std::make_pair(m_weights[i].first,s);
        ++out;
        retval *= (s ^ w);
    }
    return retval;
}

void MultiDLM::set_weights(weight_vector const& weights, feature_dictionary& dict)
{
    m_weights.clear();
    for (size_t x = 0; x != m_dlms.size(); ++x) {
        std::stringstream namestr;
        namestr << "deplm" << (x + 1);
        boost::uint32_t name = dict.get_index(namestr.str());
        double weight = weights[name];
        m_weights.push_back(std::make_pair(name,weight));
    }
}

std::string const& MultiDLM::word(lm_id_type id) const
{
    return m_dlms[0]->word(id);
}

void MultiDLM::clear()
{
    for(size_t i = 0; i < m_dlms.size(); ++i){
        delete m_dlms[i];
    }
    m_dlms.clear();
}

// special treatment of <top>
void MultiDLM::decompose(ns_RuleReader::Rule& r, ostream& out)
{
    // maps from a RuleNode to its left kids and right
    map<ns_RuleReader::RuleNode*, pair<deque<string>, deque<string> > >  head2depMap;

    ns_RuleReader::RuleNode* headOfRoot = compileHead2DependencyMap(r, r.getLHSRoot(), head2depMap);

    // print the head.

    out<<" <H> ";
    string lab;
    bool is_head_a_var = true;
    if(headOfRoot){
        headOfRoot->isPointer();
        if(headOfRoot->isPointer()) { 
            is_head_a_var = true;
            lab = headOfRoot->get_state();
            if(lab.size() && lab[0] == 'x'){
                //lab[0] = ' ';
                lab.erase(lab.begin());
                istringstream ist(lab);
                int index;
                ist >> index;
                index = r.rhsVarIndex(index);
                ostringstream ost;
                ost << index;
                lab = ost.str();
            }
        }
        else {
            lab = "\"" + headOfRoot->getLabel() + "\"";
            is_head_a_var = false;
        }
        out<<lab<< " </H>";
    } else {
        out<<"\"<top>\" </H>";
        lab = "\"<top>\"";
    }

    // print the boundaries.
    out<<" <LB> ";
    int sz =  head2depMap[headOfRoot].first.size();
    //int b = (0 + DLM::order() - 1 -1>= sz) ? sz : DLM::order() - 1 - 1;
    int b = sz ;

    if(lab == "\"<top>\""){ 
        out<<" "<<lab;
    } else {
        for(int i = 0; i <  b; ++i){
            out<<" "<<head2depMap[headOfRoot].first[i];
        }

        // we dont print the head in the boundary if the head is a token.
        // if the head is a var, we print the index so that in decoding, 
        // we can copy the boundary (excluding head) of the index (edge).
        if(is_head_a_var){ out<<" "<<lab;}
    }

    out<<" </LB>";

    // print the boundaries.
    out<<" <RB> ";
    sz =  head2depMap[headOfRoot].second.size();
    //b = (sz - DLM::order() + 1 + 1< 0) ? 0 : sz - DLM::order() +1 + 1;
    b = 0 ;
    if(lab == "\"<top>\""){ 
        out<<" "<<lab;
    } else {
        // we dont print the head in the boundary if the head is a token.
        // if the head is a var, we print the index so that in decoding, 
        // we can copy the boundary (excluding head) of the index (edge).
        if(is_head_a_var){ out<<" "<<lab;}
        for(int i = b; i <  sz; ++i){
            out<<" "<<head2depMap[headOfRoot].second[i];
        }
    }

    out<<" </RB>";

    // print out the events.
    map<ns_RuleReader::RuleNode*, pair<deque<string>, deque<string> > >:: iterator it;
    for(it = head2depMap.begin(); it != head2depMap.end(); ++it){
        // do this once for each leaf node.
        string lab;
        if(it->first){
            if( it->first->isPointer()) { 
                lab = it->first->get_state();
                if(lab.size() && lab[0] == 'x') { 
                    lab.erase(lab.begin()); 
                    istringstream ist(lab);
                    int index;
                    ist >> index;
                    index = r.rhsVarIndex(index);
                    ostringstream ost;
                    ost << index;
                    lab = ost.str();
                }  
            } else {
                lab = "\"" + it->first->getLabel() + "\"";
            }
        } else {
                lab = "";
                lab = lab + "\"" + "<top>" + "\"";
        }

        // print left events.
        int i ;
        int s = it->second.first.size(); // s can be 0.
        for(i = s - 1; i >=0; --i){
            out<<" <E> <L>";
            //int j = (i + DLM::order() - 1 - 1 > s-1)?  s-1 : i + DLM::order() -1 - 1; // one more -1 is because the head is already in the
            int j = it->second.first.size();
            --j;

            for(int k = j; k >= i; --k){
                if(k == j){ out<<" <PH>";}
                if(k == i) { out<<" "<<lab;}
                out<<" "<<it->second.first[k];
            }
            out<<" </E>";
        }

        // print right events.
        s = it->second.second.size(); // s can be 0.

        for(i=0; i < s; ++i){
            out<<" <E> <R>";
            //int j = (i - DLM::order() + 1 +1  < 0)? 0: i - DLM::order() + 1 + 1; // +1 is because the head is already in the history.
            int j = 0;

            for(int k = j; k <= i; ++k){
                if(k == j){ out<<" <PH>";}
                if(k==i){out<<" "<<lab;}
                out<<" "<<it->second.second[k];
            }
            out<<" </E>";
        }
    } 


}


// special treatment of <top>
void OrdinaryNgramDLM::decompose(ns_RuleReader::Rule& r, ostream& out)
{
    // maps from a RuleNode to its left kids and right
    map<ns_RuleReader::RuleNode*, pair<deque<string>, deque<string> > >  head2depMap;

    ns_RuleReader::RuleNode* headOfRoot = compileHead2DependencyMap(r, r.getLHSRoot(), head2depMap);

    // print out the events.
    map<ns_RuleReader::RuleNode*, pair<deque<string>, deque<string> > >:: iterator it;
    for(it = head2depMap.begin(); it != head2depMap.end(); ++it){
        // do this once for each leaf node.
        string lab;
        if(it->first->isPointer()) { 
            lab = it->first->get_state();
            if(lab.size() && lab[0] == 'x') { lab.erase(lab.begin()); }
        }
        else {lab = "\"" + it->first->getLabel() + "\"";}
        it->second.first.push_back(lab);
        it->second.second.push_front(lab);

        // print left events.
        int i ;
        int s = it->second.first.size(); // s is at least 1.
        for(i = s - 2; i >=0; --i){
            out<<" <E> <L>";
            int j = (i + DLM::order() - 1 > s-1)?  s-1 : i + DLM::order() -1;

            if(it->first->isPointer()) {
                if(i+DLM::order() - 1 > s -1){ 
                    out<<" <PH>";
                }
            }

            //if( i + DLM::order() - 1 > s - 1) 
            for(int k = j; k >= i; --k){
                out<<" "<<it->second.first[k];
            }
            out<<" </E>";
        }

        // print right events.
        s = it->second.second.size(); // s is at least 1.

        for(i=1; i < s; ++i){
            out<<" <E> <R>";
            int j = (i - DLM::order() + 1 < 0)? 0: i + DLM::order() + 1;

            if(it->first->isPointer()) {
                if(i-DLM::order() + 1 < 0){ 
                    out<<" <PH>";
                }
            }


            for(int k = j; k <= i; ++k){
                out<<" "<<it->second.second[k];
            }
            out<<"</E>";
        }
           
    } 

    // print the head.

    out<<" <H> ";
    string lab;
    headOfRoot->isPointer();
    if(headOfRoot->isPointer()) { 
        lab = headOfRoot->get_state();
        if(lab.size() && lab[0] == 'x'){
            //lab[0] = ' ';
            lab.erase(lab.begin());
        }
    }
    else {
        lab = "\"" + headOfRoot->getLabel() + "\"";
    }
    out<<lab<< " </H>";

    // print the boundaries.
    out<<" <LB> ";
    int sz =  head2depMap[headOfRoot].first.size();
    int b = (0 + DLM::order() - 1 >= sz) ? sz : DLM::order() - 1;
    for(int i = 0; i <  b; ++i){
        out<<" "<<head2depMap[headOfRoot].first[i];
    }

    out<<" </LB>";

    // print the boundaries.
    out<<" <RB> ";
    sz =  head2depMap[headOfRoot].second.size();
    b = (sz - DLM::order() + 1 < 0) ? 0 : sz - DLM::order() +1;
    for(int i = b; i <  sz; ++i){
        out<<" "<<head2depMap[headOfRoot].second[i];
    }

    out<<" </RB>";
}

// to add memo-ization of the head.
ns_RuleReader::RuleNode*
MultiDLM::
compileHead2DependencyMap(
    ns_RuleReader::Rule& r, 
        ns_RuleReader::RuleNode* node, 
    map<ns_RuleReader::RuleNode*, pair<deque<string>, deque<string> > >& m)
{
    if(!node){return NULL;}

    std::vector<ns_RuleReader::RuleNode*>* kidsp = node->getChildren();
    std::vector<ns_RuleReader::RuleNode*>& kids = *kidsp;
    
    size_t i;
    ns_RuleReader::RuleNode* headWordNode = NULL;

    if(kids.size() == 0){  // this is a leaf.
        m[node];
        return node;

    } else {

#if 0
        bool hasHead = false;
        for(i = 0; i < kids.size(); ++i){
            if(kids[i]->isHeadNode()){
                hasHead = true; break;
            }
        }
        if(!hasHead && kids.size()){
            kids.back()->set_as_head();
        }
#endif

        deque<string> leftkids;
        for(i = 0; i < kids.size(); ++i){
            if(kids[i]->isHeadNode()){
                headWordNode = compileHead2DependencyMap(r, kids[i], m);
            } else {
                ns_RuleReader::RuleNode* hw = compileHead2DependencyMap(r, kids[i], m);
                string lab;
                if(hw->isPointer()) { 
                    lab = hw->get_state();
                    if(lab.size() && lab[0] == 'x') { 
                        lab[0] = ' ';
                        lab.erase(lab.begin());
                        istringstream ist(lab);
                        int index;
                        ist >> index;
                        index = r.rhsVarIndex(index);
                        ostringstream ost;
                        ost << index;
                        lab = ost.str();
                    }
                }
                else {lab = "\"" + hw->getLabel() + "\"";}

                if(headWordNode) { // we have passed the head node.
                    m[headWordNode].second.push_back(lab);
                } else {
                    leftkids.push_back(lab);
                }
            }
        }
        if(leftkids.size()){
            int ss = leftkids.size() - 1;
            for (int l = ss;  l >=0; l--){
                m[headWordNode].first.push_front(leftkids[l]);
            }
        }

        //if(!headWordNode) { std::cerr<<"WARN No head node in rule "<<node->getString()<<"!\n"; }

        return headWordNode;
    }
}

}
