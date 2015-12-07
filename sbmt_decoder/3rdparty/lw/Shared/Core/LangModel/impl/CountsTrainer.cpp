// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include <Common/stldefs.h>
#include "LangModel/CountsTrainer.h"

#include "Common/Util.h"

using namespace std;

namespace LW {

	class SimpleCountsReader {
	public:
		/// Constructor
		SimpleCountsReader(std::istream& in, int nOrder);
		~SimpleCountsReader();
	public:
		/// Each reader is initialized and works for one n-gram order only.
		int getOrder();
		/// Returns the n-gram count of the current line
		int getCurrentCount();
		/// Sets the current count. This is not saved, the operation is just performed in memory.
		void setCurrentCount(int nCount);
		/// Returns the nIndex word in the n-gram on the current line
		const char* getCurrentWord(int nIndex);
	public:
		/// Moves to the next line. Returns false at the end of the file.
		bool next();
	private:
		/// The stream we are reading the lines from
		istream& m_in;
		/// Each reader is initialized and works for one n-gram order only.
		int m_nOrder;
		/// Current count
		int m_nCount;
		/// Current n-grams
		std::vector<const char*> m_vWords;
		/// Buffer containing the current line
		char m_pBuffer[4096];
		/// Flag that indicates a problem parsing the current line
		bool m_bBad;
	};

	SimpleCountsReader::SimpleCountsReader(istream& in, int nOrder)
		: m_in(in)
	{
		m_nOrder = nOrder;
		// m_pBuffer is already initialized
		m_bBad = false;
	}

	SimpleCountsReader::~SimpleCountsReader()
	{
	}

	int SimpleCountsReader::getOrder() 
	{
		return m_nOrder;
	}

	int SimpleCountsReader::getCurrentCount() 
	{
		return m_nCount;
	}

	void SimpleCountsReader::setCurrentCount(int nCount)
	{
		m_nCount = nCount;
	}
		

	const char* SimpleCountsReader::getCurrentWord(int nIndex)
	{
		return m_vWords[nIndex];
	}

	bool SimpleCountsReader::next() 
	{
		if (m_in) {
			m_bBad = false;
			safeGetLine(m_in, m_pBuffer, sizeof(m_pBuffer));

			// Read the counts
            char *nextToken;
			char* szCount = strtok_s(m_pBuffer, " \t", &nextToken);
			if (szCount) {
				m_nCount = atoi(szCount);
			}
			else {
				m_bBad = true;
			}

			m_vWords.clear();
			if (!m_bBad) {
				for (int i = 0; (i < m_nOrder) && !m_bBad; i++) {
					char* szWord = strtok_s(NULL, " \t", &nextToken);	
					if (szWord) {
						m_vWords.push_back(szWord);
					}
					else {
						m_bBad = true;
					}
				}
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}


/////// CountsTrainer ////////////

void CountsTrainer::computeKNDiscountCoefs(KNDiscount& discount, unsigned int nOrder, istream& in)
{
	// The file looks like
	// <count> <word1> <word2> ... <wordn>
	
	char szLine[2048];

	unsigned int n1 = 0;
	unsigned int n2 = 0;
	unsigned int n3 = 0;
	unsigned int n4 = 0;

	while (in) {
		// TODO: deal with the eventual fail()
		in.getline(szLine, sizeof(szLine));

		// Parse the line. We only care about counts, so stop at the first tab
        char* nextToken;
		char* szCount = strtok_s(szLine, "\t\n\r ", &nextToken);
		if (szCount) {
			int nCount = atoi(szCount);

			switch (nCount) {
				case 1:
					n1++;
					break;
				case 2:
					n2++;
					break;
				case 3:
					n3++;
					break;
				case 4:
					n4++;
					break;
			}
		}
	}

	// This should never happen if we have enough n-grams, but 
	// adjust the values, just in case;
	if (n1 == 0 || n2 == 0 || n3 == 0 || n4 == 0) {
		n1++;
		n2++;
		n3++;
		n4++;
	}
		
	// Apply the formula to compute the discount coefs
	double y = (double) n1 / (n1 + 2 * n2);

	discount.d1			= 1 - 2 * y * n2 / n1;
	discount.d2			= 2 - 3 * y * n3 / n2;
	discount.d3plus		= 3 - 4 * y * n4 / n3;
}

void CountsTrainer::processKNCounts(unsigned int nOrder, std::istream& in, std::ostream& extraCounts)
{
	SimpleCountsReader reader(in, nOrder);

	while (reader.next()) {
		// Nuke all counts that do not start with <s>
		if (0 != strcmp("<s>", reader.getCurrentWord(0))) {
			reader.setCurrentCount(0);
		}

		// For each unique w0, w1, w2 increment Count(w1, w2) (by adding a count of one and merging them later)
		// We know that the n-grams are unique at this point
		// We just generate "extra" counts and they will be merged with the original counts
		extraCounts << 1;
		for (unsigned int i = 0; i < nOrder; i++) {
			extraCounts << "\t" << reader.getCurrentWord(i);
		}
		extraCounts << endl;
	}
}


} // namespace LW
