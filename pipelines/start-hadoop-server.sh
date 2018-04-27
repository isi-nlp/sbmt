# call as: ". start-hadoop-server.sh [hadoop-logs-dir]"
# so that environment variables are correctly exported
export HADOOP_PREFIX=/usr/usc/hadoop/default
if [ "x" = "x$PIPELINE_HADOOP" ]; then
    PIPELINE_HADOOP=1
    export PIPELINE_HADOOP
    export PIPELINE_HADOOP_TMPFILE=$(mktemp -t tmp.XXXXXXXX)
    HCD=$SCRATCHDIR
    if test -e $HADOOP_PREFIX/setup.sh; then
      . $HADOOP_PREFIX/setup.sh
    fi
    export LANG=C
    export LC_ALL=C
fi

function cleanuphadoop {
    export HADOOP_CONF_DIR=$SCRATCHDIR/hadoop/conf
    set +e
    echo "in hadoop cleanup"
    /usr/usc/hadoop/default/bin/stop-all.sh
    set -e
}

trap "cleanuphadoop failure" EXIT INT QUIT TERM
trap "cleanuphadoop success" 0

