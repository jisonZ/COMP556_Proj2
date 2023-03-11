CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	= -lm
DEFS 	 	= 

all:	gen_file recvf sendf 

gen_file: ./send/gen_file.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o ./send/gen_file.out ./send/gen_file.c $(LDFLAGS)

recvf: ./recv/recvfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o ./recv/recvfile ./recv/recvfile.c utils.h $(LDFLAGS)

sendf: ./send/sendfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o ./send/sendfile ./send/sendfile.c utils.h $(LDFLAGS)


clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f ./send/gen_file.out
	rm -f ./send/send.out
	rm -f ./recv/recv.out
