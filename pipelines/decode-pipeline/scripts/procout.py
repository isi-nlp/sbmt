#!/usr/bin/env python

import subprocess, sys, os, threading, itertools, time, collections, traceback

class FiFo:
    def __init__(self):
        self.queue_ = collections.deque()
        self.lock_ = threading.Condition()
        self.close_request_ = False
    def __iter__(self):
        return self
    def next(self):
        return self.__next__()
    def __next__(self):
        #print >> sys.stderr, 'someone wants something from FiFo'
        with self.lock_:
            while len(self.queue_) == 0 and not self.close_request_:
                #print >> sys.stderr, 'FiFo waits'
                self.lock_.wait()
                #print >> sys.stderr, 'FiFo finishes waiting'
            if len(self.queue_) == 0:
                #print >> sys.stderr, 'FiFo raises Stop'
                raise StopIteration()
            else: 
                #print >> sys.stderr, 'FiFo sends'
                return self.queue_.popleft()
    def send(self,value):
        with self.lock_:
            assert not self.close_request_
            self.queue_.append(value)
            if len(self.queue_) == 1:
                self.lock_.notify_all()
    def close(self):
        with self.lock_:
            if not self.close_request_:
                self.close_request_ = True
                self.lock_.notify_all()
class RHPipe:
    def __init__(self,args):
        self.args_ = args
        self.input_ = FiFo()
        self.subproc_ = None
        self.stopped_ = True
        self.lock_ = threading.Condition()
    
    def close(self):
        self.stop_()
        
        #print >> sys.stderr, 'RHPipe stop_ finished'
        self.thread_.join()
        #print >> sys.stderr, 'RHPipe join finished'
        try:
            self.subproc_.wait()
        except:
            pass
    def stop_(self):
        with self.lock_:
            self.stopped_ = True
            self.input_.close()
            self.lock_.notify_all()

    def start(self):
        assert self.stopped_
        #print >> sys.stderr, 'opening %s' % self.args_
        self.subproc_ = subprocess.Popen( ' '.join(self.args_)
                                        , stdout=subprocess.PIPE
                                        , stdin=subprocess.PIPE
                                        , bufsize=0
                                        , shell=True )
        #print >> sys.stderr, '%s opened' % self.args_
        self.stopped_ = False
        self.input_ = FiFo()
        #print >> sys.stderr, 'starting proc communication thread'
        self.thread_ = threading.Thread(target=RHPipe.run_, args=(self,))
        self.thread_.start()
        #print >> sys.stderr, 'proc started'
    
    def run_(self):
        x = 0
        for inp in self.input_:
            x += 1
            #
            try:
                #print >> sys.stderr, '* writing %s:%s'  % (x,inp)
                print >> self.subproc_.stdin, inp.rstrip('\n')
                #print >> sys.stderr, '* wrote %s:%s' % (x,inp)
            except Exception as e:
                print >> sys.stderr \
                       , "".join(traceback.format_exception(*sys.exc_info()))
                break
        #print >> sys.stderr, 'RHPipe run_ preparing to close'
        self.subproc_.stdin.close()
        self.stop_()
    
    def send(self,value):
        Fail = False
        with self.lock_:
            if self.stopped_:
                raise TypeError()
        try:
            self.input_.send(value)
        except:
            print >> sys.stderr, "".join(traceback.format_exception(*sys.exc_info()))
            self.stop_()
            raise TypeError()
                    
                    
    
    def __iter__(self):
        def tpl(output):
            try:
                while True:
                    #print >> sys.stderr, '** wait to get line'
                    line = output.readline().rstrip('\n')
                    if line == '':
                        break
                    #print >> sys.stderr, '** line',line
                    v = line.split('\t')
                    yield v[0],'\t'.join(v[1:])
            except Exception as e:
                print >> sys.stderr \
                       , "".join(traceback.format_exception(*sys.exc_info()))
                self.stop_()
        def group(output):
            try:
                #print >> sys.stderr, 'group yielding lines'
                for key,lines in itertools.groupby(output,lambda x : x[0]):
                    #print >> sys.stderr, 'group yielding line'
                    lst = [line[1] for line in lines]
                    #print >> sys.stderr, '*** %s' % lst
                    yield lst
            except Exception as e:
                print >> sys.stderr \
                       , "".join(traceback.format_exception(*sys.exc_info()))
            finally:
                self.stop_()
            
            #print >> sys.stderr, 'subproc should be killed now...'
        
        return iter(group(tpl(self.subproc_.stdout)))
    
if __name__ == '__main__':
    def put(input,pipe):
        try:
            for line in input:
                pipe.send(line)
        except:
            pass
        print >> sys.stderr, 'put closing pipe'
        pipe.close()
        print >> sys.stderr, 'pipe closed'

    pipe = RHPipe(sys.argv[1:])
    pipe.start()
    putter = threading.Thread(target=put,args=(sys.stdin,pipe))
    putter.start()
    try:
        #print >> sys.stderr, 'main printing pipe'
        for line in pipe:
            #print >> sys.stderr, 'main printing line'
            print line
    finally:
        putter.join()
        sys.exit(0)
         
