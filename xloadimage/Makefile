include ../mk.config

CFLAGS += -I.. -I../common -I../mxw -I../compat

all: libxloadimage.a

libxloadimage.a: 
	$(CC) $(CFLAGS) -c *.c
	$(AR) rcs libxloadimage.a *.o

clean:
	rm -f *.o libxloadimage.a
