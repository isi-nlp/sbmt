// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _BIN_HASH_MAP_H
#define _BIN_HASH_MAP_H

#include <Common/os-dep.h>
#include <string>
#include <utility>
#include <cstdlib>

namespace LW {

// This constant should ALWAYS BE 1 or greater
#define MAX_FILL_FACTOR 1.25
// This constant must be a power of 2
#define SEQUENTIAL_SEARCH_THRESHOLD 8

// We arbitrarily choose values that define empty keys
inline bool isEmptyKey(unsigned int nKey)		{return nKey == (unsigned int) 0xFFFFFFFF;};
inline void setEmptyKey(unsigned int& nKey)		{nKey = (unsigned int) 0xFFFFFFFF;};


//inline void getEmptyKey(int& nKey)				{nKey = 0x80000000;};
//inline void getEmptyKey(unsigned short& nKey)	{nKey = (unsigned short) 0xFFFF;};
//inline void getEmptyKey(short& nKey)			{nKey = (short) 0x8000;};

inline bool isEmptyKey(const std::string& sKey) {return (sKey.size() == 0);};
inline void setEmptyKey(std::string& sKey) {sKey.clear();};


inline unsigned int getHashMask(unsigned int nMaxBits)
{
	return ~((~0)<<(nMaxBits));
}

// Hash functions
inline unsigned int getBinHash(unsigned int nKey, unsigned int nMaxBits) 
{
//    return (((nKey * 1103515245 + 12345) >> (30-nMaxBits)) & getHashMask(nMaxBits));
	return (nKey ^ (nKey << 1)) & getHashMask(nMaxBits);
}

inline unsigned int getBinHash(const char* szKey, unsigned int nMaxBits)
{
    unsigned hash;

    for (hash = 0; *szKey; szKey++) {
		hash += (hash << 3) + *szKey;
    }
    return getBinHash(hash, nMaxBits);
}

inline unsigned int getBinHash(const std::string& sKey, unsigned int nMaxBits)
{
	return getBinHash(sKey.c_str(), nMaxBits);
}

template <class Key, class Type>
class BinHashMapIterator;

template <class Key, class Type>
struct BinHashMapElement {
	Key first;
	Type second;
	BinHashMapElement() {
		// This should never be called
		//assert(false);

		// Initialize the key with empty key. 
		// This way an array is initialized by default upon creation
		getEmptyKey(first);
	}
	BinHashMapElement& operator=(BinHashMapElement& source) {
		// This should never be called
		//assert(false);
		first = source.first;
		second = second;
		return *this;
	}
};

template <class Key, class Type>
class BinHashMap 
{
public:
	typedef unsigned int Hash;
	typedef BinHashMapIterator<Key, Type> Iterator;
	typedef Iterator iterator; // For compatibility with std::map
	typedef Iterator const_iterator; // TODO: Make this really const
	typedef BinHashMapElement<Key, Type> Element;
public:
public:
	BinHashMap();
	BinHashMap(Hash nInitialSize);
	virtual ~BinHashMap();
public:
	/// Inserts a key in the map. The Type must have a default constructor.
	std::pair<iterator, bool> insert(const Key& key);
	/// Finds a key in the map
	iterator find(const Key& key) const;
	/// Erases an element with a given key
	void erase(Key& key);
	/// Returns the size of the map
	size_t size() const;
public: // These methods used to be proected, but I made them public as they are faster
	/// Fast internal insert. It does not create an iterator, just returns a pointer
	Element* insert2(const Key& key);
	/// Returns an element having a given key, NULL if not found
	Element* findElement(const Key& key) const;
protected:
	/// Searches the map for a given key. If the key is found returns true and nIndex is set accordingly
	bool findIndex(Key key, Hash& nIndex) const;
	/// Calculates a recommended hash size for a given element count
	inline Hash getRecommendedSize(Hash nElementCount) const;
	/// Returns the last valid element in the map. In a regular map this would be the element before end()
	inline Element* getEndElement() const;
	/// Returns the first non empty key in the map. Used to set up an interator.
	Element* getFirstBusyLocation() const;
	/// "Covers" the element at the given location by shifting elements to the left on top of it
	inline void shiftLeft(Hash nLocation);
public:
	/// Begin iterator. Identical with std::map
	inline iterator begin() const;
	/// End iterator. Identical with std::map
	inline iterator end() const;
private:
	/// Changes the allocated size of the map, copies elements over and reindexes the map if necessary
	void reallocate(Hash nNewSize);
	/// Given a size, it will round it to the next power of 2. Also computes the number of bits
	/// necessary to mask the hash for this size
	void roundSize(Hash& nSize, unsigned int& nMaxBits);
	/// Is the search sequential
	inline bool sequentialSearch() const {return m_nDataSize <= SEQUENTIAL_SEARCH_THRESHOLD;};
private:
	/// Holds elements in the map. 
	Element* m_pData;
	/// The number of Element allocated in m_pData
	Hash m_nDataSize;
	/// The number of valid keys in the map
	Hash m_nElementCount;
	/// Cashes the number of bits used for the hash mask. This is redundant for speed.
	/// The information can be computed from m_nDataSize
	unsigned int m_nMaxBits;
public:
	///// Debug varible to test the number of collisions in the map
	//unsigned int m_nCollisionCount;
};

template <class Key, class Type>
class BinHashMapIterator {
	friend class BinHashMap<Key, Type>;
public:
	typedef BinHashMapElement<Key, Type> Element;
	protected:
		/// Make sure it cannot be created by anybody outside
		BinHashMapIterator(Element* pElement, Element* pEnd);
	public:
		BinHashMapIterator();
		BinHashMapIterator(const BinHashMapIterator& source);
	public:
		BinHashMapIterator& operator++();
		BinHashMapIterator& operator=(const BinHashMapIterator& source);
		bool operator==(const BinHashMapIterator& source) const;
		bool operator!=(const BinHashMapIterator& source) const;
		Element& operator->();
		Element& operator*();
	protected:
		Element* m_pElement;
		Element* m_pEnd;
	};


template <class Key, class Type>
BinHashMapIterator<Key, Type>::BinHashMapIterator()
{
	m_pElement = NULL;
	m_pEnd = NULL;
}

template <class Key, class Type>
BinHashMapIterator<Key, Type>::BinHashMapIterator(Element* pElement, Element* pEnd)
{
	m_pElement = pElement;
	m_pEnd = pEnd;
}

template <class Key, class Type>
BinHashMapIterator<Key, Type>::BinHashMapIterator(const BinHashMapIterator<Key, Type>& source)
{
	operator=(source);
}

template <class Key, class Type>
BinHashMapIterator<Key, Type>&  BinHashMapIterator<Key, Type>::operator++()
{
	//Key emptyKey;
	//getEmptyKey(emptyKey);

	if (m_pElement >= m_pEnd) {
		// Somebody keeps incrementing without comparing with end()
		// Just ignore the request
		return *this;
	}

	while (m_pElement < m_pEnd) {
		m_pElement++;

		if ((m_pElement < m_pEnd) && (!isEmptyKey(m_pElement->first))) {
			return *this;
		}
	}

	return *this;
}

template <class Key, class Type>
BinHashMapIterator<Key, Type>& BinHashMapIterator<Key, Type>::operator=(const BinHashMapIterator<Key, Type>& source)
{
	m_pElement = source.m_pElement;
	m_pEnd = source.m_pEnd;

	return *this;
}

template <class Key, class Type>
bool BinHashMapIterator<Key, Type>::operator==(const BinHashMapIterator<Key, Type>& source) const
{
	return (m_pElement == source.m_pElement);
}

template <class Key, class Type>
bool BinHashMapIterator<Key, Type>::operator!=(const BinHashMapIterator<Key, Type>& source) const
{
	return (m_pElement != source.m_pElement);
}


template <class Key, class Type>
typename BinHashMapIterator<Key, Type>::Element& BinHashMapIterator<Key, Type>::operator->() 
{
	//assert(m_pElement);
	return *m_pElement;
}

template <class Key, class Type>
typename BinHashMapIterator<Key, Type>::Element& BinHashMapIterator<Key, Type>::operator*() 
{
	//assert(m_pElement);
	return *m_pElement;
}


/////////////////////////////////////

template <class Key, class Type>
BinHashMap<Key, Type>::BinHashMap()
{
	m_pData = NULL;
	m_nDataSize = 0;
	m_nElementCount = 0;
	m_nMaxBits = 0;
}

template <class Key, class Type>
BinHashMap<Key, Type>::BinHashMap(unsigned int nInitialSize) 
{
	m_pData = NULL;
	m_nDataSize = 0;
	m_nElementCount = 0;
	m_nMaxBits = 0;

	reallocate(nInitialSize);
}

template <class Key, class Type>
BinHashMap<Key, Type>::~BinHashMap()
{
	if (m_pData) {
		//Key emptyKey;
		//getEmptyKey(emptyKey);

		// Call destructors if the keys are not empty
		for(Hash i = 0; i < m_nDataSize; i++) {
			if (!isEmptyKey(m_pData[i].first)) {
				m_pData[i].second.~Type();
			}
		}

		free(m_pData);
	}
}

template <class Key, class Type>
std::pair<typename BinHashMap<Key, Type>::iterator, bool> BinHashMap<Key, Type>::insert(const Key& key)
{
	Element* pElement = insert2(key);
	//assert(pElement);

	// TODO: Put meaningful data into the bool variable
	return std::make_pair<iterator, bool>(iterator(pElement, getEndElement()), false);
}

template <class Key, class Type>
typename BinHashMap<Key, Type>::iterator BinHashMap<Key, Type>::find(const Key& key) const
{
	Element* pElement = findElement(key);
	Element* pEnd = getEndElement();
	if (pElement) {
		return iterator(pElement, pEnd);
	}
	else {
		// Not found. This iterator is already placed at the end
		return iterator(pEnd, pEnd);
	}
}

template <class Key, class Type>
void BinHashMap<Key, Type>::erase(Key& key)
{
	if (m_nElementCount >= 1) {
		// Find element to delete
		Hash nIndex;
		if (findIndex(key, nIndex)) {
			// Call the destructor explicitly
			m_pData[nIndex].second.~Type();

			// Shift elements to the left on top if it, if necessary
			// The key will be nuked
			shiftLeft(nIndex);
			m_nElementCount--;
		}
	}
}

template <class Key, class Type>
size_t BinHashMap<Key, Type>::size() const
{
	return m_nElementCount;
}

//template <class Key, class Type>
//BinHashMap<Key, Type>::Iterator& BinHashMap<Key, Type>::insert(const Key& key, const Type& type)
//{
//}

template <class Key, class Type>
typename BinHashMap<Key, Type>::Element* BinHashMap<Key, Type>::insert2(const Key& key)
{
	// Do we have enough room?
	Hash dRecommendedSize = getRecommendedSize(m_nElementCount + 1);
	if (m_nDataSize < dRecommendedSize) {
		// Reallocate elements. The size will be automatically rounded to a power of 2
		reallocate(dRecommendedSize);
	}

	// Find a free location
	unsigned int nLocation;
	// Only do something if the key is not found. 
	// If found, just ignore the request to insert it
	if (!findIndex(key, nLocation)) {
		// Copy the key
		m_pData[nLocation].first = key;

		// Zero out the element
		memset(&(m_pData[nLocation].second), 0, sizeof(Type));
		// Call the constructor in place
		new (&(m_pData[nLocation].second)) Type;

		m_nElementCount++;
	}

	return &(m_pData[nLocation]);
}

template <class Key, class Type>
bool BinHashMap<Key, Type>::findIndex(Key key, Hash& nIndex) const
{
	if (sequentialSearch()) {
		// No gaps, all elements are stored at the beginning of the array
		for (unsigned int i = 0; i < m_nDataSize; i++) {
			if (isEmptyKey(m_pData[i].first)) {
				nIndex = i;
				return false;
			}
			if (key == m_pData[i].first) {
				nIndex = i;
				return true;
			}
		}
		// Not found
		return false;
	}
	else {
		//Key emptyKey;
		//getEmptyKey(emptyKey);

		// Indexed search
		// Compute hash
		Hash hash = getBinHash(key, m_nMaxBits);
		Hash hashMask = getHashMask(m_nMaxBits);
		//assert(hash < m_nDataSize);

		// Watch out for an infinite loop if we have a bug
		// The hash should never be full! The search should end with either an empty key, or the key is found
		for (Hash i = hash; ; i = (i + 1) & hashMask) {
			if (isEmptyKey(m_pData[i].first)) {
				// Just found an empty key
				nIndex = i;
				return false;
			}
			if (key == m_pData[i].first) {
				// Found the key
				nIndex = i;
				return true;
			}
		}
	}
}

template <class Key, class Type>
typename BinHashMap<Key, Type>::Element* BinHashMap<Key, Type>::findElement(const Key& key) const
{
	Hash nIndex;
	if (findIndex(key, nIndex)) {
		return m_pData + nIndex;
	}
	else {
		return NULL;
	}
}

template <class Key, class Type>
typename BinHashMap<Key, Type>::Hash BinHashMap<Key, Type>::getRecommendedSize(unsigned int nCurrentSize) const
{
	if (nCurrentSize <= SEQUENTIAL_SEARCH_THRESHOLD) {
		// If the search is sequenctial we can fill all the Elements, no gaps
		return nCurrentSize;
	}
	else {
		// If the search is indexed, we fill only a percentage of elements
		return (unsigned int) (MAX_FILL_FACTOR * nCurrentSize);
	}
}


template <class Key, class Type>
typename BinHashMap<Key, Type>::Element* BinHashMap<Key, Type>::getEndElement() const
{
	if (m_pData) {
		return m_pData + m_nDataSize;
	}
	else {
		return NULL;
	}
}

template <class Key, class Type>
typename BinHashMap<Key, Type>::Element* BinHashMap<Key, Type>::getFirstBusyLocation() const
{
	if (NULL == m_pData) {
		return NULL;
	}

	//// We need to compare with an empty key
	//Key emptyKey;
	//getEmptyKey(emptyKey);

	// Scan keys
	Element* pElement;
	for (pElement = m_pData; (pElement - m_pData) < (int) m_nDataSize; pElement++) {
		if (!isEmptyKey(pElement->first)) {
			return pElement;
		}
	}

	// This should only happen if we deleted all keys without shrinking the hash back to 0
	return NULL;
}


template <class Key, class Type>
void BinHashMap<Key, Type>::shiftLeft(Hash nLocation)
{
	//assert(nLocation < m_nDataSize);

	//// Get the empty key
	//Hash emptyKey;
	//getEmptyKey(emptyKey);

	Hash hashMask = getHashMask(m_nMaxBits);

	// Nuke the key
	setEmptyKey(m_pData[nLocation].first);

	if (sequentialSearch()) {
		// Sequential search
		// As we don't have a lot of elements, don't bother with loops
		// just shift all elements in bulk as we know they are all in the beginning
		memcpy(m_pData + nLocation, m_pData + nLocation + 1, (m_nDataSize - nLocation - 1) * sizeof(Element));
		// Nuke the last element
		setEmptyKey(m_pData[m_nDataSize - 1].first);
	}
	else {
		// Indexed search. This is a little more complicated
		// Scan keys between the key we delete and first empty location
		for (Hash i = (nLocation + 1) & hashMask; !isEmptyKey(m_pData[i].first); i = (i + 1) & hashMask) {
			Hash nDestIndex;

			// Attempt to find the key in the hash
			// If the key is found, we inserted no gaps by nuking the key, just continue
			if (!findIndex(m_pData[i].first, nDestIndex)) {
				// If the key is not found we screwed up the hash by nuking the key
				// Move the key to the location returned by findIndex
				memcpy(&(m_pData[nDestIndex]), &(m_pData[i]), sizeof(Element));
				// Nuke the key
				setEmptyKey(m_pData[i].first);
			}
		}
	}
}


template <class Key, class Type>
typename BinHashMap<Key, Type>::Iterator BinHashMap<Key, Type>:: begin() const
{
	Element* pFirst = getFirstBusyLocation();
	Element* pEnd = getEndElement();
	if (pFirst) {
        return iterator(pFirst, pEnd);
	}
	else {
		// No elements. Just place the iterator at the end
		return iterator(pEnd, pEnd);
	}
}

template <class Key, class Type>
typename BinHashMap<Key, Type>::iterator BinHashMap<Key, Type>::end() const
{
	Element* pEnd = getEndElement();
	return iterator(pEnd, pEnd);
}

template <class Key, class Type>
void BinHashMap<Key, Type>::reallocate(unsigned int nNewSize) 
{
	// Make the size a power of 2
	unsigned int nNewMaxBits;
	roundSize(nNewSize, nNewMaxBits);

	if (m_nDataSize == nNewSize) {
		// Nothing to do. The size is the same.
		return;
	}

	// Make sure the new size can hold the number of elements present (in the case we want to shrink)
	if (m_nElementCount > nNewSize) {
		//assert(false);
		// Requested size too small. Just ignore the request for a release build.
		return;
	}

	// Reallocate the array
	Element* pOldData = m_pData;
	Hash nOldDataSize = m_nDataSize;

	m_pData = (Element*) malloc(sizeof(Element) * nNewSize);
	m_nDataSize = nNewSize;
	m_nMaxBits = nNewMaxBits;

	//// We have to compare with this key to see if the element is empty
	//Key emptyKey;
	//getEmptyKey(emptyKey);

	// Make sure all keys are empty
	for (Hash i = 0; i < m_nDataSize; i++) {
		setEmptyKey(m_pData[i].first);
	}

	// Rehash and copy keys
	if (sequentialSearch()) {
		// Sequential search
		if (nOldDataSize <= SEQUENTIAL_SEARCH_THRESHOLD) {
			// Optimize the heck out of it
			// We are transferring elements from not-indexed into not-indexed
			// Just copy the first m_nElementCount elements
			memcpy(m_pData, pOldData, m_nElementCount * sizeof(Element));
		}
		else {
			// We are transferring from indexed into non indexed. We have to scan all elements.
			Hash nIndex = 0;
			for (Hash i = 0; i < nOldDataSize; i++) {
				if (!isEmptyKey(pOldData[i].first)) {
					// Just copy the element in bulk
					memcpy(&(m_pData[nIndex]), &(pOldData[i]), sizeof(Element));

					nIndex++;
				}
			}
		}			
	}
	else {
		// Indexed search
		Hash nIndex;
		for (Hash i = 0; i < nOldDataSize; i++) {
			if (!isEmptyKey(pOldData[i].first)) {
				findIndex(pOldData[i].first, nIndex);

				// Just copy the element in bulk
				memcpy(&(m_pData[nIndex]), &(pOldData[i]), sizeof(Element));
			}
		}
	}

	free(pOldData);
}

template <class Key, class Type>
void BinHashMap<Key, Type>::roundSize(Hash& nSize, unsigned int& nMaxBits) 
{
	if (0 == nSize) {
		nMaxBits = 0;
		return;
	}

	// Make sure the first bit is not set, as we would exceed the maximum size for the hash
	//assert(0 == (nSize >>(sizeof(nSize) * 8 - 1)));

	Hash nRoundedSize;
	nMaxBits = 0;
	for (nRoundedSize = 1; nRoundedSize < nSize; nRoundedSize = (nRoundedSize << 1)) {
		nMaxBits++;
	}

	nSize = nRoundedSize;
	// nMaxBits is already set
}

} // namespace LW

#endif // _BIN_HASH_MAP_H
