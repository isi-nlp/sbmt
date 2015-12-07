#!/usr/bin/env python
import os
import cfg
import sys
import stat

def write_script(d):
    steps = cfg.steps(d)
    
    repairs = ['cat']
    for step in steps:
        if step.stage == 'post-process-extras':
            repairs.append(step.executable())
    
    postprocmap = os.path.join(d.tmpdir,'postprocmap')
    ppm = open(postprocmap,'w')
    print >> ppm, '#!/usr/bin/env bash'
    print >> ppm, 'set -e'
    print >> ppm, 'set -o pipefail'
    print >> ppm, 'cd', os.getcwd()
    print >> ppm, '|'.join(repairs)
    ppm.close()
    os.chmod(postprocmap,stat.S_IRWXU | stat.S_IRWXG | stat.S_IXUSR)
    
    return postprocmap
