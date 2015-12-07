# This script should be used to process output of itg_binarizer 
# with --keep-duplicate opton after it was sorted with 'sort'. 
# It collapses duplicate virtual rules.

my @ids;
my $rule;

sub reduce {
  print "$rule id={{{" . (join ' ', @ids) . "}}}\n" if $rule;
  $rule = '';
}

LOOP:
while (<>) {
  if (/^(V.*) id={{{(\d+)}}}$/) {
    if ($1 ne $rule) {
      reduce();
      $rule = $1;
      @ids = ($2);
    }
  
    else {
      push @ids, $2;
    }
  }
  
  else {
    print $_;
  }

}

reduce();
