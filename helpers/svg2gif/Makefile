include ../../mk.config

# Check if uClibc is present by runnig a small test through preprocessor
IS_UCLIBC := $(shell $(CC) -include features.h -dM -E - < /dev/null | grep -c "__UCLIBC__")

# If result is 1 (true), add special flags
ifeq ($(IS_UCLIBC), 1)
    CFLAGS += -fno-builtin-sin -fno-builtin-cos -fno-builtin-tan \
              -fno-builtin-atan2 -fno-builtin-fmod -fno-builtin-acos
    # Add definition so code knows to activate wrappers
    CFLAGS += -D__UCLIBC__
endif

LDFLAGS += -lgif

SRCS = svg2gif.c

OBJS = $(SRCS:.c=.o)
TARGET = svg2gif


.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET) $(OBJS)
