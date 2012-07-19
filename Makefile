CC = gcc
CFLAGS  = -Wall -O0 -static -g 
LIBS    = -lgsl -lgslcblas -lm
OBJECTS = lex.o umalloc.o syntax.o hash.o keyword.o symbol.o eval.o misc.o \
		libm.o function.o primes.o as_tree.o traverse.o list.o main.o \
		libcall.o

.PHONY: clean

all: bclite

bclite: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

clean:
	rm -rf *~ *.o



