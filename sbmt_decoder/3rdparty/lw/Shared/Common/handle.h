// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _HANDLE_H
#define _HANDLE_H
#include <iostream>
#include <Common/RefCounted.h>


using std::ostream;

/* WARNING: this implementation of smart pointer is not thread-safe */
template <class CLASSTYPE>
class SmartPtrST {
private:
	/**The object being ref counted.*/
	CLASSTYPE *m_pBody;
	/**The ptr holding the reference count to be shared with other handles.*/
	int *m_pRefCount;

#ifdef _USEHANDLEMEM
	///TODO: synchornization
	static vector<CLASSTYPE *> m_FreeList;
	///TODO: synchornization
	static vector<int *> m_FreeRefList;
#endif

protected:
public:
	///compare based on ptrs as used in boost libs - compatibility with franz code
	bool operator==(const SmartPtrST<CLASSTYPE> &h) const{
		return (m_pBody == h.m_pBody);
	}
	///compare based on ptrs as used in boost libs - compatibility with franz code
	bool operator!=(const SmartPtrST<CLASSTYPE> &h) const{
		return (m_pBody != h.m_pBody);
	}


	SmartPtrST(){ 
		m_pBody = NULL;
		m_pRefCount = new int;
		(*m_pRefCount) = 1;
	}

	SmartPtrST(CLASSTYPE *pBody){ 
		m_pBody = pBody;
		m_pRefCount = new int;
		(*m_pRefCount) = 1;
	}
	/**handle copy constructor*/
	SmartPtrST(const SmartPtrST<CLASSTYPE> &h){
		m_pBody = h.m_pBody;
        m_pRefCount = h.m_pRefCount;
		(*m_pRefCount)++;
	}
	bool isNull() const {
		return (m_pBody == NULL);
	}

	/**Assign the object ptr and increment the ref count for this handle*/
	SmartPtrST & operator=(const SmartPtrST<CLASSTYPE> &h){
		//cout << "=I:" << h.m_pBody->GetId() << " s:"<< h.m_pCount->Inc() << endl;
		(*m_pRefCount)--;
		if((*m_pRefCount) <= 0){
			//cout << "=R:" << m_pBody->GetId() << endl;
			delete m_pBody;
			delete m_pRefCount;
		}


		m_pBody = h.m_pBody;
        m_pRefCount = h.m_pRefCount;
		(*m_pRefCount)++;
		return *this;
	}
	/**Assign the object ptr and increment the ref count for this handle*/
	SmartPtrST & operator=(CLASSTYPE *h){
		//cout << "=I:" << h.m_pBody->GetId() << " s:"<< h.m_pCount->Inc() << endl;
		(*m_pRefCount)--;
		if((*m_pRefCount) <= 0){
			//cout << "=R:" << m_pBody->GetId() << endl;
			delete m_pBody;
			delete m_pRefCount;
		}
		m_pBody = h;
		m_pRefCount = new int;
		(*m_pRefCount) = 1;
		return *this;
	}
	/**compares memory locations only*/
	bool PtrEqual(const SmartPtrST<CLASSTYPE> &h) const{
		if((m_pBody == h.m_pBody) && (m_pRefCount == h.m_pRefCount)) return true;
		return false;
	}

	~SmartPtrST(){
		(*m_pRefCount)--;
		if((*m_pRefCount) <= 0){
			//cout << "~R:" << m_pBody->GetId() << endl;
			delete m_pBody;
			delete m_pRefCount;
		}
	}
	/**raw dump of the objects data*/
	friend ostream& operator <<(ostream &os, SmartPtrST<CLASSTYPE> &h){
		os << *(h.m_pBody);
		return os;
	}
	///returns the content of the SmartPtrST
	CLASSTYPE &operator*() const {return *m_pBody;}
	/**way to access the objects members through the handle*/
	CLASSTYPE *operator->() const {return m_pBody;}

	//this obtains the original object for reference look ups
	//should only be used by alignmentfactory for cloning alignments
	CLASSTYPE& GetBody() const {return (*m_pBody);};

	/// Returns the internal pointer
	CLASSTYPE* getPtr() const {
		return m_pBody;
	}
	/// Returns the internal pointer - compatible with boost shared ptrs for Franz code
	CLASSTYPE* get() const {
		return m_pBody;
	}
	operator bool() const {
        return m_pBody != 0;
	}

	///return ref count
	int getRefCount(){
		if(m_pRefCount) return *m_pRefCount;
		return 0;
	}
};



/**	Handles for use count.  Handle expects class type for body to
	provide a opearator<, and copy constructor.  It also will delete
	its own memory when no ref. is done to it.  Handles should only be passed by reference.
	This is thread-safe implementation
*/
template <class CLASSTYPE>
class SmartPtr
{
private:
	/**The object being ref counted.*/
	CLASSTYPE *m_pBody;
	/**The ptr holding the reference count to be shared with other handles.*/
	LW::RefCounted *m_pRefCount;


public:

	///compare based on ptrs as used in boost libs - compatibility with franz code
	bool operator==(const SmartPtr<CLASSTYPE> &h) const{
		return (m_pBody == h.m_pBody);
	}
	///compare based on ptrs as used in boost libs - compatibility with franz code
	bool operator!=(const SmartPtr<CLASSTYPE> &h) const{
		return (m_pBody != h.m_pBody);
	}


	SmartPtr(){ 
		m_pBody = 0;
		m_pRefCount = 0;
	}

	SmartPtr(CLASSTYPE *pBody){ 
		m_pBody = pBody;
		if (m_pBody)
			(m_pRefCount = new LW::RefCounted)->incRefCnt();
		else
			m_pRefCount = 0;
	}

	/**handle copy constructor*/
	SmartPtr(const SmartPtr<CLASSTYPE> &h){
		m_pBody = h.m_pBody;
        m_pRefCount = h.m_pRefCount;
		if (m_pRefCount)
			m_pRefCount->incRefCnt();
	}
	bool isNull() const { return (m_pBody == NULL);	}

	/**Assign the object ptr and increment the ref count for this handle*/
	SmartPtr & operator=(const SmartPtr<CLASSTYPE> &h)
	{
		if (m_pRefCount && m_pRefCount->decRefCnt() <= 0) {
			delete m_pBody;
			delete m_pRefCount;
		}

		m_pBody = h.m_pBody;
        m_pRefCount = h.m_pRefCount;
		if (m_pRefCount)
			m_pRefCount->incRefCnt();
		return *this;
	}

	/**Assign the object ptr and increment the ref count for this handle*/
	SmartPtr & operator=(CLASSTYPE *h){

		//cout << "=I:" << h.m_pBody->GetId() << " s:"<< h.m_pCount->Inc() << endl;
		if (m_pRefCount && m_pRefCount->decRefCnt() <= 0) {
			delete m_pBody;
			delete m_pRefCount;
		}

		m_pBody = h;
		if (h) {
			m_pRefCount = new LW::RefCounted;
			m_pRefCount->incRefCnt();
		}
		else
			m_pRefCount = 0;
		return *this;
	}

	/**compares memory locations only*/
	bool PtrEqual(const SmartPtr<CLASSTYPE> &h) const{
		if((m_pBody == h.m_pBody) && (m_pRefCount == h.m_pRefCount)) return true;
		return false;
	}

	~SmartPtr(){
		if (m_pRefCount && m_pRefCount->decRefCnt() <= 0) {
			delete m_pBody;
			delete m_pRefCount;
		}
	}
	/**raw dump of the objects data*/
	friend ostream& operator <<(ostream &os, SmartPtr<CLASSTYPE> &h){
		os << *(h.m_pBody);
		return os;
	}
	///returns the content of the SmartPtr
	CLASSTYPE &operator*() const {return *m_pBody;}

	/**way to access the objects members through the handle*/
	CLASSTYPE *operator->() const {return m_pBody;}

	//this obtains the original object for reference look ups
	//should only be used by alignmentfactory for cloning alignments
	CLASSTYPE& GetBody() const {return (*m_pBody);};

	/// Returns the internal pointer
	CLASSTYPE* getPtr() const {	return m_pBody;	}

	/// Returns the internal pointer - compatible with boost shared ptrs for Franz code
	CLASSTYPE* get() const { return m_pBody; }
	operator bool() const {  return m_pBody != 0; }

	///return ref count
	int getRefCount(){
		if (m_pRefCount) 
			return m_pRefCount->getRefCnt();
		else
			return 0;
	}
};

template <class CLASSTYPE>
class CHandle : public SmartPtr<CLASSTYPE>
{
public:
	CHandle() : SmartPtr<CLASSTYPE>(new CLASSTYPE()) {};
	CHandle(CLASSTYPE *pBody) : SmartPtr<CLASSTYPE>(pBody) {};
};


#endif
