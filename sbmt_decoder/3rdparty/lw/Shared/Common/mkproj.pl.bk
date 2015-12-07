# Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
my $lib = LW::Build::Lib->new;
$lib->add_defines(LM_NO_BDB_LIB, LM_NO_COMMON_LIB, NO_QT);
#if ($ENV{LWHASP}) {
#    $lib->add_ext_lib($LW::Build::HASP_HL_LIB);
#    $lib->add_defines(LW_HASP_ENABLE);	
#} elsif($ENV{LWCPU2})  {
#    $lib->add_defines(LW_CPU_2);	
#} elsif($ENV{LWCPU4}) {
#    $lib->add_defines(LW_CPU_4);	
#} elsif($ENV{LWCPU16}) {
#    $lib->add_defines(LW_CPU_16);
#}
$lib->compile("impl/MemAlloc.cpp");
$lib->compile("impl/ParamParser.cpp");
$lib->compile("impl/Platform.cpp");
$lib->compile("impl/ConditionVariable.cpp");
$lib->compile("impl/Trace.cpp");
$lib->compile("impl/Assert.cpp");
$lib->compile("impl/logprob.cpp");
$lib->compile("impl/ErrorCode.cpp");
$lib->compile("Vocab/impl/Vocab.cpp");
$lib->compile("Vocab/impl/VocabFactory.cpp");
$lib->compile("Serializer/SerializerBasicTypes.cpp");
$lib->compile("Serializer/Serializer.cpp");
$lib->compile("impl/Util.cpp");
$lib->finish();
