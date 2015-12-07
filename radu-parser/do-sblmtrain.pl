#!/usr/usc/bin/perl

# takes a tree file for sblm training, and computes GRAMMAR/ and TRAINING/ info

if( scalar(@ARGV)!=1 ){ die "Usage: do-sblmtrain.pl tree-file\n"; }

$SOURCE = $ARGV[0];
if( $SOURCE =~ /^(.*)\/training\-data\/(.*?)$/ ){ $DESTDIR = $1; }
elsif( $SOURCE =~ /^(.*)\/(.*?)$/ ){ $DESTDIR = $1; }
else{ $DESTDIR = "."; }

# create the directory structure
if( !(-e "$DESTDIR/sblm") ){
    system("mkdir $DESTDIR/sblm");
    $DESTDIR = "$DESTDIR/sblm";
    system("mkdir $DESTDIR/MT");
    system("mkdir $DESTDIR/TRAINING");
    system("mkdir $DESTDIR/GRAMMAR");
}
else{
    $DESTDIR = "$DESTDIR/sblm";
    if( !(-e "$DESTDIR/MT") ){ system("mkdir $DESTDIR/MT"); }
    if( !(-e "$DESTDIR/TRAINING") ){ system("mkdir $DESTDIR/TRAINING"); }
    if( !(-e "$DESTDIR/GRAMMAR") ){ system("mkdir $DESTDIR/GRAMMAR"); }
}

if( system("extractRules.pl $SOURCE $DESTDIR") ){ mydie("Failure in extractRules.pl"); }

if( system("sortRules.pl hf $DESTDIR") ){ mydie("Failure in sortRules.pl hf"); }
if( system("sortRules.pl mod $DESTDIR") ){ mydie("Failure in sortRules.pl mod"); }




sub mydie{
    my ($err) = @_;

    my $errlog = "$DESTDIR/errorlog.do-sblmtrain";
    open(E, ">$errlog") or die "Cannot open errlog file $errlog\n";
    print E "$err\n";
    close(E);
    die;
}
