include ../../mk.config

#
# HTTPS for Chimera
#
# By default Chimera uses the system's curl through https-helper.sh, and
# nothing is built here: "make" does nothing and "make install" installs
# the script.
#
# For a fully static build (for example the 32-bit uClibc toolchain) a
# small curl can be built from pinned sources:
#
#   make static-curl          download, check, build ./chimera-curl
#   make install-static-curl  install it as $(BINDIR)/chimera-curl
#
# https-helper.sh uses chimera-curl automatically when it is installed.
# The sources are downloaded on demand instead of being kept in the
# repository: curl as the release tarball (checked against its SHA-256),
# mbedTLS as a git checkout of the release tag (checked against its commit).
#

CURL_VERSION   = 7.47.0
CURL_TARBALL   = curl-$(CURL_VERSION).tar.gz
CURL_URL       = https://github.com/curl/curl/releases/download/curl-7_47_0/$(CURL_TARBALL)
CURL_SHA256    = df01bd42af361978d9de7de8529718bcafe01897a544a7650139a1954c55bdfe

MBEDTLS_TAG    = mbedtls-2.16.0
MBEDTLS_REPO   = https://github.com/Mbed-TLS/mbedtls.git
MBEDTLS_COMMIT = fb1972db23da39bd11d4f9c9ea6266eee665605b

CURL_DIR    = $(CURDIR)/curl-$(CURL_VERSION)
MBEDTLS_DIR = $(CURDIR)/mbedtls-mbedtls-2.16.0
PREFIX_DIR  = $(CURDIR)/build_target

TARGET = chimera-curl

TARGET_CFLAGS = $(CFLAGS) -std=gnu99

# -fPIC only when not building statically against uClibc (IS_UCLIBC is set in mk.config)
ifneq ($(IS_UCLIBC), 1)
    TARGET_CFLAGS += -fPIC
endif

# Only the flags curl needs, not Chimera's own libraries
CURL_LDFLAGS = $(STATIC) -Wl,--gc-sections,--sort-common,-s

MBEDTLS_LIBS = $(PREFIX_DIR)/lib/libmbedcrypto.a $(PREFIX_DIR)/lib/libmbedx509.a $(PREFIX_DIR)/lib/libmbedtls.a
CURL_SRC_BIN = $(CURL_DIR)/src/curl

.PHONY: all install static-curl install-static-curl clean distclean

all:
	@echo "helpers/curl: using the system curl; see 'make static-curl' for a static one"

install:
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 https-helper.sh $(DESTDIR)$(BINDIR)/

static-curl: $(TARGET)

install-static-curl: $(TARGET) install
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

# 0a. curl: download the release tarball, check it, unpack it.
#     ltmain.sh is patched so libtool passes -static on to the linker,
#     which a fully static curl needs.
$(CURL_DIR)/configure:
	[ -f $(CURL_TARBALL) ] || curl -fL -o $(CURL_TARBALL) $(CURL_URL) \
		|| wget -O $(CURL_TARBALL) $(CURL_URL)
	echo "$(CURL_SHA256)  $(CURL_TARBALL)" | sha256sum -c -
	tar xzf $(CURL_TARBALL)
	sed -i 's/^\([[:space:]]*\)-all-static)$$/\1-all-static|-static)/' $(CURL_DIR)/ltmain.sh
	grep -c -- '-all-static|-static)' $(CURL_DIR)/ltmain.sh | grep -qx 2
	touch $@

# 0b. mbedTLS: check out the release tag and make sure it is the expected commit
$(MBEDTLS_DIR)/Makefile:
	[ -d $(MBEDTLS_DIR) ] || git clone --depth 1 --branch $(MBEDTLS_TAG) $(MBEDTLS_REPO) $(MBEDTLS_DIR)
	test "`git -C $(MBEDTLS_DIR) rev-parse HEAD`" = "$(MBEDTLS_COMMIT)"
	touch $@

# 1. build mbedTLS static and install it locally in build_target
$(MBEDTLS_LIBS): $(MBEDTLS_DIR)/Makefile
	$(MAKE) lib -C $(MBEDTLS_DIR) CC="$(CC)" CFLAGS="$(TARGET_CFLAGS)"
	mkdir -p $(PREFIX_DIR)/lib $(PREFIX_DIR)/include
	cp -r $(MBEDTLS_DIR)/include/mbedtls $(PREFIX_DIR)/include/
	cp $(MBEDTLS_DIR)/library/libmbedcrypto.a $(PREFIX_DIR)/lib/
	cp $(MBEDTLS_DIR)/library/libmbedx509.a $(PREFIX_DIR)/lib/
	cp $(MBEDTLS_DIR)/library/libmbedtls.a $(PREFIX_DIR)/lib/

# 2. configure and build curl against the local mbedTLS, HTTP and HTTPS only
$(CURL_SRC_BIN): $(CURL_DIR)/configure $(MBEDTLS_LIBS)
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
		LDFLAGS="$(CURL_LDFLAGS)" \
		CC="$(CC)"
	$(MAKE) -C $(CURL_DIR)

# 3. copy the binary here under its installed name
$(TARGET): $(CURL_SRC_BIN)
	cp $(CURL_SRC_BIN) ./$(TARGET)

clean:
	@# only library/ is built; mbedTLS's own top-level clean uses
	@# "find -exec ... {} +", which older find versions do not support
	if [ -d $(MBEDTLS_DIR)/library ]; then $(MAKE) -C $(MBEDTLS_DIR)/library clean; fi
	if [ -f $(CURL_DIR)/Makefile ]; then $(MAKE) -C $(CURL_DIR) clean; fi
	rm -rf $(PREFIX_DIR) $(TARGET)

# also remove the downloaded sources
distclean: clean
	rm -rf $(CURL_DIR) $(MBEDTLS_DIR) $(CURL_TARBALL)
