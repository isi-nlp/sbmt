# Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
$app = LW::Build::App->new(target=>LangModel);
$app->add_defines(LM_NO_BDB_LIB, LM_NO_COMMON_LIB, NO_QT);
$app->add_dep(
"/Shared/Core/LangModel",
"/Shared/Common");
$app->compile_all();
$app->finish();
