// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _TRIE_H
#define _TRIE_H

#ifdef OLD_MAP
#include <hash_map>
#endif

//#include <unistd.h>
#include <vector>
#include <iostream>
#include <Common/os-dep.h>

#include "BinHashMap.h"

namespace LW {

#define TrieCountIterator TriePayloadIterator

/// Functions to reset the elements in the trie
inline void clear(unsigned int& ref) {
	ref = 0;
}

inline void clear(int& ref) {
	ref = 0;
}

inline void clear(std::string& ref){
	ref.clear();
}


/// This function is required by the tries
struct ProbNode;
inline void clear(ProbNode& ref)
{
	// No need to to anything, the constructor will initialize the node
}

/////////////////////////////////////////////////////////////

/**
* Used to replace Word IDs with real words from the vocabulary for dumping the content of tries
*/
template <class KeyType>
class KeyTranslator
{
public:
	virtual const char* translate(KeyType key) = 0;
};

/////////////////////////////////////////////////////////////

template <class KeyType, class PayloadType>
class TrieNodeChildIterator;

template <class KeyType, class PayloadType>
class TriePayloadIterator;

template <class KeyType, class PayloadType>
class TrieNodeIterator;

template <class KeyType, class PayloadType>
class TrieNodeHorzIterator;

/**
* This object is passed to insertKey in order to alter the counts for every node it finds all the way to the key
*/
template <class PayloadType>
class PayloadModifier 
{
public:
	virtual void modify(PayloadType& payload) const = 0;
};

/**
* An implementation of PayloadModifier that increments counts
*/
template <class PayloadType>
class CountIncrementor : public PayloadModifier<PayloadType>
{
public:
	virtual void modify(PayloadType& payload) const {
		payload++;
	}
};

template <class KeyType, class PayloadType>
class TrieNode
{
	friend class TrieNodeChildIterator<KeyType, PayloadType>;
	friend class TriePayloadIterator<KeyType, PayloadType>;
public:
	typedef TrieNodeIterator<KeyType, PayloadType> Iterator;
	typedef TrieNodeHorzIterator<KeyType, PayloadType> HorzIterator;
public:
#ifdef OLD_MAP
	typedef std::hash_map<KeyType, TrieNode<KeyType, PayloadType> > TrieNodeMap;	
#else
	typedef BinHashMap<KeyType, TrieNode<KeyType, PayloadType> > TrieNodeMap;	
#endif
#ifndef OLD_MAP
protected:
	// This constructor should never be called
	TrieNode(const TrieNode& source) {};
	// This operator should never be called
	TrieNode& operator=(const TrieNode&source) {};
#endif
public:
	TrieNode();
	TrieNode(KeyType key);
	TrieNode(KeyType key, const PayloadType& payload);
	~TrieNode();
public:
	/// This is the poor man's copy operator. 
	/// It transfers all pointers held by the source object to the destination
	void copyFrom(TrieNode& source);
public:
	void clear();
	TrieNode<KeyType, PayloadType>* insertKey(KeyType key);
	TrieNode<KeyType, PayloadType>* insertKey(KeyType const* pKey, unsigned int order);
	TrieNode<KeyType, PayloadType>* insertKey(KeyType const* pKey, unsigned int order, const PayloadType& payload);
	TrieNode<KeyType, PayloadType>* insertKey(KeyType const* pKey, unsigned int order, const PayloadModifier<PayloadType>& payloadModifier);
	void removeKey(KeyType const* pKey, unsigned int order);
	void removeChild(KeyType key);
	TrieNode<KeyType, PayloadType>* findNode(KeyType const* pKey, unsigned int order);
	PayloadType* findPayload(KeyType const* pKey, unsigned int order);
	void dump(std::ostream& stream, const std::string& sPrefix, KeyTranslator<KeyType>& keyTranslator);
	unsigned int getChildCount() const;
	unsigned int getChildCount(unsigned int nOrder) const;
	unsigned int getNodeCount() const;
public: 
	KeyType m_key;
	PayloadType m_payload;
private:
	TrieNodeMap* m_pChildren;
};

////////////////////////////////////////////////////////////////

/**
* Iterates over all children of a specified node.
*/
template <class KeyType, class PayloadType>
class TrieNodeChildIterator
{
private:
	bool m_bDone;
	TrieNode<KeyType, PayloadType>* m_pCurValue;
	TrieNode<KeyType, PayloadType>* m_pOwner;
	typename TrieNode<KeyType, PayloadType>::TrieNodeMap::iterator m_iterator;
public:
	TrieNodeChildIterator(TrieNode<KeyType, PayloadType>* pOwner);
	TrieNode<KeyType, PayloadType>* get();
	TrieNode<KeyType, PayloadType>* next();
	TrieNode<KeyType, PayloadType>* getOwner();
	size_t count() const;
};

//////////////////////////////////////////////////////////////

//class TrieNodeSortedChildIterator
//{
//private:
//	/// Points to the current node in the iterator
//	TrieNode<KeyType, PayloadType>* m_pCurrent;
//	/// Next element after the last valid one
//	TrieNode<KeyType, PayloadType>* m_pLast;
//	/// Array of pointers to sorted nodes
//	TrieNode<KeyType, PayloadType>** m_pSortedNodes;
//public:
//	TrieNodeSortedChildIterator(TrieNode<KeyType, PayloadType>* pOwner);
//	~TrieNodeSortedChildIterator();
//	TrieNode<KeyType, PayloadType>* get();
//	TrieNode<KeyType, PayloadType>* next();
//};
//

//////////////////////////////////////////////////////////////

template <class KeyType, class PayloadType>
class TrieNodeIterator
{
private:
	std::vector<TrieNodeChildIterator<KeyType, PayloadType> > m_nodeStack;
	KeyType* m_pNGramBuffer;
	unsigned int* m_pnKeyLength;
	TrieNode<KeyType, PayloadType>* m_pOwnerNode;
private:
	void pop();
	void push(TrieNode<KeyType, PayloadType>* pNode);
public:
	TrieNodeIterator(TrieNode<KeyType, PayloadType>* pNode, KeyType* pNGramBuffer, unsigned int* pnKeyLength);
	TrieNode<KeyType, PayloadType>* next();
	void reset();
};


template <class KeyType, class PayloadType>
class TrieNodeHorzIterator
{
private:
	std::vector<TrieNodeChildIterator<KeyType, PayloadType> > m_nodeStack;
	unsigned int m_nOrder;
	KeyType* m_pNGramBuffer;
	TrieNode<KeyType, PayloadType>* m_pOwnerNode;
private:
	void pop();
	void push(TrieNode<KeyType, PayloadType>* pNode);
public:
	TrieNodeHorzIterator(TrieNode<KeyType, PayloadType>* pNode, KeyType* pNGramBuffer, unsigned int order);
	TrieNodeHorzIterator(TrieNode<KeyType, PayloadType>* pNode, KeyType* pNGramBuffer, unsigned int order, KeyType* pStartKey, unsigned int nStartKeySize);
	TrieNode<KeyType, PayloadType>* next();
	void reset();
};

////////////////////////////////////////////////////////////

template <class KeyType, class PayloadType>
class TriePayloadIterator
{
private:
	TrieNodeHorzIterator<KeyType, PayloadType> m_nodeIterator;
public:
	TriePayloadIterator(TrieNode<KeyType, PayloadType>* pNode, KeyType* pNGramBuffer, unsigned int order)
		: m_nodeIterator(pNode, pNGramBuffer, order) {};

	TriePayloadIterator(TrieNode<KeyType, PayloadType>* pNode, KeyType* pNGramBuffer, unsigned int order, KeyType* pStartKey, unsigned int nStartKeySize)
		: m_nodeIterator(pNode, pNGramBuffer, order, pStartKey, nStartKeySize) {};

	PayloadType* next() {
		TrieNode<KeyType, PayloadType>* pNode = m_nodeIterator.next();
		if (pNode) {
			return &(pNode->m_payload);
		}
		else {
			return NULL;
		}
	}
	void reset() {
		m_nodeIterator.reset();
	};
};

/**
SortedClassIterator must implement 2 methods: 
1. SortedClass* next() that returns NULL at the end of the iteration.
2. size_t count() that returns the number of elements.
TrieNodeChildIterator does just that.
*/

#ifndef _WIN32
#  define __cdecl
#endif
	
// This function will be used as a template for sorting
typedef int (__cdecl *SorterFunction )(const void *, const void *);

template <class SortedClass, class SortedClassIterator, SorterFunction sorterFunction>
class Sorter 
{
public:
	typedef SortedClass* SortedClassPtr;
public:
	Sorter(SortedClassIterator& it, bool bAutoSort = true) {
		// Allocate array of pointers
		m_pFirst = new SortedClassPtr[it.count()];
		SortedClass** pSortedElement = m_pFirst;

		// Initialize member variables
		m_nCount = it.count();
		// TODO: This value is invalid before next() is called
		// Maybe we want to do something to protect a silly caller
		// if we add a method to get access directly to m_pCurrent
		m_pCurrent = m_pFirst - 1;
		m_pLast = m_pFirst + (m_nCount - 1);

		// Extract all elements and copy them in an array for sorting
		SortedClass* pSortedClass;

		// Check the count so we don't crash, we rather truncate the arary
		size_t nCount = 0;
		while ((pSortedClass = it.next()) && (nCount <= m_nCount)) {
			*pSortedElement = pSortedClass;

			pSortedElement++;
			nCount++;
		}

		// We are ready for quicksort here
		// if bAutoSort is false, the caller will have to call sort() function before using this class for any purpose
		if (bAutoSort) {
			sort();
		}
	}

	~Sorter() {
		delete[] m_pFirst;
	}

	void sort() {
		// Ready for quicksort
		qsort(m_pFirst, m_nCount, sizeof(SortedClass*), sorterFunction);
	}

	SortedClass* next() {
		if (m_pCurrent < m_pLast) {
			m_pCurrent++;
			return *m_pCurrent;
		}
		else {
			return NULL;
		}
	}

	size_t count() const {
		return m_nCount;
	}
private:
	/// The first element in the array
	SortedClass** m_pFirst;
	/// Last valid element in the array
	SortedClass** m_pLast;
	/// Current element
	SortedClass** m_pCurrent;
	/// Total number of elements. Redundant, but handy
	size_t m_nCount;
};

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>::TrieNode()
{
	LW::clear(m_key);
	LW::clear(m_payload);
	m_pChildren = NULL;
}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>::TrieNode(KeyType key)
{
	m_key = key;
	LW::clear(m_payload);
	m_pChildren = NULL;
}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>::TrieNode(KeyType key, const PayloadType& payload)
{
	m_key = key;
	m_payload = payload;
	m_pChildren = NULL;
}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>::~TrieNode()
{
	//if (m_pChildren) {
	//	TrieNodeMap::iterator it;

	//	// Delete all child nodes
	//	for (it = m_map.begin(); it != m_map.end(); it++) {
	//		TrieNode<KeyType, PayloadType>* pChild = it->second.m_pChild;
	//		// Don't test. Deleting a NULL pointer is benign
	//		delete pChild;
	//	}

	//}

	this->clear();
}

template <class KeyType, class PayloadType>
void TrieNode<KeyType, PayloadType>::clear()
{
	if (m_pChildren) {
		delete m_pChildren;
		m_pChildren = NULL;
	}
}

template <class KeyType, class PayloadType>
void TrieNode<KeyType, PayloadType>::copyFrom(TrieNode<KeyType, PayloadType>& source)
{
	// Cleanup the destination (this)
	this->clear();

	// Transfer all data, including pointers
	m_key = source.m_key;
	m_payload = source.m_payload;
	m_pChildren = source.m_pChildren;

	// The pointer ownership has been transferred
	source.m_pChildren = NULL;
}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNode<KeyType, PayloadType>::insertKey(KeyType key) 
{
	typename TrieNodeMap::iterator it;
        
//        std::cerr << "TrieNode::insertKey(key): " << key << std::endl;

	// Do we have a map?
	if (!m_pChildren) {
		m_pChildren = new TrieNodeMap();
		it = m_pChildren->end();
	}
	else {
		it = m_pChildren->find(key);
	}


	if (it == m_pChildren->end()) {
		// Node not there yet. Insert it
#ifdef OLD_MAP
		TrieNode<KeyType, PayloadType> node(key);
		it = m_pChildren->insert(TrieNodeMap::value_type(key, node)).first;
#else
		TrieNode<KeyType, PayloadType>* pNode = &(m_pChildren->insert2(key)->second);
		// Important: set the key. insert() will just use the default constructor
//                std::cerr << "num m_pChildren: " << m_pChildren->size() << std::endl;
		pNode->m_key = key;
                //execve("~/bin/psLangModel.sh",NULL,NULL);
		return pNode;
#endif
	}

	return &((*it).second);
}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNode<KeyType, PayloadType>::insertKey(KeyType const* pKey, unsigned int order) 
{
	// Time to stop?
	if (0 == order) {
		return this;
	}

	// At this point order > 0
	// Create a child node if necessary. This version of insertKey only inserts one child, does not go deeper
	TrieNode<KeyType, PayloadType>* pChild = insertKey(pKey[0]);
	// Delegate insertion
	return pChild->insertKey(pKey + 1, order - 1);
}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNode<KeyType, PayloadType>::insertKey(KeyType const* pKey, unsigned int order, const PayloadType& payload) 
{
	TrieNode<KeyType, PayloadType>* pNode = insertKey(pKey, order);
	if (pNode) {
		pNode->m_payload = payload;
	}
	return pNode;
}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNode<KeyType, PayloadType>::insertKey(KeyType const* pKey, unsigned int order, const PayloadModifier<PayloadType>& payloadModifier) 
{
	payloadModifier.modify(m_payload);

	// Time to stop?
	if (0 == order) {
		return this;
	}

	// At this point order > 0
	// Create a child node if necessary
	TrieNode<KeyType, PayloadType>* pChild = insertKey(pKey[0]);
	// Delegate insertion
	return pChild->insertKey(pKey + 1, order - 1, payloadModifier);
}

template <class KeyType, class PayloadType>
void TrieNode<KeyType, PayloadType>::removeKey(KeyType const* pKey, unsigned int order)
{
	if (order >= 1) {
		// Find the parent of the key
		TrieNode<KeyType, PayloadType>* pParent = findNode(pKey, order - 1);
		if (pParent) {
			pParent->removeChild(pKey[order - 1]);
		}
	}
}

template <class KeyType, class PayloadType>
void TrieNode<KeyType, PayloadType>::removeChild(KeyType key) 
{
	if (m_pChildren) {
		m_pChildren->erase(key);
	}
}

//template <class KeyType, class PayloadType>
//TrieNode<KeyType, PayloadType>* TrieNode<KeyType, PayloadType>::findNode(KeyType const* pKey, unsigned int nOrder)
//{
//	if (0 == nOrder) {
//		return this;
//	}
//
//	// Not done yet
//	if (NULL == m_pChildren) {
//		// Not found
//		return NULL;
//	}
//
//	typename TrieNodeMap::iterator it = m_pChildren->find(pKey[0]);
//	if (it == m_pChildren->end()) {
//		// Child not found
//		return NULL;
//	}
//
//	// Delegate to the child
//	TrieNode<KeyType, PayloadType>& node = (*it).second;
//	return node.findNode(pKey + 1, nOrder - 1);
//}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNode<KeyType, PayloadType>::findNode(KeyType const* pKey, unsigned int nOrder)
{
	if (0 == nOrder) {
		return this;
	}

	// Not done yet
	if (NULL == m_pChildren) {
		// Not found
		return NULL;
	}

	typename TrieNodeMap::Element* pElement = m_pChildren->findElement(pKey[0]);
	if (NULL == pElement) {
		// Child not found
		return NULL;
	}
	TrieNode<KeyType, PayloadType>* pNode = &(pElement->second);

	// Delegate to the child
	return pNode->findNode(pKey + 1, nOrder - 1);
}

template <class KeyType, class PayloadType>
PayloadType* TrieNode<KeyType, PayloadType>::findPayload(KeyType const* pKey, unsigned int order)
{
	TrieNode<KeyType, PayloadType>* pNode = findNode(pKey, order);
	if (pNode) {
		return &(pNode->m_payload);
	}
	else {
		return NULL;
	}
}

template <class KeyType, class PayloadType>
void TrieNode<KeyType, PayloadType>::dump(std::ostream& stream, const std::string& sPrefix, KeyTranslator<KeyType>& keyTranslator)
{
	// Dump the content of this entry first
	std::string sNewPrefix = sPrefix + " " + keyTranslator.translate(m_key);
    	stream << sNewPrefix << " " << m_payload << std::endl;

	// Dump all children
	TrieNodeChildIterator<KeyType, PayloadType> it(this);
	TrieNode<KeyType, PayloadType>* pChild;
	while (pChild = it.next()) {
		pChild->dump(stream, sNewPrefix, keyTranslator);
	}
}

template <class KeyType, class PayloadType>
unsigned int TrieNode<KeyType, PayloadType>::getChildCount() const
{
//	unsigned int nCount = 0;

	if (NULL == m_pChildren) {
		return 0;
	}	
	else {
		return static_cast<int> (m_pChildren->size());
	}
	
	//// Count children
	//TrieNodeMap::iterator it = m_pChildren.begin();
	//while (it != m_pChildren.end()){
	//	TrieNode<KeyType, PayloadType>& child = *it;
	//	nCount++;
	//	nCount += child.getChildCount();
	//}

	//return nCount;
}

template <class KeyType, class PayloadType>
TrieNodeChildIterator<KeyType, PayloadType>::TrieNodeChildIterator(TrieNode<KeyType, PayloadType>* pOwner) {
	//assert(pOwner);

	m_pCurValue = NULL;
	m_pOwner = pOwner;

	if (pOwner->m_pChildren) {
		m_iterator = m_pOwner->m_pChildren->begin();
		if (m_iterator == m_pOwner->m_pChildren->end()) {
			m_bDone = true;
		}
		else {
			m_bDone = false;
		}
	}
	else {
		m_bDone = true;
	}
}

template<class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNodeChildIterator<KeyType, PayloadType>::get() {
	return m_pCurValue;
}

template<class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNodeChildIterator<KeyType, PayloadType>::next() {
	if (m_bDone) {
		m_pCurValue = NULL;
	}
	else {
		if (m_iterator == m_pOwner->m_pChildren->end()) {
			m_bDone = true;
			m_pCurValue = NULL;
		}
		else {
			m_pCurValue = &((*m_iterator).second);
			++m_iterator;
		}
	};

	return m_pCurValue;
}

template<class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNodeChildIterator<KeyType, PayloadType>::getOwner() {
	return m_pOwner;
}

template<class KeyType, class PayloadType>
size_t TrieNodeChildIterator<KeyType, PayloadType>::count() const
{
	return m_pOwner->getChildCount();
}

//template<class KeyType, class PayloadType>
//TrieNodeSortedChildIterator<KeyType, PayloadType>::TrieNodeSortedChildIterator(TrieNode<KeyType, PayloadType>* pOwner)
//{
//
//}
//
//template<class KeyType, class PayloadType>
//TrieNode<KeyType, PayloadType>* TrieNodeSortedChildIterator<KeyType, PayloadType>::get() 
//{
//	return m_pCurrent;
//}
//
//template<class KeyType, class PayloadType>
//TrieNode<KeyType, PayloadType>* TrieNodeSortedChildIterator<KeyType, PayloadType>::next()
//{
//	m_pCurrent++;
//	if (m_pCurrent >= m_pLast) {
//		return NULL;
//	}
//	else {
//		return m_pCurrent;
//	}
//}

template <class KeyType, class PayloadType>
unsigned int TrieNode<KeyType, PayloadType>::getChildCount(unsigned int nOrder) const
{
	unsigned int nCount = 0;
	// Count children
	if (m_pChildren) {
		typename TrieNodeMap::iterator it = m_pChildren->begin();
		while (it != m_pChildren->end()){
			TrieNode<KeyType, PayloadType>& child = (*it).second;
			if (1 == nOrder) {
				nCount++;
			}
			else {
				nCount += child.getChildCount(nOrder - 1);
			}
			++it;
		}
	}

	return nCount;
}

template <class KeyType, class PayloadType>
unsigned int TrieNode<KeyType, PayloadType>::getNodeCount() const
{
	unsigned int nCount = 1;
	// Count children
	if (m_pChildren) {
		typename TrieNodeMap::iterator it = m_pChildren->begin();
		while (it != m_pChildren->end()){
			TrieNode<KeyType, PayloadType>& child = (*it).second;
			nCount += child.getNodeCount();
			it++;
		}
	}

	return nCount;
}


template <class KeyType, class PayloadType>
TrieNodeHorzIterator<KeyType, PayloadType>::TrieNodeHorzIterator(TrieNode<KeyType, PayloadType>* pNode, KeyType* pNGramBuffer, unsigned int order)
{
	//assert(pNode);
	//assert(pNGramBuffer);

	m_nOrder = order;
	m_pNGramBuffer = pNGramBuffer;
	m_pOwnerNode = pNode;
	
	reset();
}

template <class KeyType, class PayloadType>
TrieNodeHorzIterator<KeyType, PayloadType>::TrieNodeHorzIterator(TrieNode<KeyType, PayloadType>* pNode, KeyType* pNGramBuffer, unsigned int order,  KeyType* pStartKey, unsigned int nStartKeySize)
{
	//assert(pNGramBuffer);

	m_nOrder = order;
	m_pNGramBuffer = pNGramBuffer;
	m_pOwnerNode = pNode->findNode(pStartKey, nStartKeySize);

	reset();
}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNodeHorzIterator<KeyType, PayloadType>::next()
{
	// The iterator already points to the next item to be returned by next()
	if (m_nodeStack.size() == 0) {
		// No more nodes. Done iterating the node.
		return NULL;
	}

	if (0 == m_nOrder) {
		//assert(1 == m_nodeStack.size());
		TrieNode<KeyType, PayloadType>* pNode= m_nodeStack[0].getOwner();
		// next will return NULL next time if the stack is empty
		m_nodeStack.clear();
		return pNode;
	}

	// If we are deeper than we need, we really have a logic problem
	//assert(m_nodeStack.size() <= m_nOrder);

	// Get the iterator on the top of the stack and see if it has reached the end
	TrieNodeChildIterator<KeyType, PayloadType>& it = *(m_nodeStack.rbegin());
	if (!it.get()) {
		// Reached the end of the current node. Try the parent
		pop();
		return next();
	}
	// We have a valid node. Is it deep enough?
	else if (m_nodeStack.size() < m_nOrder) {
		// Not deep enough. Push the child on the stack to iterate it
		push(it.get());
		return next();
	}
	else {
		// The iterator points to a node that is deep enough
		TrieNode<KeyType, PayloadType>* pNode = it.get();

		// Copy the key into the buffer supplied by the client
		m_pNGramBuffer[m_nOrder - 1] = pNode->m_key;
		// The rest of the keys have already been copied by push()

		// Always move to the next node to be prepared for the next call to next()
		it.next();
		return pNode;
	}

	// This line should never be reached
	//assert(false);
	return NULL;
}

template <class KeyType, class PayloadType>
void TrieNodeHorzIterator<KeyType, PayloadType>::reset()
{
	m_nodeStack.clear();

	// If the owner node is NULL (the iterator was initialized for a key that does not exist)
	// don't put anything on the stack and next() will return NULL
	if (m_pOwnerNode) {
		push(m_pOwnerNode);
	}
}

template <class KeyType, class PayloadType>
void TrieNodeHorzIterator<KeyType, PayloadType>::pop()
{
	m_nodeStack.pop_back();
	if (m_nodeStack.size() > 0) {
		// Get the last element on the stack
		TrieNodeChildIterator<KeyType, PayloadType>& it = *(m_nodeStack.rbegin());

		// Try to advance the iterator
		if (!it.next()) {
			// End of the iterator reached. Pop this node too.
			pop();
		}
	}
}

template <class KeyType, class PayloadType>
void TrieNodeHorzIterator<KeyType, PayloadType>::push(TrieNode<KeyType, PayloadType>* pNode)
{
	TrieNodeChildIterator<KeyType, PayloadType> it(pNode);
	// Go to the first item
	it.next();
	m_nodeStack.push_back(it);

	size_t nStackSize = m_nodeStack.size();
	// Only copy if the stack size >= 2 (we don't want the root node key there)
	if (nStackSize > 1) {
		m_pNGramBuffer[nStackSize - 2] = pNode->m_key;
	}
}

//================================

template <class KeyType, class PayloadType>
TrieNodeIterator<KeyType, PayloadType>::TrieNodeIterator(TrieNode<KeyType, PayloadType>* pNode, KeyType* pNGramBuffer, unsigned int* pnKeyLength)
{
	//assert(pNode);
	//assert(pNGramBuffer);
	//assert(pnKeyLength);

	m_pNGramBuffer = pNGramBuffer;
	m_pnKeyLength = pnKeyLength;
	m_pOwnerNode = pNode;
	
	reset();
}

template <class KeyType, class PayloadType>
TrieNode<KeyType, PayloadType>* TrieNodeIterator<KeyType, PayloadType>::next()
{
	// The iterator already points to the next item to be returned by next()
	if (m_nodeStack.size() == 0) {
		// No more nodes. Done iterating the node.
		m_pnKeyLength = 0;
		return NULL;
	}

	// Get the iterator on the top of the stack and see if it has reached the end
	TrieNodeChildIterator<KeyType, PayloadType>& it = *(m_nodeStack.rbegin());

	TrieNode<KeyType, PayloadType>* pNode = it.get();
	if (!it.get()) {
		// Reached the end of the current node. Try the parent
		pop();
		return next();
	}
	// We have a valid node
	// Copy the key in the supplied buffer
	*m_pnKeyLength = m_nodeStack.size();
	m_pNGramBuffer[*m_pnKeyLength - 1] = pNode->m_key;
	// Load the children on the stack and return the node
	// Next call to next() will return the first child (if any)
	push(pNode);
	return pNode;
}

template <class KeyType, class PayloadType>
void TrieNodeIterator<KeyType, PayloadType>::reset()
{
	m_nodeStack.clear();

	// If the owner node is NULL (the iterator was initialized for a key that does not exist)
	// don't put anything on the stack and next() will return NULL
	if (m_pOwnerNode) {
		push(m_pOwnerNode);
	}
}

template <class KeyType, class PayloadType>
void TrieNodeIterator<KeyType, PayloadType>::pop()
{
	m_nodeStack.pop_back();
	if (m_nodeStack.size() > 0) {
		// Get the last element on the stack
		TrieNodeChildIterator<KeyType, PayloadType>& it = *(m_nodeStack.rbegin());

		// Try to advance the iterator
		if (!it.next()) {
			// End of the iterator reached. Pop this node too.
			pop();
		}
	}
}

template <class KeyType, class PayloadType>
void TrieNodeIterator<KeyType, PayloadType>::push(TrieNode<KeyType, PayloadType>* pNode)
{
	TrieNodeChildIterator<KeyType, PayloadType> it(pNode);
	// Go to the first item
	it.next();
	m_nodeStack.push_back(it);

	//// This span of code maintains the key in the output buffer
	//// Normally the key would be copied when the parent is returned (as the parent is returned before the children)
	//// We copy it again, just in case we change the logic later on
	//size_t nStackSize = m_nodeStack.size();
	//// Only copy if the stack size >= 2 (we don't want the root node key there)
	//if (nStackSize > 1) {
	//	m_pNGramBuffer[nStackSize - 2] = pNode->m_key;
	//}
}

} // namespace LW

#endif // _TRIE_H
