include ../../mk.config

# Paths are relative, since make cannot handle spaces in target names.
# Neither library is installed with "make install": configure's --prefix
# and libtool's -rpath break on a path with spaces. Only the static
# libraries are built, and they are copied to build_target with their
# headers.
WEBP_DIR   = libwebp-0.4.4
TIFF_DIR   = tiff-3.7.0
PREFIX_DIR = build_target

SRCS = xvl.c
OBJS = $(SRCS:.c=.o)
TARGET = xvl

#-DUSE_TIFF -DWEBP

CFLAGS +=   -I$(PREFIX_DIR)/include -DUSE_TIFF -DWEBP
LDFLAGS += 

WEBP_LIB = $(PREFIX_DIR)/lib/libwebp.a
TIFF_LIB = $(PREFIX_DIR)/lib/libtiff.a

TARGET_CFLAGS  += $(CFLAGS)
TARGET_LDFLAGS += $(HELPER_LDFLAGS) $(PREFIX_DIR)/lib/libwebp.a $(PREFIX_DIR)/lib/libtiff.a \
                  -lXpm -lXext -lX11 -lm -lz

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
		LDFLAGS="$(HELPER_LDFLAGS)" \
		AM_LDFLAGS="$(HELPER_LDFLAGS)"
	$(MAKE) -C $(WEBP_DIR)/src
	mkdir -p $(PREFIX_DIR)/lib $(PREFIX_DIR)/include/webp
	cp $(WEBP_DIR)/src/webp/decode.h $(WEBP_DIR)/src/webp/types.h $(PREFIX_DIR)/include/webp/
	cp $(WEBP_DIR)/src/.libs/libwebp.a $@
	
#		--disable-ccitt \
# --- build the local libtiff ---
$(TIFF_LIB):
	mkdir -p $(PREFIX_DIR)
	cd $(TIFF_DIR) && ./configure \
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
		CC="$(CC)" \
		CFLAGS="$(CFLAGS)" \
		CXXFLAGS="$(CFLAGS)" \
		LDFLAGS="$(HELPER_LDFLAGS)" \
		AM_LDFLAGS="$(HELPER_LDFLAGS)"
	$(MAKE) -C $(TIFF_DIR)/port
	$(MAKE) -C $(TIFF_DIR)/libtiff libtiff.la
	mkdir -p $(PREFIX_DIR)/lib $(PREFIX_DIR)/include
	cd $(TIFF_DIR)/libtiff && cp tiff.h tiffio.h tiffvers.h tiffconf.h ../../$(PREFIX_DIR)/include/
	cp $(TIFF_DIR)/libtiff/.libs/libtiff.a $@

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
	
