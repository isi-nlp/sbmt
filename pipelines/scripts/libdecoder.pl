my $in_param=0;
sub decoder_log_type {
    local $_=$_[0];
    return 'param' if $in_param;
#    $in_param=1 if /^<<<+\s*PARAMETERS/;
#    $in_param=0 if /^>>>+\s*PARAMETERS/;
    return 'summary' if /SUMMARY/;
    return 'nbest' if /^NBEST sent/;
    return 'missing' if /^NBEST MISSING/;
    return 'nbestpass' if /^\(pass \d+\) NBEST sent/;
    return 'recover' if /^RECOVERED.*hyp={{{.*?}}}/;
    return 'recoverfailed' if /^recovery failed/i;
    return 'error' if /ERROR:/ || /EXCEPTION:/;
    return 'warning' if /WARNING:/;
    return 'caught' if /CAUGHT:/;
    return 'memory' if /st9bad_alloc/;
    return 'weight' if /^double weight-/;
#    return 'time' if /user seconds/i;
    return 'pass' if /^\(pass \d+\)/;
    return '';
}

1;
