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
#   make cacert               download curl's current CA bundle to
#                             ~/chimera/cacert.pem, used by https-helper.sh
#
# https-helper.sh uses chimera-curl automatically when it is installed.
# The sources are downloaded on demand instead of being kept in the
# repository, as release tarballs checked against their SHA-256. (The
# tarballs were checked against the projects' git tags: the same files,
# plus the generated build files.)
#
# curl 8.23.0 and Mbed TLS 3.6.7 (long-term support, TLS 1.3). The
# versions used before, curl 7.47.0 and mbed TLS 2.16.0 from 2016 and
# 2018, are still there with LEGACY=1, for an old compiler that cannot
# build the new ones:
#
#   make static-curl LEGACY=1
#

ifeq ($(LEGACY),1)
CURL_VERSION    = 7.47.0
CURL_SHA256     = df01bd42af361978d9de7de8529718bcafe01897a544a7650139a1954c55bdfe
# mbed TLS 2.16 as a git checkout of the release tag, checked against
# its commit, as before
MBEDTLS_TAG     = mbedtls-2.16.0
MBEDTLS_REPO    = https://github.com/Mbed-TLS/mbedtls.git
MBEDTLS_COMMIT  = fb1972db23da39bd11d4f9c9ea6266eee665605b
MBEDTLS_DIR     = mbedtls-mbedtls-2.16.0
# curl 7.47 would also pick up OpenSSL if it found it
CURL_EXTRA_FLAGS = --without-ssl
else
CURL_VERSION    = 8.23.0
CURL_SHA256     = e3de77cfa6367083c776a26647fc9f633b538c06c3baa73d931a252ac8f72b49
MBEDTLS_VERSION = 3.6.7
MBEDTLS_TARBALL = mbedtls-$(MBEDTLS_VERSION).tar.bz2
MBEDTLS_URL     = https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-$(MBEDTLS_VERSION)/$(MBEDTLS_TARBALL)
MBEDTLS_SHA256  = a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6
MBEDTLS_DIR     = mbedtls-$(MBEDTLS_VERSION)
endif

CURL_TARBALL   = curl-$(CURL_VERSION).tar.gz
CURL_URL       = https://github.com/curl/curl/releases/download/curl-$(subst .,_,$(CURL_VERSION))/$(CURL_TARBALL)

# Relative paths, since make cannot handle spaces in target names.
CURL_DIR    = curl-$(CURL_VERSION)
PREFIX_DIR  = build_target

TARGET = chimera-curl

TARGET_CFLAGS = $(CFLAGS) -std=gnu99

# -fPIC only when not building statically against uClibc (IS_UCLIBC is set in mk.config)
ifneq ($(IS_UCLIBC), 1)
    TARGET_CFLAGS += -fPIC
endif

# Only the flags curl needs, not Chimera's own libraries.
#
# mbedTLS is passed to curl's configure as plain -I and -L flags rather
# than --with-mbedtls=DIR: configure splits DIR at spaces, so in a
# directory like "Linux ext2" it would not find mbedTLS and quietly build
# a curl without HTTPS. The paths are relative: configure runs its tests
# in $(CURL_DIR) and the build runs in $(CURL_DIR)/lib and $(CURL_DIR)/src,
# so a link $(CURL_DIR)/$(PREFIX_DIR) -> ../$(PREFIX_DIR) makes the same
# "../$(PREFIX_DIR)" valid from both levels.
MBEDTLS_CPPFLAGS = -I../$(PREFIX_DIR)/include
CURL_LDFLAGS = $(HELPER_LDFLAGS) -L../$(PREFIX_DIR)/lib

MBEDTLS_LIBS = $(PREFIX_DIR)/lib/libmbedcrypto.a $(PREFIX_DIR)/lib/libmbedx509.a $(PREFIX_DIR)/lib/libmbedtls.a
CURL_SRC_BIN = $(CURL_DIR)/src/curl

.PHONY: all install static-curl install-static-curl cacert clean distclean

all:
	@echo "helpers/curl: using the system curl; see 'make static-curl' for a static one"

install:
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 https-helper.sh $(DESTDIR)$(BINDIR)/

static-curl: $(TARGET)

install-static-curl: $(TARGET) install
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

# Root certificates as published by the curl project (from Mozilla's
# list). Fetched with the system curl over a verified connection; run it
# again now and then, as root certificates change over the years.
CACERT_URL = https://curl.se/ca/cacert.pem

cacert:
	install -d $(DESTDIR)$(USERDATADIR)
	curl -fsSL -o $(DESTDIR)$(USERDATADIR)/cacert.pem $(CACERT_URL)
	@echo "installed $(USERDATADIR)/cacert.pem"

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

# 0b. mbedTLS: download the release tarball, check it, unpack it
#     (LEGACY: check out the release tag and check its commit)
$(MBEDTLS_DIR)/Makefile:
ifeq ($(LEGACY),1)
	[ -d $(MBEDTLS_DIR) ] || git clone --depth 1 --branch $(MBEDTLS_TAG) $(MBEDTLS_REPO) $(MBEDTLS_DIR)
	test "`git -C $(MBEDTLS_DIR) rev-parse HEAD`" = "$(MBEDTLS_COMMIT)"
else
	[ -f $(MBEDTLS_TARBALL) ] || curl -fL -o $(MBEDTLS_TARBALL) $(MBEDTLS_URL) \
		|| wget -O $(MBEDTLS_TARBALL) $(MBEDTLS_URL)
	echo "$(MBEDTLS_SHA256)  $(MBEDTLS_TARBALL)" | sha256sum -c -
	bzip2 -dc $(MBEDTLS_TARBALL) | tar xf -
endif
	touch $@

# 1. build mbedTLS static and install it locally in build_target
$(MBEDTLS_LIBS): $(MBEDTLS_DIR)/Makefile
	$(MAKE) lib -C $(MBEDTLS_DIR) CC="$(CC)" CFLAGS="$(TARGET_CFLAGS)"
	mkdir -p $(PREFIX_DIR)/lib $(PREFIX_DIR)/include
	cp -r $(MBEDTLS_DIR)/include/mbedtls $(PREFIX_DIR)/include/
	if [ -d $(MBEDTLS_DIR)/include/psa ]; then cp -r $(MBEDTLS_DIR)/include/psa $(PREFIX_DIR)/include/; fi
	cp $(MBEDTLS_DIR)/library/libmbedcrypto.a $(PREFIX_DIR)/lib/
	cp $(MBEDTLS_DIR)/library/libmbedx509.a $(PREFIX_DIR)/lib/
	cp $(MBEDTLS_DIR)/library/libmbedtls.a $(PREFIX_DIR)/lib/

# 2. configure and build curl against the local mbedTLS, HTTP and HTTPS only
$(CURL_SRC_BIN): $(CURL_DIR)/configure $(MBEDTLS_LIBS)
	ln -sfn ../$(PREFIX_DIR) $(CURL_DIR)/$(PREFIX_DIR)
	cd $(CURL_DIR) && ./configure \
		--disable-shared \
		--enable-static \
		--with-mbedtls \
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
		--disable-docs \
		--without-libpsl \
		--without-libidn2 \
		--without-nghttp2 \
		--without-nghttp3 \
		--without-ngtcp2 \
		--without-brotli \
		--without-zstd \
		--without-libssh2 \
		$(CURL_EXTRA_FLAGS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CPPFLAGS="$(MBEDTLS_CPPFLAGS)" \
		LDFLAGS="$(CURL_LDFLAGS)" \
		CC="$(CC)"
	$(MAKE) -C $(CURL_DIR)

# 3. copy the binary here under its installed name, after checking that
#    it really has HTTPS: configure carries on without it if mbedTLS is
#    not found.
$(TARGET): $(CURL_SRC_BIN)
	@$(CURL_SRC_BIN) -V | grep -q '^Protocols:.* https' \
		|| { echo "helpers/curl: curl was built without HTTPS (mbedTLS not found)" >&2; exit 1; }
	cp $(CURL_SRC_BIN) ./$(TARGET)

clean:
	@# only library/ is built; mbedTLS's own top-level clean uses
	@# "find -exec ... {} +", which older find versions do not support
	if [ -d $(MBEDTLS_DIR)/library ]; then $(MAKE) -C $(MBEDTLS_DIR)/library clean; fi
	if [ -f $(CURL_DIR)/Makefile ]; then $(MAKE) -C $(CURL_DIR) clean; fi
	rm -rf $(PREFIX_DIR) $(TARGET)

# also remove the downloaded sources
distclean: clean
	rm -rf $(CURL_DIR) $(MBEDTLS_DIR) $(CURL_TARBALL) $(MBEDTLS_TARBALL)
