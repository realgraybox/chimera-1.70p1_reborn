include ../mk.config

CFLAGS += -I. -fPIC

all: libXawPlus.a

libXawPlus.a: 
	$(CC) $(CFLAGS) -c *.c
	$(AR) rcs libXawPlus.a *.o

clean:
	rm -f *.o libXawPlus.a



