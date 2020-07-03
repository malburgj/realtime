INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=-lpthread -lrt

HFILES=

CFILES1= prob4_posix.c 
CFILES2= prob4_heap.c

SRCS1= ${HFILES} ${CFILES1}
SRCS2= ${HFILES} ${CFILES2}

OBJS1= ${CFILES1:.c=.o}
OBJS2= ${CFILES2:.c=.o}

all: prob4_posix prob4_heap

clean:
	-rm -f *.o *.d *.elf

prob4_posix: prob4_posix.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@.elf $(OBJS1) $(LIBS)

prob4_heap: prob4_heap.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@.elf $(OBJS2) $(LIBS)

depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
