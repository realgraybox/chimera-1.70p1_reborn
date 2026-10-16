include ../mk.config

DATAFILES = \
	convert \
	home.html \
	help.html \
	mime.types \
	mailcap \
	protocol \
	cacheinfo \
	sendto
	
SCRIPTS = \
	chimera_print.sh

# X resources (app-defaults). Xt looks for the user's own resource file
# as $HOME/Chimera, so no XAPPLRESDIR setting is needed.
APPDEFAULTS = $$HOME/Chimera

all:
	@echo "Nothing to build in lib"

install:
	@mkdir -p $(DESTDIR)$(USERDATADIR)
	@for f in $(DATAFILES); do \
		install -m 644 $$f $(DESTDIR)$(USERDATADIR)/; \
	done
	@# the helper paths in protocol follow BINDIR
	sed 's|/usr/bin/|$(BINDIR)/|' protocol > $(DESTDIR)$(USERDATADIR)/protocol
	install -m 644 Chimera $(DESTDIR)$(APPDEFAULTS)
	@mkdir -p $(DESTDIR)$(BINDIR)
	@for f in $(SCRIPTS); do \
		install -m 755 $$f $(DESTDIR)$(BINDIR)/; \
	done
	
clean:
	@echo "Nothing to clean in lib"
