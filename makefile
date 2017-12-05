all: pcsync

pcsync: pcsync.c 
	gcc pcsync.c -o pcsync -std=c99 -pthread

clean:
	/bin/rm -fr *~ *.o pcsync
