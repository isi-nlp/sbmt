#ifndef __constituent_list_hpp__
#define __constituent_list_hpp__

#include <boost/regex.hpp>
#include <map>
#include <string>
#include <set>
#include "sbmt/span.hpp"

using namespace std;
using namespace boost;
using namespace sbmt;

namespace source_structure {

/// ordering is implemented by a hash map.
class constituent_list : public map<sbmt::span_t, std::set<std::string> > {
    typedef map<sbmt::span_t, std::set<std::string> > __base;

    map<sbmt::span_index_t, std::set<std::string> > rightadjacency;
    map<sbmt::span_index_t, std::set<std::string> > leftadjacency;

public:
    typedef std::set<std::string> data_type;
    typedef __base::const_iterator const_iterator;
    typedef __base::iterator iterator;
    constituent_list(string s) {
        regex  conss("(\\S+\\[[0-9]+,[0-9]+\\])");
        regex  cons("(\\S+)\\[(\\d+),(\\d+)\\]");

        sregex_iterator m1(s.begin(), s.end(), conss);
        sregex_iterator m2;
        BOOST_FOREACH( const match_results<string::const_iterator> & what, make_pair(m1,m2)){
            match_results<string::const_iterator> what1;
            string ss=what[1].str();
            if(regex_match(ss, what1, cons)){
                sbmt::span_index_t left=atoi(what1[2].str().c_str());
                sbmt::span_index_t right=atoi(what1[3].str().c_str());
                (*this)[sbmt::span_t(left, right)].insert(what1[1].str());
                if(left!=0){
                    rightadjacency[left-1].insert(what1[1].str());
                }
                leftadjacency[right].insert(what1[1].str());
            }
        }
        return;
    }

    data_type leftadj(const span_index_t posit) const { 
        map<sbmt::span_index_t, std::set<std::string> >::const_iterator it = leftadjacency.find(posit) ;
        if(it != leftadjacency.end()){
            return it->second;
        } else {
            return data_type();
        }
    }
    data_type rightadj(const span_index_t posit) const { 
        map<sbmt::span_index_t, std::set<std::string> >::const_iterator it = rightadjacency.find(posit) ;
        if(it != rightadjacency.end()){
            return it->second;
        }else {
            return data_type();
        }
    }

    void dump() const {
        const_iterator it;
        for(it=begin();it!=end();++it){
            cout<<it->first<<" ";
            copy(it->second.begin(), it->second.end(), ostream_iterator<string> (cout, " "));
            cout<<endl;
        }

    }
};

}

#endif
