#!/usr/bin/env python
import cfg, sys, yaml

if len(sys.argv) >=2:
    print yaml.dump(cfg.load_config_raw(sys.argv[1:],omit_sys_default=True)[0])

