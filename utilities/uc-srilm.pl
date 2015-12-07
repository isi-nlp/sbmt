#!/usr/bin/perl

while(<>) {
    s/^(-\S*)(.*)/$1.uc($2)/e;
    s#(\s<(UNK|S|/S)>(?!\S))#lc($1)#ge;
    print;
}
