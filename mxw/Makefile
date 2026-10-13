include ../mk.config

CFLAGS += -I.. -I../common -I../mxw

TARGET = libmxw.a

all: libmxw.a

libmxw.a: 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c *.c
	$(AR) rcs libmxw.a  *.o

clean:
	rm -f *.o libmxw.a

