#!/usr/bin/perl

package xrs::exec;

#####################################################
# function to run either normal, failsafe, or 
# condor commands.
#
# Created by Michel Galley (galley@cs.columbia.edu)
# $Id: exec.pm,v 1.1.1.1 2006/03/05 09:20:27 mgalley Exp $
#####################################################

use strict;
use POSIX;
use Fatal qw(open close);
use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

use xrs::env qw($EXEC_SAFE);

$VERSION     = 1.00;
@ISA         = qw(Exporter);
@EXPORT      = qw();
@EXPORT_OK   = qw(&runme);

# Print and run:
sub runme {
	my %args = @_;
	my $cmd = $args{cmd} || return "";
	my $stdout = $args{stdout} || return "";
	my $mode = $args{mode} || 'normal';
	# Three modes of execution:
	if($mode eq 'normal') {
		$cmd .= " > $stdout";
	} elsif($mode eq 'failsafe') {
		$cmd = "$EXEC_SAFE $stdout $cmd";
	} elsif($mode eq 'condor') {
		die "missing implementation\n";
	}
	unless($args{norun}) {
	  print STDERR "running: $cmd\n";
	  print `$cmd`;
	}
	return $cmd;
}

1;
