#!/bin/bash

# Same as Ignacio's qsubrun.sh, but I added to optional parameters:
#
# - 2nd argument: seconds of sleep before the job is actually run.
# - 3rd argument: dependencies with other jobs, e.g. 
# "3088.hpc-master.usc.edu:3089.hpc-master.usc.edu" means that 
# submitted job will be run only after jobs 3088 and 3089 are
# finished.  ("none" means none)
#
# added by Steve:  set queue=xxx to use alt queue

CMD=$1
SLEEP=$2
DEPEND=$3
queue=${queue:-isi}
CWD=`pwd`;
FNAME=`mktemp $HOME/nlg/jobs/QSUBRUN.XXXXXX`;

echo "#!/bin/bash" > $FNAME;

echo "#PBS -l nodes=1" >> $FNAME;
echo "#PBS -l walltime=48:00:00" >> $FNAME;
echo "#PBS -l mem=2000mb" >> $FNAME;
echo "#PBS -q $queue" >> $FNAME;
if [ -n "$DEPEND" ]; then
  echo "#PBS -W depend=afterany:$DEPEND" >> $FNAME;
fi
echo "#PBS -e /tmp/\$PBS_JOBID.\$PBS_JOBNAME" >> $FNAME;
echo "#PBS -o /tmp/\$PBS_JOBID.\$PBS_JOBNAME" >> $FNAME;
echo "ulimit -c 0" >> $FNAME;
echo "unset LANG" >> $FNAME
echo "unset SUPPORTED" >> $FNAME
echo "cd $CWD" >> $FNAME;
if [ -n "$SLEEP" ]; then
  echo "sleep $SLEEP" >> $FNAME;
fi
echo -e "$1" >> $FNAME;

qsub -V $FNAME;
