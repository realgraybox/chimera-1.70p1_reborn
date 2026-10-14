include ../../mk.config

# path to local WEBP build
WEBP_DIR   = $(CURDIR)/libwebp-0.4.4
PREFIX_DIR = $(CURDIR)/build_target

# path to local libtiff build
TIFF_DIR   = $(CURDIR)/tiff-3.7.0

SRCS = xvl.c
OBJS = $(SRCS:.c=.o)
TARGET = xvl

#-DUSE_TIFF -DWEBP

CFLAGS +=   -I$(PREFIX_DIR)/include -DUSE_TIFF -DWEBP
LDFLAGS += 

WEBP_LIB = $(PREFIX_DIR)/lib/libwebp.a
TIFF_LIB = $(PREFIX_DIR)/lib/libtiff.a

TARGET_CFLAGS  += $(CFLAGS)
TARGET_LDFLAGS += $(LDFLAGS) $(PREFIX_DIR)/lib/libwebp.a $(PREFIX_DIR)/lib/libtiff.a -lm -lz

.PHONY: all clean install

# main target depends on objects and local WEBP library
all: $(WEBP_LIB) $(TIFF_LIB) $(TARGET)

# build main program
$(TARGET): $(OBJS) $(WEBP_LIB)
	$(CC) -o $@ $^ $(TARGET_LDFLAGS) $(TARGET_CFLAGS)
	
# build local WEBP
$(WEBP_LIB):
	mkdir -p $(PREFIX_DIR)
	cd $(WEBP_DIR) && ./configure \
		ac_cv_header_emmintrin_h=no \
		--prefix=$(PREFIX_DIR) \
		--enable-static \
		--disable-shared \
        --disable-libwebpencode \
        --disable-animations \
        --disable-threading \
        --disable-gl \
        --disable-png \
        --disable-tiff \
        --disable-gif \
        --disable-jpeg \
        --disable-programs \
		CC="$(CC)" \
		CFLAGS="$(CFLAGS)" \
		CXXFLAGS="$(CFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		AM_LDFLAGS="$(LDFLAGS)"
	$(MAKE) -C $(WEBP_DIR)/src
	$(MAKE) -C $(WEBP_DIR)/src install
	
#		--disable-ccitt \
# --- build the local libtiff ---
$(TIFF_LIB):
	mkdir -p $(PREFIX_DIR)
	cd $(TIFF_DIR) && ./configure \
		--prefix=$(PREFIX_DIR) \
		--enable-static \
		--disable-shared \
		--disable-thunder \
		--disable-next \
		--disable-logluv \
		--disable-strip-chopping \
		--disable-jpeg \
		--disable-extrasample-as-alpha \
		--disable-check-ycbcr-subsampling \
		--disable-pixarlog \
		mandir=/usr/share/man \
		CC="$(CC)" \
		CFLAGS="$(CFLAGS)" \
		CXXFLAGS="$(CFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		AM_LDFLAGS="$(LDFLAGS)"
	$(MAKE) -C $(TIFF_DIR)/port
	$(MAKE) -C $(TIFF_DIR)/libtiff
	$(MAKE) -C $(TIFF_DIR)/libtiff install

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -m 755 chimera-xvl $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf $(PREFIX_DIR)
	@if [ -d "$(WEBP_DIR)" ] && [ -f "$(WEBP_DIR)/Makefile" ]; then \
		$(MAKE) -C $(WEBP_DIR) clean; \
	fi
	@if [ -d "$(TIFF_DIR)" ] && [ -f "$(TIFF_DIR)/Makefile" ]; then \
		$(MAKE) -C $(TIFF_DIR) clean; \
	fi
	
