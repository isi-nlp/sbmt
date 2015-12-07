if [ "x" = "x$PIPELINE_TRAPS" ]; then
    export PIPELINE_TRAPS=1
    export SBMT_RUNALL_ARGS=$@

    if (test "x" != "x$PBS_NODEFILE") && test -f $PBS_NODEFILE; then
      for x in $(cat $PBS_NODEFILE | sort -u); do
        ssh -n $x "rm -rf /tmp/*.hpc-pbs.hpcc.usc.edu/*" &
        done
      wait
    fi

    function cleanup {
        set +e
        if (test "x" != "x$PBS_NODEFILE") && test -f $PBS_NODEFILE; then
            if (test "xsuccess" != "x$1") && test -d "/tmp/$PBS_JOBID/hadoop-logs"; then
                mkdir -p $PBS_O_WORKDIR/$PBS_JOBID
                for x in $(cat $PBS_NODEFILE | sort -u); do
                    ssh -n $x "cp -r /tmp/$PBS_JOBID/hadoop-logs $PBS_O_WORKDIR/$PBS_JOBID" 
                done
            fi
            for x in $(cat $PBS_NODEFILE | sort -u); do
                ssh -n $x "rm -rf /tmp/$PBS_JOBID/*" &
            done
        wait
        echo $SRCDIR/runall $SBMT_RUNALL_ARGS | mail -s"job $1" $USER 
        fi
        set -e
    }

    #trap "cleanup failure" EXIT INT QUIT TERM
    #trap "cleanup success" 0
    set -e
fi

