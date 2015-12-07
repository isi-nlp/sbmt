#     Written by Wei Wang.
#===============================================================================

use strict;


package FeatureFuncs;
require Exporter;

use strict;
use Maths;

our @ISA = ("Exporter");
our @EXPORT = qw(cond_feat perm_feat is_lexicalized global_m1);

# &cond_feat(attrHash, nunem-name, denorm-name, feat-name);
sub cond_feat {
    my ($attrHash, $nname, $dname, $fname) = @_;

    my $nval;
    my $dval;
    
    if(defined $attrHash->{$nname}) {
	$nval = $attrHash->{$nname};
    } else {
	print STDERR "$nname not defined.\n";
    }

    if(defined $attrHash->{$dname}) {
	$dval = $attrHash->{$dname};
    } else {
	print STDERR "$dname not defined.\n";
    }

    return "$fname=e^" . log($nval/$dval);
}


# permutation feature.
# &perm_feat(attrHash, fname);
sub perm_feat {
    my ($attHash, $fname) = @_;

    my $rhs= $attHash->{"rhs"};
    my @rhsvars;
    push @rhsvars,$1 while($rhs =~ /\bx(\d+)\b/g);

   my $m=0;
    for (0..($#rhsvars-1)) {
	++$m unless ($rhsvars[$_] + 1) == $rhsvars[$_+1];
    }

    return "$fname=$m";
}

# &is_lexicalized(attrHash, fname);
sub is_lexicalized {
    my ($attHash, $fname) = @_;

    my $lhs = $attHash->{"lhs"};
    my $rhs = $attHash->{"rhs"};

     my @rhsArr = split(' ', $rhs); # rhs tokens
     foreach my $rhsToken (@rhsArr) { # for each token
       if ($rhsToken =~ /^x/) {  # if its an xrs variable
       } else {                  # lexical item
	   return "$fname=1";
       }
   }

    my @lhsWords;
    if($lhs =~ /(\"(.+?)\")/g) {
	   return "$fname=1";
    }
   
   return "$fname=0";
}

# computes the global model 1 features.
# &global_m1(f-string, e-string, t-table)
# \TODO: make it log add.
sub global_m1 {
	my ($f, $e, $ttable) = @_;
	#print $f . $e . "EEE\n";
	my @fw = split /\s+/, $f;
	my @ew = split /\s+/, $e;
	my $l = @ew;
	my $m = @fw;

	# add a NULL at the end of @ew
	push @ew, "NULL";

	my $total_log_prob = 0.0;
	for my $j (0 .. $m - 1){
		# @fw[$j] now.
		my $t_fj_given_e = 0;
		for my $i (0 .. $l){
			my $tmp;
			my $p;
			my $tmp_prob = 0.000000001;
			if (defined($tmp=$ttable->{$ew[$i]})){
				if (defined($p=$tmp->{$fw[$j]})){ 
					if($p > $tmp_prob){ $tmp_prob = $p; }
				}
			}
			$t_fj_given_e += $tmp_prob;
		}
		if($t_fj_given_e == 0){$t_fj_given_e = -20;}
		else {$t_fj_given_e = &my_log10($t_fj_given_e);}
		$total_log_prob += $t_fj_given_e - &my_log10($l+1);
	}

	return $total_log_prob;
}


1;





