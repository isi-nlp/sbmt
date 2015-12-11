#!/usr/bin/env python
import os
import cfg
import sys
import stat

def write_script(d, stage, weightstring=None, logfile=True, include_instruction_pipe=False):
    if stage not in set(['nbest','forest']):
        raise Exception
    if logfile:
        logdir = os.path.join(d.outdir,'logs')
        cfg.execute(d,'mkdir -p %s' % logdir)
    ruledir = d.config['rules']
    decodefile = os.path.join(d.tmpdir,'decoder')
    decodescript = open(decodefile,'w')
    infos = []
    print >> decodescript, '#!/usr/bin/env bash'
    print >> decodescript, 'HOST=`hostname`'
    if logfile:
        print >> decodescript, 'LOG=%s/decode-log.$HOST-$$.log' % logdir
    print >> decodescript, 'cd %s' % d.tmpdir
    print >> decodescript, 'set -e'
    print >> decodescript, 'set -o pipefail'
    if include_instruction_pipe:
        print >> decodescript, os.path.join(d.scriptdir,'decoder-instructions'), ruledir , '-c %s | \\' % d.config_files
    print >> decodescript, d.config['decoder']['exec'], "%s/xsearchdb" % ruledir , '--multi-thread \\'
    if 'weights' in d.config:
        print >> decodescript, '  -w %s \\' % os.path.abspath(d.config['weights'])
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
        print >> decodescript, '  --output-format nbest \\'
    elif stage == 'forest':
        print >> decodescript, '  --output-format forest \\'
    if logfile:
        print >> decodescript, '  2> >(gzip > $LOG.gz) \\'
    if stage == 'forest':
        print >> decodescript, "| sed -u -e 's/@UNKNOWN@//g' " 
    else:
        print >> decodescript, "\n"
    decodescript.close()
    os.chmod(decodefile, stat.S_IRWXU | os.stat(decodefile)[stat.ST_MODE])
    return decodefile
