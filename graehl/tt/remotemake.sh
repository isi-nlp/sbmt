#!/bin/bash
ssh -i /cache/.ssh/identity nlg0 'cd ~/dev/tt && PATH=/home/graehl/isd/linux/bin:$PATH LD_LIBRARY_PATH=/home/graehl/isd/linux/lib ARCH=linux make'
