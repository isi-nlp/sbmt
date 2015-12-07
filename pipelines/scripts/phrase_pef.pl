#!/usr/bin/env perl

use FindBin qw($Bin);
use lib $Bin;

use NLPRules qw(extract_lhs_safe extract_rhs_fast extract_rhs_safe target_words source_words extract_feat_safe feature_spec);

# stdin:  rule
# stdout: f \t e \t id \t count

while (<>) {
    chomp;
    $rule = $_;
    $e = join(" ", target_words(extract_lhs_safe($rule)));
    $f = join(" ", source_words(extract_rhs_safe($rule)));
    %v = feature_spec(extract_feat_safe($rule));
    
    print "$f\t$e\t$v{id}\t$v{count}\n";
}
