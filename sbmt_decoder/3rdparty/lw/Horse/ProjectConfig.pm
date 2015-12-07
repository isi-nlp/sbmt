# Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
package LW::Build;

$TOP_DIR="\$(TOP_DIR)";
use LW::Build::Global;
$TOP_DIR="\$(TOP_DIR)";

$sln = LW::Build::Solution->new(name=>Horse);
$TOP_DIR="\$(TOP_DIR)";


# default build environment
$PROJ_DIR = "$TOP_DIR/Horse";
$DEFAULT_CONFIG->add_includes("$PROJ_DIR/Lib");


@PROJ_DECODER_LIBS = (
'/Shared/Core/LangModel',
'/Shared/Common'
);

@REST_OF_THEM = (
'App/LangModel',
);

@PROJLIST = (@PROJ_DECODER_LIBS, @BUFFALO_LIBS, @HORSE_LIBS, @REST_OF_THEM);
