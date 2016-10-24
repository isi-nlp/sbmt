// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _LiBESet_H_
#define _LiBESet_H_

#include <set>


using namespace std;


template<class T>
class LiBESet : public set<T>
{
    typedef set<T> _base;
public:

    typedef typename _base::iterator iterator;
    typedef typename _base::const_iterator const_iterator;

    //! Input operator
    friend istream& operator>>(istream& in, LiBESet& se){
	string buffer;
	se.clear();
	getline(in, buffer,'\n');
	istringstream ist(buffer);
	T ft;
	while(ist >> ft) {
	    se.insert(ft);
	}
	return in;
    }

    /** 
    *   Outputs a LiBESet into one line. Fields are separated by
    *   space.
    */
    friend ostream& operator<<(ostream& out, const LiBESet<T>& se)
    {
	typename LiBESet<T>::const_iterator i;
	for(i = se.begin(); i != se.end(); i++) {
	    out << *i << ' ';
	}
	out<<endl;
	return out;
    }

};



#endif
