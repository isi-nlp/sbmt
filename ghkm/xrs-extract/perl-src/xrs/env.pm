#!/usr/bin/perl

package xrs::env;

#####################################################
# xrs-extract environment variables.
#
# Created by Michel Galley (galley@cs.columbia.edu)
# $Id: env.pm,v 1.2 2006/10/01 00:30:07 mgalley Exp $
#####################################################

use POSIX;
use Fatal qw(open close);
use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = 1.00;
@ISA         = qw(Exporter);
@EXPORT      = qw();
@EXPORT_OK   = qw($BINDIR $SCRIPTDIR $EXEC_SAFE);

# Directories:
$BASEDIR   = "..";
$BINDIR    = "$BASEDIR/bin";
$SCRIPTDIR = "$BASEDIR/scripts";

# Programs:
$EXEC_SAFE = "$SCRIPTDIR/exec_safe";

1;
