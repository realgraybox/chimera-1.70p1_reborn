include ../../mk.config

# check if uClibc is present by running a small test through preprocessor
IS_UCLIBC := $(shell $(CC) -include features.h -dM -E - < /dev/null | grep -c "__UCLIBC__")

TARGET_CFLAGS = $(CFLAGS) -std=gnu99

# -fPIC only for 64bit
ifneq ($(IS_UCLIBC), 1)
    TARGET_CFLAGS += -fPIC
else
    TARGET_CFLAGS += -D__UCLIBC__
endif

# path to sources
MBEDTLS_DIR = $(CURDIR)/mbedtls-mbedtls-2.16.0
CURL_DIR    = $(CURDIR)/curl-7.47.0

# local path where mbedtls temporary installed
PREFIX_DIR  = $(CURDIR)/build_target

TARGET = curl.7.47.0

# Definer de faktiske filer vi holder øje med
MBEDTLS_LIBS = $(PREFIX_DIR)/lib/libmbedcrypto.a $(PREFIX_DIR)/lib/libmbedx509.a $(PREFIX_DIR)/lib/libmbedtls.a
CURL_SRC_BIN = $(CURL_DIR)/src/curl

# Kun reelle ikke-fil targets skal være .PHONY
.PHONY: all clean install

# build mbedtls first, then curl
all: $(TARGET)

# 1. build mbedTLS static and install local in build_target folder
$(MBEDTLS_LIBS):
	# using mbedTLS own Makefile-system but modify CC and CFLAGS
	$(MAKE) lib -C $(MBEDTLS_DIR) CC="$(CC)" CFLAGS="$(TARGET_CFLAGS)" 
	# create local folders for header and library
	mkdir -p $(PREFIX_DIR)/lib $(PREFIX_DIR)/include
	cp -r $(MBEDTLS_DIR)/include/mbedtls $(PREFIX_DIR)/include/
	cp $(MBEDTLS_DIR)/library/libmbedcrypto.a $(PREFIX_DIR)/lib/
	cp $(MBEDTLS_DIR)/library/libmbedx509.a $(PREFIX_DIR)/lib/
	cp $(MBEDTLS_DIR)/library/libmbedtls.a $(PREFIX_DIR)/lib/

# 2. configure and build curl statisc with reference to local build mbedTLS
$(CURL_SRC_BIN): $(MBEDTLS_LIBS)
	cd $(CURL_DIR) && ./configure \
		--disable-shared \
		--enable-static \
		--with-mbedtls=$(PREFIX_DIR) \
		--disable-ftp \
		--disable-file \
		--disable-ldap \
		--disable-rtsp \
		--disable-dict \
		--disable-telnet \
		--disable-tftp \
		--disable-pop3 \
		--disable-imap \
		--disable-smb \
		--disable-smtp \
		--disable-gopher \
		--disable-manual \
		--without-libidn \
		--without-librtmp \
		--without-zlib \
		--disable-ldap \
		--disable-ldaps \
		--disable-sspi \
		--disable-crypto-auth \
		--disable-ntlm-wb \
		--disable-ipv6 \
		--disable-verbose \
		--disable-ftps \
		--disable-libcurl-option \
		--disable-mqtt \
		--without-ssl \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		CC="$(CC)"
		
	$(MAKE) -C $(CURL_DIR)
	
	
# 3. Kopier den endelige binære fil til hovedmappen
$(TARGET): $(CURL_SRC_BIN)
	cp $(CURL_SRC_BIN) ./$(TARGET)
	
install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -m 755 https-helper_nocookie.sh $(DESTDIR)$(BINDIR)/
	install -m 755 https-helper_cookie.sh $(DESTDIR)$(BINDIR)/

clean:
	-$(MAKE) -C $(MBEDTLS_DIR) clean
	-$(MAKE) -C $(CURL_DIR) clean
	rm -rf $(PREFIX_DIR) curl.7.47.0
