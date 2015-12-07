# Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
$lib = LW::Build::Lib->new;
$lib->add_defines(_DECODER, VOCAB_USE_STD_MAP, LM_NO_BDB_LIB, LM_NO_COMMON_LIB, NO_QT);
$lib->set_optimization_level(3) if windows eq $LW::Build::OS_TYPE;
$lib->compile_all();
$lib->finish();
