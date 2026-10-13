include ../mk.config

CFLAGS += -I. -fPIC

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: all clean

all: libXawPlus.a

# make's built-in rule compiles each .c with CPPFLAGS and CFLAGS, and
# only the files that changed. Headers are not tracked: run
# "make clean" after editing a .h file.
libXawPlus.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

clean:
	rm -f *.o libXawPlus.a
