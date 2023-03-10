CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	= -lm
DEFS 	 	= 

all:	gen_file recv send test

gen_file: gen_file.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o gen_file gen_file.c $(LDFLAGS)

test: test.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o test test.c $(LDFLAGS)

recv: recvfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o recv recvfile.c utils.h $(LDFLAGS)

send: sendfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o send sendfile.c utils.h $(LDFLAGS)
	
# encode: encodeTest.c utils.h
# 	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o encode encodeTest.c utils.h $(LDFLAGS)

clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f gen_file
	rm -f send
	rm -f recv
	rm -f test