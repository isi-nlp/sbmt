HIERO=/home/nlg-01/chiangd/hiero-mira2
export PYTHONPATH=$HIERO:$HIERO/lib
export LD_LIBRARY_PATH=$HIERO/lib

# this is a flag for MPI at USC, to keep it from conflicting
# with Python's malloc.
export MX_RCACHE=0 
