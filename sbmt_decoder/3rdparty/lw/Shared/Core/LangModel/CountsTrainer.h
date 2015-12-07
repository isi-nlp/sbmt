// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _COUNTS_TRAINER_H
#define _COUNTS_TRAINER_H

#include <vector>
#include <string>
#include <iostream>
#ifdef __GCC2__
#include <fstream.h>
#endif

// TODO: It is wrong to include a file this way. KNDiscount structure must be moved in a different file
#include "LangModel/impl/LangModelKN.h"


namespace LW {

	class CountsTrainer 
	{
	public:
		static void computeKNDiscountCoefs(KNDiscount& discount, unsigned int nOrder, std::istream& in);
		static void processKNCounts(unsigned int nOrder, std::istream& in, std::ostream& out);
	};

} // namespace LW

#endif // _COUNTS_TRAINER_H
