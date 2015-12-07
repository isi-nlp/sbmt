trunk=${SBMT_TRUNK:-~graehl/t}
src=$trunk/graehl/tt
pushd $trunk
svn update
export BIN_PREFIX=/tmp/forest-em-blob
mkdir -p $BIN_PREFIX
cd $src
make -j 4 install
popd
b64=~graehl/isd/hpc-opteron/bin
d64=x86_64
mkdir -p $d64
for f in forest-em sortpercent forestviz treeviz; do 
cp -f $BIN_PREFIX/$f.static $f; cp -f $BIN_PREFIX/$f.debug .
cp -f $b64/$f.static $d64/$f; cp -f $b64/$f.debug $d64
done
rm -rf $BIN_PREFIX
cp -f $src/forest-em-button.sh .
cp -f $src/addfield.pl .
chmod a+x forest-em forest-em-button.sh
exec cp -p $src/makeblob.sh .
