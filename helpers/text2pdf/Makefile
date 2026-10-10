include ../../mk.config

SRCS = text2pdf.c

OBJS = $(SRCS:.c=.o)
TARGET = text2pdf
CFLAGS += --std=c99
LDFLAGS +=

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET) $(OBJS)
