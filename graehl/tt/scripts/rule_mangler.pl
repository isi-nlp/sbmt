#!/usr/bin/env perl

#
# Author: graehl
# Created: Tue Aug 24 16:42:04 PDT 2004
#
# see usage (@end)
#

#use strict;
#use warnings;

use strict;

use Getopt::Long;
use Pod::Usage;
use File::Basename;

### script info ##################################################
my $scriptdir; # location of script
my $scriptname; # filename of script

BEGIN {
   $scriptdir = &File::Basename::dirname($0);
   ($scriptname) = &File::Basename::fileparse($0);
   push @INC, $scriptdir; # now you can say: require "blah.pl" from the current directory
}

require "exec_filter.pl";

#use WebDebug qw(:all);
my $DEBUG=$ENV{DEBUG};
sub debug {
    print STDERR join(',',@_),"\n" if $DEBUG;
}

### arguments ####################################################
&usage() if (($#ARGV+1) < 1); # die if too few args

my $id_regexp='id=(\d+)';
my $prob_regexp='prob=(\S+)';
my $charset=undef;
my $infile_name; # name of input file
my $rules_file;
my $quote=0;
my $htmlent=0;

GetOptions("help" => \&usage,
           "htmlentities" => \$htmlent,
           "idregexp:s" => \$id_regexp,
           "probregexp:s" => \$prob_regexp,
           "rules=s" => \$rules_file,
           "quote" => \$quote,
           "charset:s" => \$charset,
); # Note: GetOptions also allows short options (eg: -h)


use Encode;
use Encode::CN;
sub usage() {
    pod2usage(-exitstatus => 0, -verbose => 2);
#  print "Usage: getlines_id.pl [options] index1 index2 index3... \n\n";
#  print "\t--help\t\tGet this help\n";
#  print "\t--idregexp\t\t(default is "id=(\d+)")\n";
#  print "Selects (in sorted order) lines with id=#index1, id=#index2,... from STDIN";
  exit(1);
}


### main program ################################################

my $encin=defined $charset ? $charset : "utf8";

binmode STDIN, ":$encin";
binmode STDOUT, ":utf8";
binmode STDERR, ":utf8";

while(<>) {
        chomp;
        my $line=$_;
        my $prob='';
        $prob = $1 if $line =~(/$prob_regexp/);
        $line =~ s/\s*\#\#\#.*$//;
        $line="$line\\n$prob" if ($prob);
       
        my $line_utf8=$line;
        if (defined($charset) ) {
            if(0) {
                $line_utf8= &exec_filter("iconv -f $charset -t utf8",$line);
                &exec_exitvalue;
                if ($?) {
                    $line_utf8=$line;
                    print STDERR "Warning: couldn't convert from charset $charset to utf8.\n";
                }
            } else {
                if (0) {
                    if (!defined(from_to($line_utf8,$charset,"utf8"))) {
                        $line_utf8=$line;
                        print STDERR "Warning: couldn't convert from charset $charset to utf8.\n";
                    }
                }
            }
        }
        if ($htmlent) {
            use HTML::Entities;
            &debug($line_utf8);
            $line_utf8 = encode_entities($line_utf8);
            &debug($line_utf8);
        }
        if ($quote) {
            if(1) {
                $line_utf8 =~ s/\"/\\"/g;
            } else {
                $line_utf8 =~ s/\" \"//g;
                $line_utf8 =~ s/\"//g;
            }
            $line_utf8="\"$line_utf8\"";
        }
        print $line_utf8;
        &debug($line_utf8);
}


__END__

=head1 NAME

    getlines_id.pl

=head1 SYNOPSIS

    rule_mangler.pl [options]

     Options:
       --help          
       --rules rulesfile
       --probregexp regexp
       --charset encoding
       --htmlentities
       --quote

=head1 OPTIONS

=over 8

=item B<-help>

    Print a brief help message and exits.

=item B<--probregexp 'regexp'>

    (default is "prob=(\S+)").  $1 is appended to the rule text if found.

=item B<--charset encoding>

    (default is none).  output is converted to utf8 from encoding (e.g. gb2312) if specified

=item B<--quote>

    double-quote (and escape internal double quotes) rules

=back

=head1 DESCRIPTION

    outputs a string label summarizing rules

=cut  
