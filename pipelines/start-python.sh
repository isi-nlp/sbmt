#if test -e /usr/usc/python/2.7.8/setup.sh; then
#    . /usr/usc/python/2.7.8/setup.sh
#fi
export PATH=/home/nlg-01/chiangd/pkg64/python/bin:$PATH
SRC=$(cd $(dirname $BASH_SOURCE); pwd)
export PYTHONPATH=$SRC/lib/python2.7/site-packages/:$SRC/lib
export LD_LIBRARY_PATH=$SRC/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$SRC/lib:$DYLD_LIBRARY_PATH
export LANG=C
export LC_ALL=C
