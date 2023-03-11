CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	= -lm
DEFS 	 	= 

all:	gen_file recvf sendf 

gen_file: ./send/gen_file.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o gen_file.out gen_file.c $(LDFLAGS)

recvf: ./recv/recvfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o recvfile recvfile.c utils.h $(LDFLAGS)

sendf: ./send/sendfile.c utils.h
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o sendfile sendfile.c utils.h $(LDFLAGS)


clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f gen_file.out
	rm -f sendfile
	rm -f recvfile
