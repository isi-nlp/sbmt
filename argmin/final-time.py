#!/usr/bin/python
#
#  final-time.py < logfile
#
#  Go over argmin output and extract the final usec when the last
#  intermediate 1-best output was found.
#
#  $Id: final-time.py 1327 2006-10-13 21:07:57Z jturian $
#

import sys, re

last = (None, None)
best_re = re.compile("\(intermediate\) \[\S+ wall sec, (\S+) user sec, \S+ system sec, \S+ major page faults\] .*sent=([0-9]+).*foreign-length=([0-9]+)")
for l in sys.stdin:
	m = best_re.search(l)
	if m:
		usec = float(m.group(1))
		sent = int(m.group(2))
		len = int(m.group(3))
		if last[0] == sent:
			assert last[2] <= usec
		elif last[0] != None:
			print last[1], last[2]
		last = (sent, len, usec)
