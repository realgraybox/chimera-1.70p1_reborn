include ../mk.config

CFLAGS += -I.. -I../common -I../mxw

all: libhtmlw.a

libhtmlw.a: 
	$(CC) $(CFLAGS) -c *.c
	$(AR) rcs libhtmlw.a *.o

clean:
	rm -f *.o libhtmlw.a
