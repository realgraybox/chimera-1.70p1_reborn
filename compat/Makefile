include ../mk.config

# fallback-sources
SRCS = \
  $(if $(NEED_STRCASECMP), strcasecmp.c) \
  $(if $(NEED_GETCWD), getcwd.c) \
  $(if $(NEED_STRSTR), strstr.c) \
  $(if $(NEED_STRTOL), strtol.c strtoul.c)

OBJS = $(SRCS:.c=.o)
TARGET = libxcompat.a

all: $(if $(SRCS), $(TARGET), nothing)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

clean:
	rm -f $(OBJS) $(TARGET)

nothing:
	@echo "Nothing to build (all compatibility functions available)"
