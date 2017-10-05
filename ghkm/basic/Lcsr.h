// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/*
 * Lcsr: Fuctor to compute the longest common subsequence
 *       ratio.
 */

#ifndef _Lcsr_H_
#define _Lcsr_H_

#include <string>
#include <vector>
#include <algorithm>
#include "LiBEDefs.h"

using namespace std;

/**
 * Lcsr: Fuctor to compute the longest common subsequence
 *       ratio.
 */
class  Lcsr {
public:
    //! Computes the common subsequence ratio of two strings.
    //! If the third argument is true, we only consider the
    //! the longest common CONTINUOUSE subsequence.
    SCORET operator()(const string w, const string v, bool continuous=false) const
    {
	// when both string are of size 0, then their
	// lcsr is 0.
	if(w.length() == 0 || v.length() == 0) return 0.0;

	SCORET s;
	
	if(continuous){
	    // longest common contineous subsequence.
	    s= lccs(w,v)/ (SCORET)(max(w.length(), v.length()));
	} else {
	    // longest common subsequence.
	    s= lcs(w,v)/ (SCORET)(max(w.length(), v.length()));
	}

	return s;
    }

    //! longest common subsequence of two strings.
    //! This method is case insensitive.
    size_t lcs(const string ww, const string vv) const
    {
	string w = ww ;
	string v = vv;

	size_t size;

	//  convert the strings to lower cases.
	for(size = 0; size < w.length(); ++size){ w[size] = tolower(w[size]);}
	for(size = 0; size < v.length(); ++size){ v[size] = tolower(v[size]);}


	size_t i=0, j=0;
	vector< vector<int> > m;
	vector< vector<int> > :: iterator it;

	m.resize(w.length());

	// the following code computes the lcs using
	// the dynamic programming.
	for(it = m.begin(); it != m.end(); ++it){
	    (*it).resize(v.length());
	}

	if(w.length() == 0 || v.length() == 0){
	    return 0;
	}

	// initializaiton 1.
	if(w[0] == v[0]){
	    m[0][0] = 1;
	} else {
	    m[0][0] = 0;
	}

	// initializaiton 2.
	for(i = 1; i < w.length(); ++i){
	    if(w[i] == v[0]){
		m[i][0] = 1;
	    } else {
		m[i][0] = m[i-1][0];
	    }
	}

	// initializaiton 3.
	for(j = 1; j < v.length(); ++j){
	    if(v[j] == w[0]){
		m[0][j] = 1;
	    } else {
		m[0][j] = m[0][j-1];
	    }
	}

	// recursion.
	for(i = 1; i < w.length(); ++i){
	    for(j = 1; j < v.length(); ++j){
		if(w[i] == v[j]){
                    m[i][j] = m[i-1][j-1] + 1;
		} else {
		    m[i][j] = max(m[i-1][j], m[i][j-1]);
		}
	    }
	}

	return m[w.length()-1][v.length()-1];
    }

    //! longest continuous common subsequence.
    size_t lccs(const string ww, const string vv) const
    {
	size_t i,j;

	string w = ww;
	string v = vv;

	// convert the strings to lower case.
	for(i = 0; i < w.length(); ++i){ 
	    w[i] = tolower(w[i]);
	}
	for(i = 0; i < v.length(); ++i){ 
	    v[i] = tolower(v[i]);
	}

	// the continuous strings are found
	// using the string matching operation.
	size_t match = 0;
	for(i = 0; i < w.length() ; ++i){
	    for(j = i + 1; j < w.length()+1; ++j){
		if(j - i > match){
		    string substr(w, i, j-i);
		    if(v.find(substr) != string::npos){
			match = substr.length();
		    }
		}
	    }
	}

	return match;
    }
};


#endif
