import sys, collections, heapq, random, log
import socket
from mpi4py import MPI

class Die(object):
    pass

def pmap_master(input, verbose=False, tag=0, shuffle=False, hook=None):
    if verbose:
        log.writeln("beginning")

    input = enumerate(input)
    if shuffle:
        input = list(input)
        random.shuffle(input)
        input = iter(input)
    output = []
    flushed = 0

    idle = set(slaves)
    busy = {}

    while len(idle)+len(busy) > 0:
        while len(idle) > 0:
            node = idle.pop()
            try:
                (i,line) = input.next()
            except StopIteration:
                if verbose:
                    log.writeln("send to node %s: Die" % node)
                world.send(Die(), dest=node, tag=tag)
            else:
                if verbose:
                    log.writeln("send to node %s: %s" % (node, line))
                world.send(line, dest=node, tag=tag)
                if verbose:
                    log.writeln("add node %s to busy list" % (node,))
                busy[node] = i

        if len(busy) > 0:
            status = MPI.Status()
            line = world.recv(source=MPI.ANY_SOURCE, tag=tag, status=status)
            node = status.source
            if verbose:
                log.writeln("received from %s: %s" % (node, line))

            i = busy[node]
            del busy[node]
            heapq.heappush(output, (i, line))

            while len(output) > 0 and output[0][0] == flushed:
                (i, line) = heapq.heappop(output)
                yield line
                flushed += 1

            if hook:
                hook(node)

            if verbose:
                log.writeln("adding %s to idle list" % (node,))
            idle.add(node)
    if verbose:
        log.writeln("finished")

def pmap_slave(f, verbose=False, tag=0, hook=None):
    while True:
        if verbose:
            log.writeln("ready to receive data")
        msg = world.recv(source=master, tag=tag)

        if isinstance(msg, Die):
            if verbose:
                log.writeln("received from master: Die")
            break

        if verbose:
            log.writeln("received from master: %s" % (msg,))
        msg = f(msg)
        if verbose:
            log.writeln("sending output to master")
        world.send(msg, dest=master, tag=tag)
        if hook:
            hook()

    if verbose:
        log.writeln("finished")
    return ()

def pmap(f, input, verbose=False, tag=0, shuffle=False, master_hook=None, slave_hook=None):
    if rank == master:
        return pmap_master(input, verbose, tag, shuffle=shuffle, hook=master_hook)
    else:
        return pmap_slave(f, verbose, tag, hook=slave_hook)

world = MPI.COMM_WORLD
rank = world.Get_rank()
size = world.Get_size()
master = 0
slaves = set(r for r in xrange(size) if r != master)
log.prefix = '[%s] ' % rank
log.writeln("host %s rank %s\n" % (socket.gethostname(), rank))

if __name__ == "__main__":
    for line in pmap(lambda x: x, file("/home/nlg-01/chiangd/hiero-mira2/parallel.py"), verbose=True):
        sys.stdout.write(line)
