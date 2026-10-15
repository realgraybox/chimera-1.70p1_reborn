include ../../mk.config

SRCS = xmenu.c

OBJS = $(SRCS:.c=.o)
TARGET = chimera-menu
CFLAGS += --std=c99

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

# chimera-menu, and the sendto script Chimera runs on a right-click.
# The menu itself is lib/sendto, installed to ~/chimera.
install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -m 755 mime/sendto $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET) $(OBJS)
