// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _COUNTS_READER_H
#define _COUNTS_READER_H

#include <iostream>
#include <assert.h>

#include "LangModel/LangModel.h"
#include "Trie.h"

namespace LW {

class CountsReader
{
public:
	/// This class will NOT delete the pointer passed
	/// @arg pInput The stream to read the counts from
	/// @arg bHasDoubleCounts if true, we have two counts per n-gram as opposed to just one
	CountsReader(std::istream* pInput, bool bHasDoubleCounts = false);
	/// Destructor
	~CountsReader();
public:
	/// This method will load a text count files (count, word, word, word...) into a count trie
	/// The caller can request to add new words to the vocabulary (bUpdateVocab = true), or just use existing ones
	/// (if the vocabulary is already loaded)
	/// @arg in [in] Input Stream to read counts from
	/// @arg vocab [in/out] Vocabulary used to map the words
	/// @arg bUpdateVocab [in] if set to false, the vocabulary will not be expanded
	///      with new words
	/// @arg counts [out] Trie used to hold the counts
	/// @arg nMaxOurder [out] Max order found in the vocabulary
	static void readCounts(
		std::istream& in, 
		LWVocab& vocab, 
		bool bUpdateVocab, 
		TrieNode<LWVocab::WordID, int>& counts, 
		unsigned int* pnMaxOrderFound);
public:
	/// Indicates some problem with the stream
	inline bool fail();
	/// Makes sure we have an n-gram of the desired order in the buffer
	/// If the current n-gram order is less, we keep reading until we find one if any
	/// If the current n-gram order is higher, just return false;
	/// Returns true if we have an n-gram of the desired order in the buffer
	bool moveOrder(unsigned int nOrder);
	/// Returns the current n-gram order we are reading from the file
	unsigned int getCurrentOrder() const;
	/// Returns the current n-gram 
	void getCurrentNGram(char** ppWords) const;
	/// Returns the current count
	int getCurrentCount() const;
	/// Returns the second count (if available in the file)
	int getCurrentCount2() const;
	/// Returns false if there is no more data for the given order
	bool moveNext();
	/// Returns -1 if the current value is < passed value
	/// Returns 0 if the current value == passed value
	/// Returns 1 otherwise
	/// The caller must check the order of the current n-gram to avoid comparing apples and oranges
	int compare(char** ppWords) const;
	/// Max order is the max n-gram order present in the file
	unsigned int getMaxOrder() const;
	/// Count summary is an array of unsigned int. cs[n] is the count summary for n-gram
	void getCountSummary(unsigned int* pnCountSummary) const;
	/// Accessor
	bool getMinFlag() const;
	/// Accessor
	void setMinFlag(bool bMinFlag);
public:
	/// Writes the header of the file (n-gram count summary, etc)
	/// pnCountSummary is allowed to be null if the caller does 
	/// not know the counts
	static void writeHeader(std::ostream& out, unsigned int nMaxOrder,  unsigned int* pnCountSummary = NULL);
	/// Writes the header for each n-gram (e.g. "\2-grams:"
	static void writeNGramHeader(std::ostream& out, unsigned int nOrder);
	/// Writes an nGram having single counts
	static void writeNGram(std::ostream& out, unsigned int nOrder, char** nGram, int nCount);
	/// Writes an nGram having double Counts
	static void writeNGram(std::ostream& out, unsigned int nOrder, char** nGram, int nCount, int nCount2);
	/// Writes the footer ("\end\");
	static void writeFooter(std::ostream& out);
private:
	/// Describes various statuses encountered during parsing
	enum ParseStatus{PS_BEFORE_DATA, PS_READING_COUNTS, PS_BEFORE_READING_NGRAM, PS_READING_NGRAM, PS_DONE};
private:
	/// If this variable is true, the file holds 2 counts per n-gram. If not, it holds one count per n-gram
	bool m_bHasDoubleCounts;
	/// Buffer for the current line
	char m_currentLine[4096];
	/// The stream we read the data from
	std::istream* m_pInput;
	/// Signals whether we have any valid pendig data. If not, we have finished reading the file
	bool m_bHasPendingData;
	/// Stores pending counts. Same as m_pPendingWords
	int m_nPendingCount;
	/// Stores the second count (in case we have 2 counts per n-gram)
	int m_nPendingCount2;
	/// Buffer that stores the words accross calls. e.g. if the caller asks for a 2-gram count and there's nothing left
	/// we read a 3-gram count and store it until next time we are called and we set the pending data flag (m_bPendingData)
	char* m_pPendingWords[MAX_SUPPORTED_ORDER];
	/// Summarizes counts by n-gram order, as read from the file
	unsigned int m_countSummary[MAX_SUPPORTED_ORDER + 1];
	/// Max order, as read from the file header
	unsigned int m_nMaxOrder;
	/// The current order of n-grams we are reading
	unsigned int m_nCurrentOrder;
	/// Variable used by moveOrder() and moveNext() to communicate so moveNext() will stop 
	/// advancing
	unsigned int m_nExpectedOrder;
	/// Keeps track of parsing status
	ParseStatus m_nParseStatus;
	/// External flag, used for example by the merger. Has no internal meening.
	/// We are lazy here, we should use a decorator class
	bool m_bMinFlag;
	/// Current line number for reporting parsing errors
	unsigned int m_nCurrentLineNumber;
};

} // namespace LW

#endif // _COUNTS_READER_H

