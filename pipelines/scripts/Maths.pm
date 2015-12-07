#
#===============================================================================
#
#         FILE:  Maths.pm
#
#  DESCRIPTION:  
#
#        FILES:  ---
#         BUGS:  ---
#        NOTES:  ---
#       AUTHOR:   (), <>
#      COMPANY:  
#      VERSION:  1.0
#      CREATED:  11/04/05 12:30:21 PST
#     REVISION:  ---
#===============================================================================

package Maths;
require Exporter;

our @ISA = ("Exporter");
our @EXPORT = qw(ln_add log10_add convert_base my_log10);
use strict;


sub ln_add {
    my ($x, $y) = @_;
    my $negInf = -inf;

    if($y == $negInf or $x - $y > 20.0) {return $x;}
    if($x == $negInf or $y - $x > 20.0) {return $y;}
    if($x > $y) {return log(1.0 + exp($y - $x)) + $x;}
    else {return log(1.0 + exp($x - $y)) + $y;}
}

sub log10_add {
    my ($x, $y) = @_;
	return &convert_base(10.0, &ln_add($x, $y))
}

sub my_log10
  {
      my $prob = shift @_;
	  $prob = log($prob)/log(10);
      return $prob;
  }

sub convert_base{
   my ($base, $value ) = @_;
   return $value/ log($base);
}

1;
