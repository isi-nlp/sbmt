#!/usr/bin/env perl
my $beg=shift;
my $end=shift;
$end > $beg || die;
my $template=shift;
defined $template || die;

my ($width,$height)=(1550,570);

use CGI qw(:standard);
#print header;
print start_html("viterbi em derivations");
sub getsvg
{
    my ($i,$b)=@_;
    return '<embed src="'.eval($template)."\" width=$width height=$height/><br />\n";
}
for ($beg..$end) {
#    print h3("Deriv #$_");
    print getsvg($_,0),getsvg($_,1);
}
print end_html;
