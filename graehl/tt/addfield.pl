#!/usr/bin/env perl

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

my $DEBUG=$ENV{DEBUG};
sub debug {
    print STDERR join(',',@_),"\n" if $DEBUG;
}

sub expand_tilde {
    my $dir = shift;
    # tilde expansion
    if ($dir =~ /~/) {
	$dir =~ s{ ^ ~( [^/]* ) }
	{ $1 ? (getpwnam($1))[7] : ($ENV{'HOME'} || $ENV{'LOGDIR'} || (getpwuid($>))[7]) }ex;
    }
    return $dir;
}

### arguments ####################################################
my $basename='';
my $forestem="forest-em";
my $which=`which $forestem 2>/dev/null`;
my $root_normfile='';
my $lhs_normfile='';
my $outviterbiprefix='';
my $fieldname='';
chomp $which;
$forestem="/home/nlg-01/blobs/forest-em/latest/forest-em" unless -x $which;

&usage() if (($#ARGV+1) < 1); # die if too few args

GetOptions("help" => \&usage,
           "altforestem:s" => \$forestem,
           "basename:s" => \$basename,
           "root-normgroups-file:s" => \$root_normfile,
           "lhs-normgroups-file:s" => \$lhs_normfile,
           "outviterbiprefix:s" => \$outviterbiprefix,
           "fieldname=s" => \$fieldname,
);

$fieldname = shift if scalar @ARGV == 1;
&usage if scalar @ARGV;

sub usage() {
    pod2usage(-exitstatus => 0, -verbose => 2);
    exit(1);
}

&debug($basename);
$basename=expand_tilde($basename);
&debug($basename);
$_=$fieldname;
my $addk=(s/_add(\d+)$//) ? $1 : '';
print STDERR "addk=$addk\n";
&debug($_);
die "unrecognized fieldname \"$fieldname\" ".'- must match http://twiki.isi.edu/NLP/ForestEMButton' unless /^(unit_|)([^_]+_|)(prob|count)(_it\d+|)?$/;
my ($unit,$norm,$probcount,$iter)=($1,$2,$3,$4);
&debug($unit,$norm,$probcount,$iter);
if($iter) {
 $iter =~ /(\d+)/ or die;
 $iter=$1;
}
unless ($norm) {
    die "you must specify normalization except for unit_count_it1" unless ($fieldname eq 'unit_count_it1'); #($iter and ($probcount eq 'count') and $iter==1);
    $norm="root_";
}
die "only the first iteration of unit initialization is allowed" if ($unit eq 'unit_' && $iter ne 1);
my $normfile='';
if ($addk) {
    die "add k (to denominator) smoothing is meaningless for count - please request prob instead." unless ($probcount eq 'prob');
    $probcount='count';
    $normfile= ( $norm =~ /^root/ ? $root_normfile : $lhs_normfile);
    die "coudln't find -normfile $normfile, required for addk" unless -r $normfile;
}
 my $infile=$unit;
if ($iter) {
 $norm =~ s/_$//;
 $infile .= $norm.(($probcount eq 'prob') ? '.params' : '.counts').'.restart.1.iteration.'.$iter;
} else {
 $infile.=$norm.$probcount;
}
$infile=$basename . $infile;
print STDERR "looking for infile $infile\n";
my $viterbi=$infile;
$viterbi=~s/(counts?|params|prob)/viterbi/;
-r $infile or die "couldn't find $infile";
print STDERR "viterbi em derivations should be at $viterbi (tell tipu!).\n";
if ($outviterbiprefix) {
    if ( -r $viterbi) {
        my $viterbi_link="$outviterbiprefix.$fieldname.viterbi";
        print STDERR "creating symlink to viterbi at $viterbi_link\n";
        unlink $viterbi_link;
        symlink $viterbi,$viterbi_link or die $!;
    } else {
        print STDERR "warning: couldn't find viterbi $viterbi - did you run the button with outviterbi=1?\n" unless -r $viterbi;
    }
}
my @baseargs=($forestem,'-b','-','-B','-','-i','0','-I',$infile,'-F',$fieldname);
my @addkargs=$addk ? ('--normalize-initial','-n',$normfile,'--add-k-smoothing',$addk) : ();
&debug(@baseargs,@addkargs);
system @baseargs,@addkargs;
__END__

=head1 NAME

    addfield.pl

=head1 SYNOPSIS

    addfield.pl [options]

=head1 OPTIONS

=over 8

=item B<-fieldname fieldname> (MANDATORY)

    Write modified rules file here, and also create symlink
    filename.fieldname.viterbi to the viterbi derivations for that fieldname

=item B<-outviterbiprefix outviterbiprefix>

    symlink outviterbiprefix.fieldname.viterbi to the viterbi derivations for
    that fieldname

=item B<-basename pathname_prefix>

    Expect input files: ${basename}{unit_,}{lhs,root}*

=item B<-root-norm pathname>
=item B<-lhs-norm pathname>

    Needed only for _addk fieldname, to perform renormalization

=item B<-altforestem pathname>

    Location of the forest-em program (default forest-em or /home/rcf-12/graehl/bin/forest-em)

=item B<-help>

    Print a brief help message and exits.    

=back

=head1 DESCRIPTION

    Used for outputs of forest-em-button.pl.  Adds field=val to rules on stdin.

=cut
