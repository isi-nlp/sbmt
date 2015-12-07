#!/usr/bin/perl

# usage: mem-watcher.pl "ls -al > out"
# This script runs a command line, and watches the command lines memory usage

use strict;
use warnings;
use POSIX ":sys_wait_h";

my $command_line = shift @ARGV or die "Please specify a command line (in quotes)";

my $tick = 1;			# seconds to wait

if (!defined(my $kidpid = fork())) { 
  die "********* Couldn't fork, oops *********";
} elsif ($kidpid==0) { 
  exec("$command_line");
} else { 

  my $max_mem=0;
  my $max_cpu=0;
  my $max_rss=0;
  my $max_mem_percent=0;
  
  while (1) {
    my $info = `ps -o vsz,rss,%mem,%cpu -p $kidpid | tail -n 1`;
    my @info_arr = split(' ', $info);
    if ($info_arr[0]>$max_mem) { 
      $max_mem = $info_arr[0];
    }
    if ($info_arr[1]>$max_rss) { 
      $max_rss = $info_arr[1];
    }
    if ($info_arr[2]>$max_mem_percent) { 
      $max_mem_percent = $info_arr[2];
    }
    if ($info_arr[3]>$max_cpu) { 
      $max_cpu = $info_arr[3];
    }
    waitpid($kidpid,&WNOHANG);
    my $stat = $?;
    if ($stat!=-1) { 
      last;
    }
    sleep($tick);
  }
  print "maxvsz=$max_mem, maxrss=$max_rss, maxcpu=$max_cpu, max%mem=$max_mem_percent for command line(pid=$kidpid): <<<$command_line>>>\n";
}


