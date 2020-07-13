INCLUDE_DIRS = 
LIB_DIRS = 
CC=g++

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
CPPLIBS= -L/usr/lib -lopencv_core -lopencv_flann -lopencv_video -lpthread -lrt

PRODUCT=prob5
HFILES= 
CFILES= 
CPPFILES= ${PRODUCT}.cpp

SRCS= ${HFILES} ${CFILES}
CPPOBJS= ${CPPFILES:.cpp=.o}

all: ${PRODUCT}

clean:
	-rm -f *.o *.d
	-rm -f *.elf

distclean:
	-rm -f *.o *.d

${PRODUCT}: ${PRODUCT}.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@.elf $@.o `pkg-config --libs opencv` $(CPPLIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

.cpp.o:
	$(CC) $(CFLAGS) -c $<
