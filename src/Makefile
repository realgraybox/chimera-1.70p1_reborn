include ../mk.config

# uClibc flags are set in mk.config

SRCS = \
	cache.c url.c net.c ftp.c convert.c gopher.c http.c  \
    util.c inline.c \
    document.c local.c widget.c main.c mime.c input.c md5.c chimera_auth.c \
    fallback.c lang.c stringdb.c charset.c imgload.c

OBJS = $(SRCS:.c=.o)
TARGET = chimera
CFLAGS += -I../XawPlus-2.0.2 -I../ -funit-at-a-time


.PHONY: all clean install

all: $(TARGET)

# Relink when one of our own libraries has been rebuilt
LIBDEPS = ../libhtmlw/libhtmlw.a ../mxw/libmxw.a ../xloadimage/libxloadimage.a \
          $(filter %.a,$(XAW_LIBS))

$(TARGET): $(OBJS) $(LIBDEPS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(CFLAGS)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET) $(OBJS)
