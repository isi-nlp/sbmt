#export BOOST_LOCATION=/home/wwang/3rdparty/boost_1_35_0
export BOOST=/home/wwang/3rdparty/boost_1_35_0
#export BOOST_ROOT=/home/wwang/3rdparty/boost_1_35_0
#export BOOST_BUILD=/lwbest/tmp-use/runtime-info/boost-build
export BOOST_BUILD_PATH=/home/wwang/3rdparty/boost_1_35_0/tools/build/v2
export BOOST_BUILD=/home/wwang/3rdparty/boost_1_35_0/tools/build/v2
# bjam release clean
#bjam --extra-lib=hoard -j 2 --mini-ngram-order=all pipeline release
#bjam -j 2 --mini-ngram-order=5 pipeline release
#bjam -j 1 --mini-ngram-order=5 mini_decoder release
#bjam -j 2 --mini-ngram-order=5 pipeline debug
#bjam  -j 1 --prefix=/home/wwang/sbmt-dev/sbmt_trunk_rule-context --mini-ngram-order=5 pipeline release
#bjam  -j 1 --prefix=/home/wwang/sbmt-dev/sbmt_trunk_rule-context --mini-ngram-order=5 pipeline release 
#bjam   --allocator=tbb -j 1 --prefix=/home/wwang/sbmt-dev/new_feats/bin release install-pipeline 
bjam

