#!/usr/bin/perl -w
#===============================================================================
#
#         FILE:  fwd.pl
#
#        USAGE:  ./fwd.pl 
#
#  DESCRIPTION:  
#
#      OPTIONS:  ---
# REQUIREMENTS:  ---
#         BUGS:  ---
#        NOTES:  ---
#       AUTHOR:   (), <>
#      COMPANY:  
#      VERSION:  1.0
#      CREATED:  01/27/06 15:44:04 PST
#     REVISION:  ---
#===============================================================================

use strict;


while(<STDIN>){
   $_ =~
   s/VOCAB_USE_STD_MAP -DLM_NO_BDB_LIB -DLM_NO_COMMON_LIB/VOCAB_USE_STD_MAP/g;
   print $_;
}
