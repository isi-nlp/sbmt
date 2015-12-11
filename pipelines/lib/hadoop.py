#!/usr/bin/env python

import subprocess, sys, optparse, os, glob, re

class Hadoop:
    def syscall(self,command,echo=True):
        if echo:
      
            print >> sys.stderr, command, '(%s=%s)' % ('HADOOP_CONF_DIR',os.getenv('HADOOP_CONF_DIR'))
        if not self.echo: 
            subprocess.check_call( 'set -e; set -o pipefail; '+ command
                                 , shell=True
                                 , executable='/bin/bash' )
    
    def hadoop_file_exists(self,f):
        self.fullstart()
        try:
            self.syscall("%s fs -test -e %s >&2" % (self.hadoop_bin,f))
            return True
        except:
            return False
    
    def hadoop_is_directory(self,f):
        self.fullstart()
        try:
            if not self.hadoop_file_exists(f):
                return False
            self.syscall("%s fs -test -d %s 1>&2" % (self.hadoop_bin,f))
            return True
        except:
            return False
    
    def local_file_exists(self,f):
        try:
            self.syscall("test -e %s" % f)
            return True
        except:
            return False
    
    def local_is_directory(self,f):
        try:
            self.syscall("test -e %s" % f)
            self.syscall("test -d %s" % f)
            return True
        except:
            return False
    
    def is_directory(self,f):
        if not self.serial:
            return self.hadoop_is_directory(f)
        else:
            return self.local_is_directory('%s/%s' % (self.tmpdir,f))
    
    def file_exists(self,f):
        if not self.serial:
            return self.hadoop_file_exists(f)
        else:
            return self.local_file_exists('%s/%s' % (self.tmpdir,f))

    def checkstart(self):
        confdir = ''
        if os.getenv('PIPELINE_HADOOP_TMPFILE'):
            with open(os.getenv('PIPELINE_HADOOP_TMPFILE')) as tempfile:
                for line in tempfile:
                    confdir = line.strip()
            if confdir != '':
                os.environ['HADOOP_CONF_DIR'] = confdir
        return confdir   
    
    def writestart(self):
        if os.getenv('PIPELINE_HADOOP_TMPFILE'):
            with open(os.getenv('PIPELINE_HADOOP_TMPFILE'),'w') as tempfile:
                print >> tempfile, os.getenv('HADOOP_CONF_DIR')

    def fullstart(self):
        confdir = self.checkstart()
        if confdir == '':
            self.start()
        else:
            os.environ['HADOOP_CONF_DIR'] = confdir
            
    def start(self,mappers=0,reducers=0,compress=True):
        self.checkstart()
        opts = ''
        mpn = int(mappers)
        rpn = int(reducers)
        if compress:
            opts += '-z '
        if mpn > 0:
            opts += '-m %s ' % mpn
        if rpn > 0:
            opts += '-r %s ' % rpn
        if os.getenv('HADOOP_CONF_DIR','') != '':
            opts += '-R '
        else:
            os.environ['HADOOP_CONF_DIR'] = '/scratch/hadoop/conf'
        if not self.serial:
            self.syscall('/home/nlg-02/pust/bin/pbs_hadoop_nologs2.py %s >&2' % opts)
        else:
            self.syscall('mkdir -p /tmp/serial-hadoop')
        self.writestart()

    def hadoop_mapreduce( self
                        , mapper
                        , reducer
                        , combiner
                        , input
                        , output
                        , compress
                        , partitionkeys
                        , sortkeys
                        , options ):
        self.fullstart()
        if isinstance(input,list):
            input = ' '.join('-input "%s"' % inp for inp in input)
        else:
            input = '-input "%s"' % input
        if self.file_exists(output):
            self.remove(output)
        if combiner is not None:
            combiner = '-combiner "%s"' % combiner
        else:
            combiner = ''
        pythonpath = os.environ.get('PYTHONPATH',os.path.abspath(os.path.dirname(__file__)))

        mapred = '%(bin)s jar %(jar)s %(options)s %(combiner)s -mapper "%(mapper)s" ' + \
                 '-reducer "%(reducer)s" ' + \
                 '%(input)s -output "%(output)s"'

        options = re.sub("-jobconf", "-D", options)
        if isinstance(partitionkeys,int) or isinstance(sortkeys,int):
            if partitionkeys is None:
                partitionkeys = sortkeys
            if sortkeys is None:
                sortkeys = partitionkeys
            options += ' -D stream.num.map.output.key.fields=%s ' % str(sortkeys)
            options += ' -D num.key.fields.for.partition=%s ' % str(partitionkeys)
        if isinstance(compress,bool):
            if compress:
                options += ' -D mapred.output.compress=true '
            else:
                options += ' -D mapred.output.compress=false '

        cmdsent = mapred % { 'bin'      : self.hadoop_bin
                           , 'jar'      : self.hadoop_jar
                           , 'options'  : options
                           , 'mapper'   : mapper
                           , 'reducer'  : reducer
                           , 'combiner' : combiner
                           , 'input'    : input
                           , 'output'   : output }
        cmdsent += ' -cmdenv PYTHONPATH=%s' % pythonpath
        if isinstance(partitionkeys,int):
             cmdsent += ' -partitioner org.apache.hadoop.mapred.lib.KeyFieldBasedPartitioner '
        self.syscall('( ' + cmdsent + ' ) >&2')

    def hadoop_remove(self,file):
        self.fullstart()
        if self.hadoop_file_exists(file):
            self.syscall("%s fs -rmr %s >&2" % (self.hadoop_bin,file))
    
    def hadoop_mkdir(self,f):
        self.fullstart()
        if not self.hadoop_is_directory(f):
            self.syscall("%s fs -mkdir %s >&2" % (self.hadoop_bin,f))
    
    def hadoop_move(self,f1, f2):
        self.fullstart()
        self.syscall("%s fs -mv %s %s >&2" % (self.hadoop_bin,f1,f2))

    def hadoop_get(self,file,dest):
        self.fullstart()
        self.syscall("%s fs -get %s %s >&2" % (self.hadoop_bin,file,dest + '.tmp'))
        self.syscall("mv %s %s >&2" % (dest + '.tmp', dest))

    def hadoop_getmerge(self,file,dest):
        self.fullstart()
        self.syscall("%s fs -getmerge %s %s >&2" % (self.hadoop_bin,file,dest + '.tmp'))
        self.syscall("mv %s %s >&2" % (dest + '.tmp', dest))
    
    def hadoop_put(self,file,dest):
        self.fullstart()
        self.syscall("%s fs -put %s %s >&2" % (self.hadoop_bin,file,dest))
    
    def serial_mapreduce( self
                        , mapper
                        , reducer
                        , combiner
                        , input
                        , output
                        , compress   
                        , partitionkeys
                        , sortkeys
                        , options ):
        if isinstance(compress,bool) and compress:
            output = " | gzip > %s/%s" % (self.tmpdir,output)
        else:
            output = " > %s/%s" % (self.tmpdir,output)
        if isinstance(input,list):
            input = ' '.join('%s/%s' % (self.tmpdir,inp) for inp in input)
        else:
            input = '%s/%s' % (self.tmpdir,input)
        if reducer == 'NONE':
            cmdstr = "cat %s | %s %s" % (input,mapper,output)
        else:
            cmdstr = "cat %s | %s | sort | %s %s" % (input, mapper, reducer, output)
        self.syscall(cmdstr)    
    
    def serial_get(self,f,d):
        self.syscall("cp %s/%s %s" % (self.tmpdir,f,d))
    
    def serial_getmerge(self,f,d):
        self.syscall("cp %s/%s %s" % (self.tmpdir,f,d))
        
    def serial_mkdir(self,f):
        self.syscall("mkdir -p %s/%s" % (self.tmpdir,f))
    
    def serial_put(self,f,d):
        self.syscall("cp %s %s/%s" % (f,self.tmpdir,d))

    def serial_remove(self,file):
        self.syscall("rm -rf %s/%s" % (self.tmpdir,file))

    def serial_move(self,f1, f2):
        self.syscall("mv %s/%s %s/%s" % (self.tmpdir,f1,self.tmpdir,f2))

    def __init__(self,echo=False, serial=False,home=''):
        self.tmpdir = os.getenv('TMPDIR','/tmp')
        if not serial:
            self.hadoop_home = os.getenv('HADOOP_PREFIX',home)
            self.hadoop_bin = os.path.join(self.hadoop_home, 'bin', 'hadoop')
            try:
                self.hadoop_jar = glob.glob(os.path.join(self.hadoop_home, 'contrib','streaming','hadoop-streaming*.jar'))[0]
            except:
                serial = True
                pass
        self.echo = echo
        self.serial = serial
        if serial:
            self.tmpdir = os.path.join(self.tmpdir,'serial-hadoop')
            self.syscall('mkdir -p %s' % self.tmpdir)

    def mkdir(self,d):
        if self.serial:
            self.serial_mkdir(d)
        else:
            self.hadoop_mkdir(d)
    
    def get(self,f,d):
        if self.local_is_directory(d):
            d = os.path.join(d,os.path.basename(f))
        if self.local_file_exists(d):
            self.syscall("rm -rf %s" % d)
        if self.serial:
            self.serial_get(f,d)
        else:
            self.hadoop_get(f,d)

    def getmerge(self,f,d):
        if self.local_is_directory(d):
            d = os.path.join(d,os.path.basename(f))
        if self.local_file_exists(d):
            self.syscall("rm -rf %s" % d)
        if self.serial:
            self.serial_getmerge(f,d)
        else:
            self.hadoop_getmerge(f,d)

    def put(self,f,d):
        if self.file_exists(d):
            self.remove(d)
        if self.serial:
            self.serial_put(f,d)
        else:
            self.hadoop_put(f,d)
    
    def remove(self,file):
        if self.serial:
            self.serial_remove(file)
        else:
            self.hadoop_remove(file)

    def move(self,f1,f2):
        if self.serial:
            self.serial_move(f1,f2)
        else:
            self.hadoop_move(f1,f2)

    def mapreduce( self
                 , input
                 , output
                 , mapper='cat'
                 , reducer='NONE'
                 , combiner=None
                 , compress=None
                 , partitionkeys=None
                 , sortkeys=None
                 , options=''):
        if self.serial:
            call = self.serial_mapreduce
        else:
            call = self.hadoop_mapreduce
        call( mapper=mapper
            , reducer=reducer
            , combiner=combiner
            , input=input
            , output=output
            , compress=compress
            , partitionkeys=partitionkeys
            , sortkeys=sortkeys
            , options=options )


