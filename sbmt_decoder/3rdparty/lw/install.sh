#!/bin/bash

TOP_DIR=$PWD
cd Horse
sed 's/\/home\/wwang\/lw-dev\/Troy/$(TOP_DIR)/g' Makefile > Makefile.bk;
mv Makefile.bk Makefile
TOP_DIR=$TOP_DIR make clean
TOP_DIR=$TOP_DIR make
cd
