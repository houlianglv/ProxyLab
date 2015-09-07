#
# Makefile for Proxy Lab
#
# You may modify is file any way you like (except for the handin
# rule). Autolab will execute the command "make" on your specific
# Makefile to build your proxy from sources.
#

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c $(LDFLAGS)

proxy.o: proxy.c csapp.h sbuf.h
	$(CC) $(CFLAGS) -c proxy.c $(LDFLAGS)

sbuf.o: sbuf.c sbuf.h csapp.h
	$(CC) $(CFLAGS) -c sbuf.c $(LDFLAGS)

cache.o: cache.c cache.h csapp.h
	$(CC) $(CFLAGS) -c cache.c $(LDFLAGS)

proxy: sbuf.o proxy.o csapp.o cache.o
	$(CC) $(CFLAGS) -o proxy proxy.o csapp.o sbuf.o cache.o $(LDFLAGS)
# Creates a tarball in ../proxylab-handin.tar that you should then
# hand in to Autolab. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")
clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz