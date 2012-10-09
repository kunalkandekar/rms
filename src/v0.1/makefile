#
# Makes dfs
#

CFLAGS	= -g
CFLAGSL	= -g -fPIC
SRC     =  
OBJ     = $(SRC:.cpp=.o)
CC	= gcc

UTILDIR=../lib/util

BINDIR=../../bin

LINK	= -lm -lpthread -ldl -lresolv #-lposix4 -lsocket -lnsl -lcurses 

INCLUDE	= -I./ -I/usr/include -I$(UTILDIR)

all: clean rmcast.o rmctest

rmctest: clean hashtable.o mqueue.o async.o uthread.o rmcast.o
	$(CC) $(CFLAGS) -o $(BINDIR)/rmctest rmctest.c $(BINDIR)/mqueue.o $(BINDIR)/async.o\
	                   $(BINDIR)/hashtable.o $(BINDIR)/uthread.o\
	                   $(BINDIR)/rmcast.o $(INCLUDE) $(LINK)

rmcast.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rmcast.o -c rmcast.c $(INCLUDE)

uthread.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/uthread.o -c $(UTILDIR)/uthread.c $(INCLUDE)
	
mqueue.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/mqueue.o -c $(UTILDIR)/mqueue.c $(INCLUDE)

async.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/async.o -c $(UTILDIR)/async.c $(INCLUDE)

hashtable.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/hashtable.o -c $(UTILDIR)/hashtable.c $(INCLUDE)

#.o:
#	$(CC) $(CFLAGS) -o .o -c .c 

clean:
	rm -f *.so rmctest *.o
