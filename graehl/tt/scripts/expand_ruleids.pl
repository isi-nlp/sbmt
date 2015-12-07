#!/usr/bin/env perl

#
# Author: graehl
# Created: Tue Aug 24 16:42:04 PDT 2004
#
# see usage (@end)
#

use strict;
use warnings;

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
my $probfile=undef;
my $countfile=undef;

GetOptions("help" => \&usage,
           "idregexp:s" => \$id_regexp,
           "probregexp:s" => \$prob_regexp,
           "rules=s" => \$rules_file,
           "quote" => \$quote,
           "charset:s" => \$charset,
); # Note: GetOptions also allows short options (eg: -h)
&debug($prob_regexp);

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

my %rules=();
my %probs=();
my %counts=();
my $encin=defined $charset ? $charset : "utf8";

use encoding 'utf8';
binmode STDERR, ":encoding(utf8)";
&debug("encoding",$encin,"rules_file",$rules_file);
open(RULES,"<:encoding($encin)",$rules_file) || die $!;
use open IN => ":encoding(utf8)";

#if (defined $countfile) {
#    open COUNTS,$countfile  || die;
#}
while(<RULES>) {
    if (/$id_regexp/) {
        chomp;
        my $line=$_;
        my $id=$1; 
        my $prob='';
        $prob = $1 if $line =~ /$prob_regexp/;
        $probs{$id}=$prob if ($prob);
        $line =~ s/\s*\#\#\#.*$//;
        &debug($id,$line);
        my $line_utf8=$line;

        $rules{$id}=$line_utf8;
        &debug($id,$line_utf8);
    }
}

&debug(%rules);

sub exprule {
    my ($id,$prevchar)=@_;
    return "\"unknown #$id\"" unless exists $rules{$id};
    my $rule= $rules{$id};
    my ($l,$r)=split /\s*->\s*/,$rule;
#    $l="\"$1\"$2" if $l =~ /^([^A-Z(]+)(.*)$/;
    $l=~ s/([^() ]+)/(substr($1,0,1)eq'"'?$1:"\"$1\"")/ge;
    my $h=exists $probs{$id} ? $probs{$id} : '';
    if ($quote) {
        $rule = "$l->\\n$r\\n$h";
        if(1) {
            $rule =~ s/(\")/\\$1/g;
        } else {
            $rule =~ s/\" \"//g;
            $rule =~ s/\"//g;
        }
        $rule="\"$rule\"";
        return $rule;
    } else {
        $r =~ s/\"//g;
        $r="\"$r\"";
            ##$id:
        my $sub="\"$h\" , $l , $r, ";
        if ($prevchar eq '(') {
            return $sub;
        } else {
            return "($sub)";
        }
    }
}

    while(<>) {
    s/(\D?)(\d+)/$1.&exprule($2,$1)/ge;  
    print;
}

__END__

=head1 NAME

    getlines_id.pl

=head1 SYNOPSIS

    getlines_id.pl [options] index1 index2 index3...

     Options:
       --help          
       --idregexp regexp
       --rules rulesfile
       --charset encoding
       --quote

=head1 OPTIONS

=over 8

=item B<-help>

    Print a brief help message and exits.

=item B<--rules filename>

    get rules (with ### separator followed by attribute id=NNN) from here (using the specified encoding)
    
=item B<--idregexp 'regexp'>

    (default is "id=(\d+)").  $1 is the id checked against the user input

=item B<--probregexp 'regexp'>

   used to get prob from rules (capture into $1)
    
=item B<--charset encoding>

    (default is none).  output is converted to utf8 from encoding (e.g. gb2312) if specified

=item B<--quote>

    double-quote (and escape internal double quotes) rules

=back

=head1 DESCRIPTION

    Substitutes for any digit-sequence NNN the id=NNN rule (everything before '###') from rulesfile
    
=cut  
