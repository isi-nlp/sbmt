// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include "LangModel/CountsMerger.h"
#ifndef __GCC2__
#include <fstream>
#endif
#include "LangModel/LangModel.h"
#include "CountsReader.h"
#include "Common/handle.h"
#include <cstring>

using namespace std;

namespace LW
{
CountsMerger::CountsMerger(LangModelParams& lmParams)
{
	m_lmParams = lmParams;
}

CountsMerger::~CountsMerger()
{
	cleanup();
}

void CountsMerger::cleanup()
{
	for (size_t i = 0; i < m_inFiles.size(); i++) {
		delete m_inFiles[i];
	}
}

bool CountsMerger::merge(std::ostream& log, const std::vector<std::string>& inCountFileNames, const std::string sOutFileName)
{
	// Perform the cleanup, just in case somebody calls this method twice, without re-creating the class
	cleanup();

	if (inCountFileNames.size() == 0) {
		log << "No input files supplied." << endl;
		return false;
	}

	// Open all files
	for (size_t i = 0; i < inCountFileNames.size(); i++) {
		ifstream* pInput = new ifstream(inCountFileNames[i].c_str());
		if (pInput->fail()) {
			log << "Cannot open input file  " << inCountFileNames[i] << endl;
			return false;
		}
		m_inFiles.push_back(pInput);
	}

	// Open output files; one file per n-gram order
	vector<SmartPtr<ofstream> > vOut;
	for (size_t i = 0; i < MAX_SUPPORTED_ORDER; i++) {
		char szFileName[1000];
		lw_snprintf(szFileName, 999, "%s.%d", sOutFileName.c_str(), i + 1);

		SmartPtr<ofstream> pOut = new ofstream(szFileName);
		vOut.push_back(pOut);

		if (pOut->fail()) {
			log << "Cannot open output file " << szFileName << endl;
			return false;
		}
	}

	ofstream out(sOutFileName.c_str());
	if (out.fail()) {
		log << "Cannot open output file " << sOutFileName << endl;
		return false;
	}

	// Create count readers
	// A vector would be cleaner, but an array is faster, as we have to iterate a lot
	size_t nReaderCount = m_inFiles.size();
	CountsReader** countReaders = new CountsReader* [nReaderCount];
	for (size_t i = 0; i < nReaderCount; i++) {
		countReaders[i] = new CountsReader(m_inFiles[i]);
	}

	// WRITE HEADER
	// At this point we have read the counts for all n-grams
	// but we cannot estimate the counts, as some n-grams are common
	// to more than one file. On top of this, we might skip some n-grams 
	// because of cut-offs. 
	// For now, just put a fake count and deal with it later.
	// The counts are not used for anything right now
	unsigned int counts[MAX_SUPPORTED_ORDER + 1];
	memset(counts, 0, sizeof(counts));
	CountsReader::writeHeader(out, countReaders[0]->getMaxOrder(), counts);

	char* minNGram[MAX_SUPPORTED_ORDER];
	for (unsigned int nOrder = 1; nOrder <= countReaders[0]->getMaxOrder(); nOrder++) {
		ostream* pOut = vOut[nOrder - 1].getPtr();
		bool bHasMin = false;
		do {
			// Find the minimum first
			bHasMin = false;
			for (size_t i = 0; i < nReaderCount; i++) {
				// Only do something if the reader is able
				// to provide us with an n-gram having a certain order
				CountsReader* pReader = countReaders[i];
				if (pReader->moveOrder(nOrder)) {
					if (!bHasMin) {
						// We don't have a min yet, just copy
						pReader->getCurrentNGram(minNGram);
						bHasMin = true;
					}
					else {
						// Check if it less than the min
						if (-1 == pReader->compare(minNGram)) {
							// We found a new minimum
							pReader->getCurrentNGram(minNGram);
						}
					}
				}
			}

			// We have the minimum. Sum up all corresponding n-grams
			if (bHasMin) {
				unsigned int nCountSum = 0;
				for (size_t i = 0; i < nReaderCount; i++) {
					CountsReader* pReader = countReaders[i];
				
					pReader->setMinFlag(false);

					// Make sure this reader has an n-gram of the right order. Skip if it does not.
					if (pReader->moveOrder(nOrder)) {
						if (0 == pReader->compare(minNGram)) {
							// Set the flag to advance 
							pReader->setMinFlag(true);

							// Found a match. Add it up
							nCountSum += pReader->getCurrentCount();
						}
					}
				}

				// We now have the n-gram and the summarized count for it
				// Write it if the counts are above the threashold for this n-gram order
				if (nCountSum >= m_lmParams.m_nCountCutoffs[nOrder]) {
					*pOut << nCountSum;
					for (unsigned int i = 0; i < nOrder; i++) {
						*pOut << "\t" << minNGram[i];
					}
					*pOut << endl;
				}

				// Advance the readers that have the minimum
				// Aparently this could be done in the previous loop,
				// but the char* pointers are in the internal buffer of the reader
				// and they would get screwed up.
				for (size_t i = 0; i < nReaderCount; i++) {
					CountsReader* pReader = countReaders[i];

					// Test for min and advance
					if (pReader->getMinFlag()) {
						pReader->moveNext();
					}
				}
			}
		} while (bHasMin);
	}

	// Write the footer of the merged file
	CountsReader::writeFooter(out);

	// Delete count readers
	for (size_t i = 0; i < nReaderCount; i++) {
		delete countReaders[i];
	}
	delete[] countReaders;

	return true;
}
//bool CountsMerger::merge(std::ostream& log, const std::vector<std::string>& inCountFileNames, const std::string sOutFileName)
//{
//	// Perform the cleanup, just in case somebody calls this method twice, without re-creating the class
//	cleanup();
//
//	if (inCountFileNames.size() == 0) {
//		log << "No input files supplied." << endl;
//		return false;
//	}
//
//	// Open all files
//	for (size_t i = 0; i < inCountFileNames.size(); i++) {
//		ifstream* pInput = new ifstream(inCountFileNames[i].c_str());
//		if (pInput->fail()) {
//			log << "Cannot open input file  " << inCountFileNames[i] << endl;
//			return false;
//		}
//		m_inFiles.push_back(pInput);
//	}
//
//	// Open output file
//	ofstream out(sOutFileName.c_str());
//	if (out.fail()) {
//		log << "Cannot open output file " << sOutFileName << endl;
//		return false;
//	}
//
//	// Create count readers
//	// A vector would be cleaner, but an array is faster, as we have to iterate a lot
//	size_t nReaderCount = m_inFiles.size();
//	CountsReader** countReaders = new CountsReader* [nReaderCount];
//	for (size_t i = 0; i < nReaderCount; i++) {
//		countReaders[i] = new CountsReader(m_inFiles[i]);
//	}
//
//	// WRITE HEADER
//	// At this point we have read the counts for all n-grams
//	// but we cannot estimate the counts, as some n-grams are common
//	// to more than one file. On top of this, we might skip some n-grams 
//	// because of cut-offs. 
//	// For now, just put a fake count and deal with it later.
//	// The counts are not used for anything right now
//	unsigned int counts[MAX_SUPPORTED_ORDER + 1];
//	memset(counts, 0, sizeof(counts));
//	CountsReader::writeHeader(out, counts, countReaders[0]->getMaxOrder());
//
//	char* minNGram[MAX_SUPPORTED_ORDER];
//	for (unsigned int nOrder = 1; nOrder <= countReaders[0]->getMaxOrder(); nOrder++) {
//		// Write n-gram header
//		CountsReader::writeNGramHeader(out, nOrder);
//
//		bool bHasMin = false;
//		do {
//			// Find the minimum first
//			bHasMin = false;
//			for (size_t i = 0; i < nReaderCount; i++) {
//				// Only do something if the reader is able
//				// to provide us with an n-gram having a certain order
//				CountsReader* pReader = countReaders[i];
//				if (pReader->moveOrder(nOrder)) {
//					if (!bHasMin) {
//						// We don't have a min yet, just copy
//						pReader->getCurrentNGram(minNGram);
//						bHasMin = true;
//					}
//					else {
//						// Check if it less than the min
//						if (-1 == pReader->compare(minNGram)) {
//							// We found a new minimum
//							pReader->getCurrentNGram(minNGram);
//						}
//					}
//				}
//			}
//
//			// We have the minimum. Sum up all corresponding n-grams
//			if (bHasMin) {
//				unsigned int nCountSum = 0;
//				for (size_t i = 0; i < nReaderCount; i++) {
//					CountsReader* pReader = countReaders[i];
//				
//					pReader->setMinFlag(false);
//
//					// Make sure this reader has an n-gram of the right order. Skip if it does not.
//					if (pReader->moveOrder(nOrder)) {
//						if (0 == pReader->compare(minNGram)) {
//							// Set the flag to advance 
//							pReader->setMinFlag(true);
//
//							// Found a match. Add it up
//							nCountSum += pReader->getCurrentCount();
//						}
//					}
//				}
//
//				// We now have the n-gram and the summarized count for it
//				// Write it if the counts are above the threashold for this n-gram order
//				if (nCountSum >= m_lmParams.m_nCountCutoffs[nOrder]) {
//					out << nCountSum;
//					for (unsigned int i = 0; i < nOrder; i++) {
//						out << "\t" << minNGram[i];
//					}
//					out << endl;
//				}
//
//				// Advance the readers that have the minimum
//				// Aparently this could be done in the previous loop,
//				// but the char* pointers are in the internal buffer of the reader
//				// and they would get screwed up.
//				for (size_t i = 0; i < nReaderCount; i++) {
//					CountsReader* pReader = countReaders[i];
//
//					// Test for min and advance
//					if (pReader->getMinFlag()) {
//						pReader->moveNext();
//					}
//				}
//			}
//		} while (bHasMin);
//	}
//
//	// Write the footer of the merged file
//	CountsReader::writeFooter(out);
//
//	// Delete count readers
//	for (size_t i = 0; i < nReaderCount; i++) {
//		delete countReaders[i];
//	}
//	delete[] countReaders;
//
//	return true;
//}

} // namespace LW
