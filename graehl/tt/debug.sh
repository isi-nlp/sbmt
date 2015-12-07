#!/bin/bash
. ~/isd/env.sh
cd $d
#gdb --args bin/tt.cygwin.test --catch_system_errors=no
gdb --args bin/$ARCH/forest-em.debug -f sample/norm_and_forests -L 0 -R sample/rule_list -w 1 -W 1
