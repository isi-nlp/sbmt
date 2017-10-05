// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _LiBEList_H_
#define _LiBEList_H_
#include <list>

using namespace std;

template<class T>
class LiBEList : public  list<T>
{
    typedef list<T>  _base;
public:

    LiBEList()  {}

    typedef typename _base::iterator iterator;
    typedef typename _base::const_iterator const_iterator;

    //! Input operator
    friend istream& operator>>(istream& in, LiBEList& se){
	string buffer;
	se.clear();
	getline(in, buffer,'\n');
	istringstream ist(buffer);
	T ft;
	while(ist >> ft) {
	    se.insert(se.end(), ft);
	}
	return in;
    }

    /** 
    *   Outputs a LiBEList into one line. Fields are separated by
    *   space.
    */
    friend ostream& operator<<(ostream& out, const LiBEList<T>& se)
    {
	typename LiBEList<T>::const_iterator i;
	for(i = se.begin(); i != se.end(); i++) {
	    out << *i << ' ';
	}
	out<<endl;
	return out;
    }


};

#endif

