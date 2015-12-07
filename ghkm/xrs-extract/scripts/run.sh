#!/bin/bash

# Just a dummy script that emulates the interface
# of qsubrun.sh (in the same directory), but that
# actually does NOT submit anything on the cluster.
# Instead, all programs are run locally.
# 
# This script is called in the_button.pl.

CMD=$1
SLEEP=$2
DEPEND=$3
CWD=`pwd`;
FNAME=`mktemp $HOME/jobs/QSUBRUN.XXXXXX`;

echo "#!/bin/bash" > $FNAME;

if [ -n "$SLEEP" ]; then
  echo "sleep $SLEEP" >> $FNAME;
fi
echo -e "$1" >> $FNAME;

source $FNAME;
