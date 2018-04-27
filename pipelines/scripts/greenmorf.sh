#!/usr/bin/env bash
SOURCEFILE=$1
SOURCELANG=$2
USOURCELANG=$2
if test "x$SOURCELANG" == "xil3"; then USOURCELANG="uig"; fi
if test "x$SOURCELANG" == "xil5"; then USOURCELANG="tir"; fi
if test "x$SOURCELANG" == "xil6"; then USOURCELANG="orm"; fi
if test "x$SOURCELANG" == "xil4"; then USOURCELANG="ukr"; fi
OTMPDIR=$3
MORFTBL=$4
SCRIPTDIR=$(dirname $BASH_SOURCE)
mkdir -p $OTMPDIR
TDIR=$(mktemp -d $OTMPDIR/ugreenXXXXX.tmp)
mkdir -p $TDIR
cat $SOURCEFILE | /home/nlg-03/mt-apps/green-rules/latest/bin/ugreen.pl -l $USOURCELANG \
| sed -e "s/.* ::$USOURCELANG \(.*\) ::eng \(.*\) ::sem-type .*/\1\t\2/" \
| grep -v '^$' | lc -f 'green\t%line' > $TDIR/tsv

$SHOME/elisa-pipe/dict2rulesmorf.sh $TDIR/tsv $MORFTBL $TDIR/green.green-rules green
perl -i -e 'while(<>){chomp; $line=$_; ~/.*foreign-length=10\^-(\d+)/; $s=$1*$1; print "$line greenlensq=10^-$s\n";}' $TDIR/green.green-rules

$SCRIPTDIR/dumpmatchmorf $TDIR/green.green-rules \
/home/nlg-02/pust/Morfessor-2.0.1/segment.sh $MORFTBL
