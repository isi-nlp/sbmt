#!/usr/bin/perl -I../Shared/PerlLib -I.
# Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

package LW::Build;

use LW::Build::Solution;
use LW::Build::Global;
use ProjectConfig;

$sln = LW::Build::Solution->new(name=>Horse);
$root = "$PROJ_DIR/build-$OS";
$DEFAULT_CONFIG->setRoot($root);

foreach $p (@PROJLIST) {
	$sln->add($p);
}

$sln->finish();
