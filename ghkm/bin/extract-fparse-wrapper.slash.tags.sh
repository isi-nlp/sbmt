#!/bin/sh

# Some arguments go to fixup-wsd, the rest go to extract. Assume no
# spaces in arguments.

FIXUP_ARGS=
EXTRACT_ARGS=

while [ $# -gt 0 ]; do
    if [ "$1" = "--keep-align" -o "$1" = "--strip-align" ]; then
	FIXUP_ARGS="$FIXUP_ARGS $1"
    else
	EXTRACT_ARGS="$EXTRACT_ARGS $1"
    fi
    shift
done

  PYTHONPATH=/home/nlg-01/chiangd/sbmt-hadoop /usr/usc/python/default/bin/python /home/nlg-02/pust/ghkm/bin/extract-fparse.slash.tags.py $EXTRACT_ARGS |
  /home/nlg-03/mt-apps/grrr/fixup-wsd $FIXUP_ARGS 



