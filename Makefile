CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	= 
DEFS 	 	=

all:	gen_file test

gen_file: gen_file.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o gen_file gen_file.c

test: test.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o test test.c
clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f gen_file
	rm -f test
