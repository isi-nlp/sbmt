// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _COUNTS_MERGER_H
#define _COUNTS_MERGER_H

#include <vector>
#include <string>
#include <iostream>
#ifdef __GCC2__
#include <fstream.h>
#endif

#include "LangModel/LangModel.h"

namespace LW {
	class CountsMerger {
	public:
		CountsMerger(LangModelParams& lmParams);
		~CountsMerger();
	private:
		void cleanup();
	public:
		bool merge(std::ostream& log, const std::vector<std::string>& inCountFileNames, const std::string sOutFileName);
	private:
		/// Input streams to read the data from
		std::vector<std::ifstream*> m_inFiles;
		/// Used to store count cut-offs
		LangModelParams m_lmParams;
	};

} // namespace LW

#endif // _COUNTS_MERGER_H
