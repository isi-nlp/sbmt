#!/usr/bin/env python
import os
import cfg
import sys
import stat

def write_instruction_script(d, logfile=True):
    instructfile = os.path.join(d.tmpdir,'instructions')
    instructscript = open(instructfile,'w')
    print >> instructscript, '#!/usr/bin/env bash'
    print >> instructscript, 'HOST=`hostname`'
    if logfile:
        logdir = os.path.join(d.outdir,'logs')
        cfg.execute(d,'mkdir -p %s' % logdir)
        print >> instructscript, 'LOG=%s/instruction-log.$HOST-$$.log' % logdir
    print >> instructscript, os.path.join(d.scriptdir,'decoder-instructions'), d.config['rules'] , '-c %s \\' % d.config_files
    if logfile:
        print >> instructscript, ' 2> $LOG \\'
        #print >> instructscript, '  2> >(gzip > $LOG.gz) \\'
    print >> instructscript, '\n\n'
    if logfile:
        print >> instructscript, 'gzip $LOG \n\n'
    instructscript.close()
    os.chmod(instructfile, stat.S_IRWXU | os.stat(instructfile)[stat.ST_MODE])
    return instructfile

def write_script(d, stage, weightstring=None, logfile=True, include_instruction_pipe=False, decodefile=None):
    
    if stage not in set(['nbest','forest']):
        raise Exception
    if logfile:
        logdir = os.path.join(d.outdir,'logs')
        cfg.execute(d,'mkdir -p %s' % logdir)
    ruledir = d.config['rules']
    if decodefile is None:
        decodefile = os.path.join(d.tmpdir,'decoder')
    decodescript = open(decodefile,'w')
    infos = []
    print >> decodescript, '#!/usr/bin/env bash'
    print >> decodescript, 'HOST=`hostname`'
    print >> decodescript, 'TMPDIR=%s' % os.getenv('TMPDIR','/tmp')
    if logfile:
        print >> decodescript, 'LOG=%s/decode-log.$HOST-$$.log' % logdir
        print >> decodescript, 'INSLOG=%s/instruction-log.$HOST-$$.log' % logdir
    print >> decodescript, 'cd %s' % d.tmpdir
    print >> decodescript, 'set -e'
    print >> decodescript, 'set -o pipefail'
    if include_instruction_pipe:
        print >> decodescript, os.path.join(d.scriptdir,'decoder-instructions'), ruledir , '-c %s \\' % d.config_files 
        if logfile:
            print >> decodescript, ' 2> $INSLOG \\'
        print >> decodescript, ' | \\' 
    print >> decodescript, d.config['decoder']['exec'], "%s/xsearchdb" % ruledir , '--multi-thread \\'
    if 'weights' in d.config:
        weightstring = d.config['weights'] 
    if weightstring:
        print >> decodescript, '  -w %s \\' % os.path.abspath(weightstring)
    if 'nbests' not in d.config['decoder']['options']:
        d.config['decoder']['options']['nbests'] = 10
    for k,v in d.config['decoder']['options'].iteritems():
        print >> decodescript,'  --%s %s \\' % (k,v)
    for step in cfg.steps(d):
        if step.stage == 'decode':
            print >> decodescript, '  %s \\' % step.options
            if step.info != '':
                infos.append(step.info)
    if len(infos) > 0:
        print >> decodescript, '  -u %s \\' % ','.join(infos)

    if stage == 'nbest':
        print >> decodescript, '  --output-format nbest --newline-after-pop true --append-rules \\'
    elif stage == 'forest':
        print >> decodescript, '  --output-format forest --newline-after-pop true \\'
    if logfile:
        print >> decodescript, ' 2> $LOG \\'
        #print >> decodescript, '  2> >(gzip > $LOG.gz) \\'
    if stage == 'forest':
        print >> decodescript, "| %s/join_forests" % d.scriptdir 
    else:
        print >> decodescript, "| %s/join_nbests %s" % (d.scriptdir,d.config['decoder']['options']['nbests'])
    print >> decodescript, '\n\n'
    if logfile:
        print >> decodescript, 'gzip  $LOG\n\n'
        if include_instruction_pipe:
            print >> decodescript, 'gzip $INSLOG\n\n'
    decodescript.close()
    os.chmod(decodefile, stat.S_IRWXU | os.stat(decodefile)[stat.ST_MODE])
    return decodefile

if __name__ == '__main__':
    import argparse
    arp = argparse.ArgumentParser()
    arp.add_argument( 'decodepipe')
    arp.add_argument( 'tunedir'
                    , nargs='?'
                    , help='output directory of ruleset pipeline'
                    , action=cfg.store_abspath
                    , default=argparse.SUPPRESS
                    )
    d = cfg.parse_args(arp,default='$tunedir/tune.config', modeldir=True)

    write_script(d,'nbest', weightstring=os.path.join(d.config['tunedir'],'weights.final'),logfile=False,include_instruction_pipe=True,decodefile=d.config['decodepipe'])
    pass
