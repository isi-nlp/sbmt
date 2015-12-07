#!/usr/bin/env perl
use FindBin qw($Bin);
use lib $Bin;
use NLPRules qw(extract_lhs_safe extract_rhs_safe extract_feat_safe feature_spec);

# trivial_cond_prob.pl
# input:  rule
# output: etree \t id \t count

while (<>) {
    chomp;
    $rule = $_;
    $e = extract_lhs_safe($rule);
    $f = extract_rhs_safe($rule);
    %v = feature_spec(extract_feat_safe($rule));
    print "$e\t$f\t$v{id}\t$v{count}\n";
}
