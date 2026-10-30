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
# as $HOME/Chimera, so no XAPPLRESDIR setting is needed. Chimera is the
# dark theme (the default), Chimera-light the light one, which Xt reads
# instead with: chimera -xrm '*customization: -light'
APPDEFAULTS_DIR = $$HOME
THEMES = Chimera Chimera-light

all:
	@echo "Nothing to build in lib"

install:
	@mkdir -p $(DESTDIR)$(USERDATADIR)
	@for f in $(DATAFILES); do \
		install -m 644 $$f $(DESTDIR)$(USERDATADIR)/; \
	done
	@# the helper paths in protocol follow BINDIR
	sed 's|/usr/bin/|$(BINDIR)/|' protocol > $(DESTDIR)$(USERDATADIR)/protocol
	@# a resource file the user has changed is kept as <name>.old
	@for f in $(THEMES); do \
		dest="$(DESTDIR)$(APPDEFAULTS_DIR)/$$f"; \
		if [ -f "$$dest" ] && ! cmp -s "$$f" "$$dest"; then \
			cp -p "$$dest" "$$dest.old"; \
			echo "lib: your previous $$dest is kept as $$dest.old"; \
		fi; \
		install -m 644 "$$f" "$$dest"; \
	done
	@mkdir -p $(DESTDIR)$(BINDIR)
	@for f in $(SCRIPTS); do \
		install -m 755 $$f $(DESTDIR)$(BINDIR)/; \
	done
	
clean:
	@echo "Nothing to clean in lib"
