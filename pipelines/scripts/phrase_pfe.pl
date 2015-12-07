#!/usr/bin/env perl

use FindBin qw($Bin);
use lib $Bin;
use NLPRules qw(extract_lhs_safe extract_rhs_fast extract_rhs_safe target_words source_words extract_feat_safe feature_spec);

# stdin:  rule
# stdout: e \t f \t id \t count

while (<>) {
    chomp;
    $rule = $_;
    $e = join(" ", target_words(extract_lhs_safe($rule)));
    $f = join(" ", source_words(extract_rhs_safe($rule)));
    %v = feature_spec(extract_feat_safe($rule));
    
    print "$e\t$f\t$v{id}\t$v{count}\n";
}
