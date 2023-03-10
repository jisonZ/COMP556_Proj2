CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	= -lm
DEFS 	 	= 

all:	gen_file recvf sendf test encode

gen_file: ./send/gen_file.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o ./send/gen_file.out ./send/gen_file.c $(LDFLAGS)

test: test.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o test test.c $(LDFLAGS)

recvf: ./recv/recvfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o ./recv/recv.out ./recv/recvfile.c utils.h $(LDFLAGS)

sendf: ./send/sendfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o ./send/send.out ./send/sendfile.c utils.h $(LDFLAGS)
	
encode: encodeTest.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o encode encodeTest.c utils.h $(LDFLAGS)

clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f ./send/gen_file.out
	rm -f ./send/send.out
	rm -f ./recv/recv.out
	rm -f test
	rm -f encode