#!/usr/bin/env perl

while (<>) {
    /^\s*(\S+)\s+(.*)\s+(\S+)\s*$/; # first token is count, last token is event, everything in between is context
    if (defined($prev) && $prev ne $2) {
        while (($x, $p) = each %block) {
            $p /= $sum;
            print "$p\t$prev $x\n" if ($p >= $threshold);
        }
        $sum = 0;
        %block = ();
    }
    $sum += $1;
    $prev = $2;
    $block{$3} = $1;
}
if (defined($prev)) {
    while (($x, $p) = each %block) {
        $p /= $sum;
        print "$p\t$prev $x\n" if ($p >= $threshold);
    }
}




