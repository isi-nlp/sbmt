#!/usr/bin/env perl
use FindBin qw($Bin);
use lib $Bin;
use NLPRules qw(extract_root_nt extract_feat_safe feature_spec);

# prob.pl
# input:  rule
# output: root \t id \t count

while (<>) {
    chomp;
    $rule = $_;
    $r = extract_root_nt($rule);
    %v = feature_spec(extract_feat_safe($rule));
    print "$r\t$v{id}\t$v{count}\n";
}
