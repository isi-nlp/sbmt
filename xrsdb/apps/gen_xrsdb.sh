#!/bin/bash

# a simple script i wrote to demonstrate creating an xrsdb in parallel.
# not exactly a production-quality tool (notice all the hardcoded variables)
# but it works.

# bin should be set to a current location containing the xrsdb tools
# once in a while i change the internal format of the db, so make sure that you
# query the db with the same version of the tools you used to create it.
bin=/home/nlg-02/pust/bin

# the number of jobs to split the creation task across
m=25

# this is where all intermediary files for creating the xrsdb will go.  
# some intermediary files need to be seen by all processes, and some are just
# saved for debugging any problems that arise. 
# probably this should be scratch...
temp=/home/nlg-02/pust/xrsdb_workspace

# the actual xrsdb directory that will be created.
dbdir=$temp/xrsdb

# the grammar you want to store.  i assume it has been gzipped.  this script
# will not modify the original grammar
grammar=/home/nlg-03/sdeneefe/workflow/ara-rule4030/rules.final.gz

# qsub command.  its never taken more than 2 hours to complete any step, but
# you might want to up the time the first time you run this just in case
tm="2:00:00"
qs="qsub -q isi -l walltime=$tm"

# generate frequency statistics
# each process k will generate stats for lines i in the original grammar 
# satisfying i % m == k, and make a file containing those lines.
# so grammar is split into grammar.i.gz (i = 0..(m-1))
# the frequency tables are stored in freq.i
waitfreq=""
mergein=""
for k in `seq 0 $[$m - 1]`; do
    waitfreq=$waitfreq:`$qs -v g=$grammar,k=$k,m=$m,t=$temp,b=$bin $bin/gen_freq.sh`
done

# the statistics from gen_freq are summed to create a frequency table valid for
# the whole original grammar.  also, a skeletal directory layout 
# is created for xrsdb, stored in "xrsdb".
# the frequency table is stored in freq
waitinit=:`$qs -W depend=afterok$waitfreq -v b=$bin,t=$temp,m=$m,x=$dbdir $bin/gen_init.sh`

# multiword keys and primary word keys are created for each rule.
# grammar files are split on j = hash(primary_word) % m 
# so now grammar file is split into grammar.keyword.i.j.gz (i,j = 0..(m-1))
waitprep=""
for k in `seq 0 $[$m - 1]`; do
    waitprep=$waitprep:`$qs -W depend=afterok$waitinit -v t=$temp,b=$bin,k=$k,m=$m $bin/gen_prep.sh`
done

# each process k merges and sorts grammar.keyword.i.k.gz based on primary key
# into grammar.keyword.k.gz
# rules are inserted into the database
# since each keyword is stored in its own file in xrsdb, and since no two 
# processes will be working on the same keyword, this step can be done in
# parallel
for k in `seq 0 $[$m - 1]`; do
    $qs -W depend=afterok$waitprep -v b=$bin,k=$k,m=$m,t=$temp,x=$dbdir $bin/gen_populate.sh
done

