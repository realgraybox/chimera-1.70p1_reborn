include ../mk.config

CFLAGS += -I..

all: libcommon.a

libcommon.a:
	$(CC) $(CFLAGS) -c *.c
	$(AR) rcs libcommon.a *.o

clean:
	rm -f *.o libcommon.a
