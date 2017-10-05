#!/usr/bin/env perl

use 5.006;
use warnings;
use strict;
use Cwd 'abs_path';
use File::Spec;
use Getopt::Long qw(:config bundling no_ignore_case);
use File::Basename;


my $SCRDIR;
BEGIN{
	$SCRDIR = File::Basename::dirname(abs_path($0));
}


my $DEINT_DERIV_AUX =  File::Spec->catfile($SCRDIR, "deint_deriv_aux.pl");
my $DERIV2XRS = File::Spec->catfile($SCRDIR, "deriv2xrs");

die "Cannot execute $DEINT_DERIV_AUX\n" unless -x $DEINT_DERIV_AUX;
die "Cannot execute $DERIV2XRS\n" unless -x $DERIV2XRS;


sub usage {
    print << "EOF";
Usage: @{[basename($0)]} [options] 

 --deriv  file  deriv file
 --db|D   file  db file
 --log    file  log file

EOF
    exit(1);
}


my $deriv_file;
my $db_file;
my $log_file;
GetOptions(
    'deriv=s' => \$deriv_file,
    'db|D=s' => \$db_file,
    'log=s' => \$log_file,
    'help|h' => \&usage,
  ) || die "ERROR: Problem processing options\n";

usage() unless $deriv_file;
usage() unless $db_file;

my @cmd = ($DERIV2XRS, '--deriv-file', $deriv_file, '--db', $db_file);
my $cmd = join(' ', @cmd);
$cmd .= " 2> $log_file.1 | $DEINT_DERIV_AUX 2> $log_file.2";
warn "# $cmd\n";
system("$cmd");







