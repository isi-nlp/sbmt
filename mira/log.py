import sys
import monitor
import time
import traceback
import datetime

prefix = '' # for parallel.py to set so log message from clients have [mpi.rank] before them
level = 1
file = sys.stderr

def writeln(s=""):
    if s[-1] != '\n':
        s += '\n'
    write(s)

tstart=time.time()

def strexcept(trace=True):
    e=sys.exc_info()
    return "".join(traceback.format_exception_only(e[0],e[1])+(traceback.format_exception(*e) if trace else []))

def dec(t):
    return "%.02f" % t

def write(s):
    s = "t=%s wt=%s %s" % (dec(monitor.cpu()), dec(time.time()-tstart), s)
    for l in s.splitlines(True):
        file.write(prefix + l)
    file.flush()

def datetoday():
    return str(datetime.datetime.today())

def str2date(s):
    return datetime.datetime.strptime(s,'%Y-%m-%d %H:%M:%S.%f')

if __name__ == "__main__":
    prefix='[prefix] '
    write("line1\nline2\n")
    writeln("line1\nline2\n")
    for i in range(1,100):
        writeln("i=%s"%i)
