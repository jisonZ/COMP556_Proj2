CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	= 
DEFS 	 	=

all:	gen_file test recev send

gen_file: gen_file.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o gen_file gen_file.c

test: test.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o test test.c

recev: recvfile.c 
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o recev recvfile.c
send: sendfile.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o send sendfile.c 
	

clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f gen_file
	rm -f test
	rm -f send
	rm -f recev
