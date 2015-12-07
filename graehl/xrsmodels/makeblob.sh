src=/home/rcf-12/graehl/dev/xrsmodels
bins=bin/linux/cp-check.static
pushd $src
cvs update
cd ../shared
cvs update
cd $src
make $bins
popd
cp $src/libxrs.pl .
cp $src/green.pm .
cp $src/add-xrs-models.pl .
cp $src/addfield-sparse.pl .
cp $src/create-lexicon-rules.pl .
cp $src/create-manlex-rules.pl .
cp $src/unigram-expectation.pl .
cp $src/identity-ascii-xrs.pl .
cp $src/fixpos.pl .
cp $src/LEXICON.BROWN.AND.WSJ .
for f in $bins; do
 b=${f%.static}
 cp $src/$bins `basename $b`
done
