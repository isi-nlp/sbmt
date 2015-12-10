#!/usr/bin/env python
import re
import os
import os.path
import sys
import yaml
import itertools
import hadoop
import string
import subprocess, shlex
import argparse

class PTemplate(string.Template):
    pattern = r"""
       \$(?:
          (?P<escaped>\$) |   # Escape sequence of two delimiters
          (?P<named>[_a-zA-Z][-_a-zA-Z0-9]*)      |   # delimiter and a Python identifier
          [{\(](?P<braced>[_a-zA-Z0-9][-_a-zA-Z0-9]*(\.[_a-zA-Z0-9][-_a-zA-Z0-9]*)*)[}\)]   |   # delimiter and a braced identifier
          (?P<invalid>)              # Other ill-formed delimiter exprs
        )
        """

stages_ = set(( 'ghkm', 'prefeature', 'feature', 'extract', 'training', 'extras', 'dictionary'
              , 'post-process-extras', 'global-extras', 'lattice', 'decode', 'inline-extras' ))
              


class step:
    def __init__(self,name,output,execute,stage,file='',suffix='',binarize=None):
        self.name = name
        self.output = output
        self.execute = execute
        self.stage = stage
        self.file = file
        self.suffix = suffix
        #if isinstance(binarize,str):
        #    self.binarize = [binarize]
        #elif isinstance(binarize,list):
        #    self.binarize = binarize
        #else:
        #    self.binarize = []
        if stage not in stages_:
            raise 'error: stage must be member of set' + str(stages_)
    
    def __repr__(self):
        t = (self.name,self.output,self.execute,self.stage,self.file,self.suffix)
        return repr(t)
    
    def realfile_(self,hadoop,outdir,tmpdir):
        m = re.search(r'\.(gz|bz2)',self.file)
        if m:
            if m.group(1) == 'gz':
                hadoop.syscall('gunzip -c %s > %s' % (self.file,os.path.join(tmpdir,'ttable')))
            else:
                hadoop.syscall('bunzip2 -c %s > %s' % (self.file,os.path.join(tmpdir,'ttable')))
            return os.path.join(tmpdir,'ttable')
        else:
            return self.file
    
    def cleanfile_(self,file,hadoop,outdir,tmpdir):
        if file != self.file:
            hadoop.syscall('rm -rf %s' % os.path.join(tmpdir,'ttable'))
        
    def output_filename(self):
        return [os.path.basename(o) + self.suffix for o in self.output]
    
    def executable(self):
        return PTemplate(self.execute).safe_substitute({'file':self.file,'suffix':self.suffix})
    
    def run(self,hadoop,outdir,tmpdir):
        bname = []
        output_suffix = [o + self.suffix for o in self.output]
        if len(self.output) > 0:
            bname = [os.path.basename(o) for o in self.output]
        # bname+suffix and output_suffix frequently used together
        osbn = zip(output_suffix, [b + self.suffix for b in bname])
        if len(self.output) > 0 and all([os.path.exists(o) for o in output_suffix]):
            for os, bn in osbn:
                hadoop.put(os, bn)
            return
        else:
            file = self.realfile_(hadoop,outdir,tmpdir)
            execute = PTemplate(self.execute).safe_substitute({'file':file,'suffix':self.suffix})
            hadoop.syscall(execute)
            if len(self.output) > 0:
                for os, bn in osbn:
                    if self.stage == 'training':
                        hadoop.getmerge(bn, os)
                    else:
                        hadoop.get(bn, os)
            self.cleanfile_(file,hadoop,outdir,tmpdir)

class Step:
    def __init__(self,name,sect,mp):
        self.name = name
        self.stage = sect['stage']
        if 'file' in mp:
            self.file = mp['file']
        if 'rank' in sect:
            self.rank = int(sect['rank'])
        else:
            self.rank = 0

        if self.stage == 'dictionary':
            output = ['dict.' + name]
        else:
            output = ['part.' + name]
        if 'output' in sect:
            output = sect['output']
            # convert scalar entries to singleton lists
            if not isinstance(output, (list, tuple)):
                output = [output]
        if 'suffix' in mp:
            output = [o + mp['suffix'] for o in output]
        output = [os.path.join(mp['tmpdir'], o) for o in output]
        self.output = output
        if 'exec' in sect:
            self.execute = PTemplate(sect['exec']).safe_substitute(mp)
        else:
            self.execute = ''
        #if 'binarize' in sect:
        #    binarize = sect['binarize']
        #    if isinstance(binarize,str):
        #        self.binarize = [binarize]
        #    elif isinstance(binarize,list):
        #        self.binarize = binarize
        #else:
        #    self.binarize = []
        if self.stage not in stages_:
            raise 'error: stage must be member of set' + str(stages_)
    
    def __repr__(self):
        t = (self.output,self.execute,self.stage)
        return repr(t)
    
    def output_filename(self):
        return [os.path.basename(o) for o in self.output]
    
    def executable(self):
        return self.execute
    

    def run(self,hadoop):
        bname = []
        output = self.output
        if len(output) > 0:
            bname = [os.path.basename(o) for o in output]
        # bname and output frequently used together
        opbn = zip(output, bname)
        if len(output) > 0 and all([os.path.exists(o) for o in output]):
            for op, bn in opbn:
                hadoop.put(op, bn)
            return
        else:
            hadoop.syscall(self.execute)
            if len(output) > 0:
                for op, bn in opbn:
                    if self.stage == 'training':
                        hadoop.getmerge(bn, op)
                    else:
                        hadoop.get(bn, op)

class DecoderStep(Step):
    def __repr__(self):
        p = ''
        if 'info' in self.__dict__:
            p = self.info
        t = (self.name,self.options,p,self.stage)
        return repr(t)

    def __init__(self,name,sect,mp):
        Step.__init__(self,name,sect,mp)
        ins = ''
        if 'options' in sect:
            ins = ' '.join(PTemplate('--%s %s' % (k,v)).safe_substitute(mp) for (k,v) in sect['options'].iteritems())
        if 'options-exec' in sect:
            oexec = PTemplate(sect['options-exec']).safe_substitute(mp)
            proc = subprocess.Popen(shlex.split(oexec),stdout=subprocess.PIPE)
            (outstr,ignore) = proc.communicate()
            if proc.returncode != 0:
                raise subprocess.CalledProcessError(proc.returncode,oexec)
            if ins == '':
                ins = outstr.strip()
            else:
                ins = ins + ' ' + outstr.strip()
        self.options = ins
        if 'info' in sect:
            self.info = sect['info']
        else:
            self.info = ''

def make_step(name,sect,mp):
    if sect['stage'] == 'decode':
        return DecoderStep(name,sect,mp)
    else:
        return Step(name,sect,mp)

def tomerge(sect):
    if 'merge' not in sect:
        return False
    a = sect['merge']
    if isinstance(a,bool):
        return a
    if isinstance(a,int):
        return bool(int)
    return False

def make_steps(tmpdir,sect,name,cfg,dirs,suffix='',hadoopargs='',configfiles=''):
    def matching_files(dirs,suffix):
        filesuf = []
        for dir in dirs:
            #print >> sys.stderr, "%%% matching",dir,"to",sect['extension']
            splt = dir.split(':')
            if len(splt) > 1 and splt[0] == sect['extension']:
                filesuf.append((':'.join(splt[1:]),suffix))
            elif re.search(r'\.%s(\..*)?$' % sect['extension'],dir):
                filesuf.append((dir,suffix))
            elif os.path.isdir(dir):
                for f in os.listdir(dir):
                    f = os.path.join(dir,f)
                    s = suffix
                    if os.path.isdir(f):
                        s = suffix + '.' + os.path.basename(f)
                    filesuf += matching_files([f],s)
        return filesuf
    sout = []
    mp = { 'curr': os.path.dirname(sys.argv[0]) 
         , 'hadoop': hadoopargs 
         , 'config': configfiles
         , 'tmpdir': tmpdir
         , 'suffix': suffix }
    
    if 'extension' not in sect:
        sout.append(make_step(name,sect,mp))
    else:
        if tomerge(sect):
            filestr = ' '.join(f for f,s in matching_files(dirs,suffix))
            mp['suffix'] = ''
            mp['file'] = filestr
            sout.append(make_step(name,sect,mp))
        else:
            sss = []
            for f,s in matching_files(dirs,suffix):
                m = dict(mp.iteritems())
                m['file'] = f
                m['suffix'] = ''#s
                sss.append(make_step(name,sect,m))
            if len(sss) > 0:
                sout.append(sss[-1])
    return sout

def active(sect):
    if 'active' not in sect:
        return True
    a = sect['active']
    if isinstance(a,bool):
        return a
    if isinstance(a,int):
        return bool(int)
    if isinstance(a,str):
        if a == 'False' or a == 'false' or a == 'FALSE' or a == '0':
            return False
        if a == 'True' or a == 'true' or a == 'TRUE':
            return True
        try:
            x = int(a)
            return x != 0
        except:
            pass    
    return False

def steps(dtm):
    cfg = dtm.config
    dirs = dtm.modeldir
    tmpdir = dtm.tmpdir
    outdir = dtm.outdir
    sout = []
    flist = cfg['rule-extraction']['features']
    for name,sect in flist.iteritems():
        numsteps = 0
        if active(sect):
            
            sect['active'] = True
            if 'stages' in sect:
                sout_ = []
                keep = True
                if isinstance(sect['stages'],list):
                    stagelist = [s for s in sect['stages']]
                elif isinstance(sect['stages'],dict):
                    stagelist = [s for (t,s) in sect['stages'].iteritems()]
                for s in stagelist:
                    if s['stage'] == 'training':
                        dir = outdir
                    else:
                        dir = tmpdir
                    sout__ = make_steps(dir, s, name, cfg, dirs)
                    
                    if len(sout__) == 0:
                        keep = False
                        sect['active'] = False
                        break
                    else:
                        sout_ += sout__
                if keep:
                    numsteps = len(sout_)
                    sout += sout_
            else:
                try:
                    if sect['stage'] == 'training':
                        dir = outdir
                    else:
                        dir = tmpdir
                except:
                    print >> sys.stderr, "\n\noops: %s:" % name, sect, "does not appear to be a valid rule-extraction step\n\n"
                    continue
                sout_ = make_steps(dir,sect,name,cfg,dirs)
                if len(sout_) == 0:
                    sect['active'] = False
                else:
                    numsteps = len(sout_)
                    sout += sout_
        else:
            sect['active'] = False
        #print >> sys.stderr, "name", name, "active:", sect['active'], "steps:", numsteps 
    sout.sort(lambda a,b : cmp(a.rank,b.rank))
    #print >> sys.stderr, "##### steps #####"
    #for ss in sout:
    #    print >> sys.stderr, ss
    #print >> sys.stderr, "##### /steps ####"
    return sout

def merge_uniq_lists(user, default):
    ret = default[:]
    dset = set(default)
    for x in user:
        if x not in dset:
            dset.add(x)
            ret.append(x)
    return ret

def merge_configs(user, default):
    if user is None:
        user = {} # an empty field in the config is assumed to be a dictionary
    if default is None:
        default = {}
    if isinstance(user,dict) and isinstance(default,dict):
        for k,v in default.iteritems():
            if k not in user:
                user[k] = v
            elif k == 'model-directories':
                user[k] = merge_uniq_lists(user[k],v)
            else:
                user[k] = merge_configs(user[k],v)
    return user

def expand(config,mp):
    def join(prefix,suffix):
        if prefix == '':
            return str(suffix)
        if suffix == '':
            return str(prefix)
        return str(prefix) + '.' + str(suffix)
    
    def flatten_vars(config,mp,prefix=''):
        if isinstance(config,list):
            for x,v in enumerate(config):
                flatten_vars(v,mp,join(prefix,x))
        elif isinstance(config,dict):
            for k,v in config.iteritems():
                flatten_vars(v,mp,join(prefix,k))
        elif prefix != '' and config is not None:
            mp[prefix] = config
    
    def substitute(config,mp):
        if isinstance(config,str):
            c = PTemplate(config).safe_substitute(mp)
            if c != config:
                return (True,yaml.load(c))
            else:
                return (False,c)
        elif isinstance(config,dict):
            b = False
            for k,v in config.iteritems():
                bb,cc = substitute(v,mp)
                b = b or bb
                config[k] = cc
            return b,config
        elif isinstance(config,list):
            b = False
            for k,v in enumerate(config):
                bb,cc = substitute(v,mp)
                b = b or bb
                config[k] = cc
            return b,config
        else:
            return False,config
    subbed = True
    while subbed:
        nmp = dict(mpp for mpp in mp.iteritems() if mpp[1] is not None)
        flatten_vars(config,nmp)
        subbed,config = substitute(config,nmp)
    return config
            
def load_config_raw(filestrs,vars={},default=None,omit_sys_default=False):
    if omit_sys_default == False:
        f = os.path.join(os.path.dirname(__file__),'default.sbmt.cfg')
        vars=dict((v for v in vars.iteritems())) #clone it
        c = f
        conf = yaml.load(open(c))
        conffiles_list = [c]
    else:
        conf = {}
        conffiles_list = []
    
    if filestrs:
        if isinstance(filestrs,list):
            filestrs = ':'.join(filestrs)
        files = [ x for x in itertools.ifilter(lambda x : x != '', filestrs.split(':')) ]
    else:
        files = []
    if default is not None:
        #print >> sys.stderr, 'default',default,'loading'
        files = default.split(':') + files
    #print >> sys.stderr,'config files:',files
    for x in files:
        x = os.path.abspath(x)
        conf = merge_configs(yaml.load(open(x)),conf)
        conffiles_list.append(x)
    
    return conf,':'.join(conffiles_list)

def load_config(filestrs,vars={},default=None,write=None):
    conf,conffiles = load_config_raw(filestrs,vars,default)
    vars['curr'] = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
    if write is None:
        vars['config'] = conffiles
    else:
        vars['config'] = write
    conf = expand(conf,vars)
    return conf

def maybe_abspath(f):
    if os.path.exists(f):
        return os.path.abspath(f)
    else:
        return f

class store_abspath(argparse.Action):    
    def __call__(self,parser, namespace, values,option_string=None):
        if isinstance(values,list):
            setattr(namespace,self.dest,[maybe_abspath(v) for v in values])
        else:
            setattr(namespace,self.dest,maybe_abspath(values))

def make_hadoop(config):
    return hadoop.Hadoop( echo=config['rule-extraction']['hadoop']['echo']
                        , serial=config['rule-extraction']['hadoop']['serial']
                        , home=config['rule-extraction']['hadoop']['home']
                        )

def execute(d,cmd,*args,**mp):
    cmd = string.Template(cmd)
    cmd = cmd.safe_substitute(mp)
    d.hadoop.syscall(cmd)

def parse_args( parser
              , cmdline = sys.argv[1:]
              , config = None
              , default = None
              , modeldir = False 
              , write = None
              , overwrite = True ):
              
    rootdir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    scriptdir = os.path.join(rootdir,'scripts')
    if modeldir:
        parser.add_argument( '-m', '--modeldir' 
                           , nargs='+'
                           , default=[]
                           , dest='modeldir' 
                           , help='list of directories/files/auxilliary commands that will be ' +
                                  'pattern matched against \'extension\' fields in the command line' 
                           , action=store_abspath
                           )
    if config is None:
        parser.add_argument( '-c','--config'
                           , dest='config'
                           , default=[]
                           , nargs='+'
                           , help='colon or space separated list of config-files. ' + 
                                  'later files are merged into earlier ones, overriding conflicts.  ' +
                                  'config-files are in YAML format. %s/default.sbmt.cfg is loaded first.' % os.path.dirname(__file__) )
    parser.add_argument( '-o', '--output'
                       , dest='outdir'
                       , default='.'
                       , help='output directory. should be the same for run* jobs.  ' +
                              'defaults to "."' )
    parser.add_argument( '-t', '--tmpdir' 
                       , dest='tmpdir' 
                       , default='$outdir/tmp' 
                       , help='temporary files stored here.  should be the same for all run* jobs.  ' + 
                              'defaults to $outdir/tmp' )
    d = parser.parse_args(cmdline)
    d.tmpdir = PTemplate(d.tmpdir).safe_substitute({'outdir':d.outdir})
    d.tmpdir = os.path.abspath(d.tmpdir)
    d.outdir = os.path.abspath(d.outdir)
    vars = dict(d.__dict__)
    vars['curr'] = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
    if config is not None:
        if isinstance(config,list):
            config = ':'.join(config)
        d.config = PTemplate(config).safe_substitute(vars)
    if default is not None:
            tmpcfg,ignore = load_config_raw(d.config)
            default = PTemplate(default).safe_substitute(tmpcfg) # in case of config-file bootstrap
            default = PTemplate(default).safe_substitute(vars)
            print >> sys.stderr, "### default=%s ###" % default
    if write is not None:
        write = PTemplate(write).safe_substitute(vars)
        if (not overwrite) and os.path.exists(write):
            print >> sys.stderr, "### config exists: no overwrite ###"
            d.config =  write
            default = None
            write = None
    config = load_config(d.config,vars=vars,default=default,write=write)
    config_raw,ignore = load_config_raw(d.config,vars=vars,default=default)
    d.config = config
    d.config_raw = config_raw
    
    if not modeldir:
        d.modeldir = []
    if 'model-directories' in d.config:
        oldmodels = d.config['model-directories'][:]
    else:
        oldmodels = []
    
    def find_(x,seq):
        for y in seq:
            if x == y:
                return True
        return False
    # its important to add to the modeldirs in such a ways as to
    #   - preserve uniqueness
    #   - preserve order of entry
    # hopefully the number of models never exceeds a few dozen...
    newmodels = []
    for x in itertools.chain(oldmodels,d.modeldir):
        if not find_(x,newmodels):
            newmodels.append(x)
    
    d.modeldir = newmodels
    
    d.config_raw['model-directories'] = yaml.load(yaml.dump(d.modeldir))
    for k,v in [ p for p in d.__dict__.iteritems() ]:
        if k not in set(['outdir','tmpdir','config','modeldir','config_raw']):
            d.config_raw[k] = yaml.load(yaml.dump(v))
            d.config[k] = yaml.load(yaml.dump(v))
        if (write is not None) and k not in set(['outdir','tmpdir','config','config_raw','modeldir']):
            delattr(d,k)
            
    d.config_files = vars['config']
    d.hadoop = make_hadoop(d.config)
    d.scriptdir = scriptdir
    d.rootdir = rootdir
    
    # allows variables to expand based on whether step is active at runtime, since steps() will now explicitly set the active flag
    # to True/False
    stepss = steps(d)
    d.config = expand(d.config,vars)
    
    if write is not None:
        if not overwrite and os.path.exists(write):
            pass
        else:
            execute(d,'mkdir -p '+d.outdir)
            execute(d,'mkdir -p '+d.tmpdir)
            write = PTemplate(write).safe_substitute(vars)
            ocfg = yaml.dump(d.config_raw)
            focfg = open(os.path.join(d.outdir,write),'w')
            #focfgn = open(os.path.join(d.outdir,write) +".expanded", 'w')
            #focfgs = open(os.path.join(d.outdir,write) + ".steps", 'w')
            print >> focfg, ocfg
            #print >> focfgn, yaml.dump(d.config)
            print >> sys.stderr, "steps:"
            for stp in stepss: 
                print >> sys.stderr, yaml.dump(stp.__dict__)
            print >> sys.stderr, '\n'
    return d
        
