include ../mk.config

# uClibc flags are set in mk.config

SRCS = \
	cache.c url.c net.c ftp.c convert.c gopher.c http.c  \
    util.c inline.c \
    document.c local.c widget.c main.c mime.c input.c md5.c chimera_auth.c \
    fallback.c lang.c stringdb.c

OBJS = $(SRCS:.c=.o)
TARGET = chimera
CFLAGS += -I../XawPlus-2.0.2 -I../ -std=gnu99 -funit-at-a-time


.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET) $(OBJS)
