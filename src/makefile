#
# Makes rms
#

UTILDIR=./lib/util

BINDIR=../bin

LINK	= -lcurses -lm -lposix4 -lpthread -lsocket -lnsl -ldl -lresolv

INCLUDE	= -I./ -I/usr/include -I$(UTILDIR)

CFLAGS=-g -Wall #-O3 -fPIC
SRC=  
OBJ= $(SRC:.cpp=.o:INCLUDE)
CC= gcc

all: clean rms.o rmstest

rmstest: clean  librms.so
	$(CC) $(CFLAGS) -o rmstest rmstest.c -lrms.so -L

#******************************** UTILS ********************************
mqueue.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/mqueue.o -c $(UTILDIR)/mqueue.c $(INCLUDE)

async.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/async.o -c $(UTILDIR)/async.c $(INCLUDE)

hashtable.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/hashtable.o -c $(UTILDIR)/hashtable.c  $(INCLUDE)

uthread.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/uthread.o -c $(UTILDIR)/uthread.c  $(INCLUDE)

#********************************* RMC LIB ********************************	
rms.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rms.o -c rms.c $(INCLUDE)

rms_channel.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rms_channel.o -c rms_channel.c $(INCLUDE)

rms_msg.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rms_msg.o -c rms_msg.c $(INCLUDE)

rms_sys.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rms_sys.o -c rms_sys.c $(INCLUDE)

rms_interface.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rms_interface.o -c rms_interface.c $(INCLUDE)

rms_util.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rms_util.o -c rms_util.c $(INCLUDE)

rms_manager.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rms_manager.o -c rms_manager.c $(INCLUDE)

rms_sender.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rms_sender.o -c rms_sender.c $(INCLUDE)

rms_receiver.o:
	$(CC) $(CFLAGS) -o $(BINDIR)/rms_receiver.o -c rms_receiver.c $(INCLUDE)

librms.so: clean rms_interface.o rms_util.o rms_manager.o rms_receiver.o rms_sender.o rms_channel.o rms_msg.o rms_sys.o mqueue.o hashtable.o async.o
	gcc -shared -o $(BINDIR)/librms.so $(BINDIR)/rms_interface.o $(BINDIR)/rms_util.o $(BINDIR)/rms_manager.o $(BINDIR)/rms_receiver.o $(BINDIR)/rms_sender.o\
	    $(BINDIR)/rms_channel.o $(BINDIR)/rms_msg.o $(BINDIR)/rms_sys.o $(BINDIR)/mqueue.o $(BINDIR)/hashtable.o $(BINDIR)/async.o $(LINK)
	rm -f $(BINDIR)/*.o

#.o:
#	$(CC) $(CFLAGS) -o .o -c .c 

#********************************* CLEAN ********************************
clean:
	rm -f $(BINDIR)/*.so $(BINDIR)/rmstest $(BINDIR)/*.o
