#
# Compilation and link flags
#
CC = gcc -O3
CFLAGS =  
LIBS=  -lm 

OBJ = parser.o model2.o model2tree.o tree.o sentence.o edge.o prob.o hash.o jmphash.o key.o grammar.o mymalloc.o mymalloc_char.o effhash.o getGrammar.o trees2rules.o
HDR = parser.h model2.h model2tree.h tree.h treelexhead.h sentence.h prob.h hash.h jmphash.h key.h grammar.h mymalloc.h mymalloc_char.h effhash.h

all: assignModel2Prob

$(OBJ) : $(HDR)

ofiles:
	gcc -c parser.c model2.c tree.c sentence.c edge.c prob.c hash.c jmphash.c key.c grammar.c mymalloc.c mymalloc_char.c effhash.c $(CFLAGS) $(LIBS)

parser: parser.o model2.o tree.o sentence.o edge.o prob.o hash.o jmphash.o key.o grammar.o mymalloc.o mymalloc_char.o effhash.o
	gcc -o parser parser.o model2.o tree.o sentence.o edge.o prob.o hash.o jmphash.o key.o grammar.o mymalloc.o mymalloc_char.o effhash.o $(CFLAGS) $(LIBS)

assignModel2Prob: assignModel2Prob.o model2.o model2tree.o tree.o sentence.o edge.o hash.o jmphash.o key.o grammar.o mymalloc.o mymalloc_char.o effhash.o prob.o
	gcc -o assignModel2Prob assignModel2Prob.o model2.o model2tree.o tree.o sentence.o edge.o hash.o jmphash.o key.o grammar.o mymalloc.o mymalloc_char.o effhash.o prob.o $(CFLAGS) $(LIBS)

useAPI: useAPI.o model2.o hash.o jmphash.o key.o grammar.o mymalloc.o mymalloc_char.o tree.o effhash.o prob.o
	gcc -o  useAPI useAPI.o model2.o tree.o hash.o jmphash.o key.o grammar.o mymalloc.o mymalloc_char.o effhash.o prob.o$(CFLAGS) $(LIBS)


clean:
	rm -f *.o
