// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#include "ScalarEvent.h"

namespace STL_HASH {
#ifdef WIN32
	size_t hash_value(const PointerEvent& pe) {return pe.hash();}
#endif
}

