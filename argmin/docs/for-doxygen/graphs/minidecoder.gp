
#set title "CKY decoding times on Arabic corpus"
set xlabel "Sentence length (# words)"
set ylabel "Decoding time (seconds)"

set terminal postscript
set output 'minidecoder.eps'
plot 'arabic-minidecoder-times.dat' title "Arabic corpus", x**3*0.04 title "0.04 * x^3"

set terminal png
set output 'minidecoder.png'
plot 'arabic-minidecoder-times.dat' title "Arabic corpus", x**3*0.04 title "0.04 * x^3"

