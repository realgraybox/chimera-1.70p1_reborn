include ../mk.config

DATAFILES = \
	convert \
	home.html \
	help.html \
	mime.types \
	mailcap \
	protocol \
	cacheinfo
	
SCRIPTS = \
	chimera_print.sh

all:
	@echo "Nothing to build in lib"

install:
	@mkdir -p $(DESTDIR)$(USERDATADIR)
	@for f in $(DATAFILES); do \
		install -m 644 $$f $(DESTDIR)$(USERDATADIR)/; \
	done
	@mkdir -p $(DESTDIR)$(BINDIR)
	@for f in $(SCRIPTS); do \
		install -m 755 $$f $(DESTDIR)$(BINDIR)/; \
	done
	
clean:
	@echo "Nothing to clean in lib"
