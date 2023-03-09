CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	= 
DEFS 	 	=

all:	gen_file test recv send encode

gen_file: gen_file.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o gen_file gen_file.c

test: test.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o test test.c

recv: recvfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o recv recvfile.c utils.h

send: sendfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o send sendfile.c utils.h
	
encode: encodeTest.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o encode encodeTest.c utils.h

clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f gen_file
	rm -f test
	rm -f send
	rm -f recv
	rm -f encode
