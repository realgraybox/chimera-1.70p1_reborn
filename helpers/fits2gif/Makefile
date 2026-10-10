include ../../mk.config

SRCS = fits2gif.c

OBJS = $(SRCS:.c=.o)
TARGET = fits2gif
CFLAGS += --std=c99
LDFLAGS += -lgif

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET) $(OBJS)
