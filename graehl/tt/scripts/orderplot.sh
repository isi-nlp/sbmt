#!/bin/bash

if [ -f $1 ] ; then 
#sed 's/e\^//' < $1 > $1.dat
    #enhanced
cat > $1.eps.gnuplot << EOF
set terminal postscript eps noenhanced color solid defaultplex "Times-Roman" 24
set size 1.5,1.5
set output "$1.eps"
$3
EOF

cat > $1.png.gnuplot << EOF
set terminal png color
set output "$1.png"
$3
EOF
    
cat > $1.gnuplot << EOF
set xlabel "quantile (fraction of items less than y-axis)"
set ylabel "$1"
set logscale y
$2
plot "$1" using 1:2 notitle with lines
EOF
    
gnuplot $1.eps.gnuplot $1.gnuplot

gnuplot $1.png.gnuplot $1.gnuplot

#gnuplot -persist -raise $1.gnuplot

else
echo arg file \"$1\" not found.
fi