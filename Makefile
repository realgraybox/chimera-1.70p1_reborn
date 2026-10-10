# Top-level Makefile
include mk.config

PACKAGE = chimera-browser
VERSION = 1.70
DISTDIR = $(PACKAGE)-$(VERSION)-20260626


LDFLAGS ?=
SUBDIRS = compat common lib libhtmlw mxw util xloadimage XawPlus-2.0.2 src \
			helpers/fits2gif helpers/svg2gif helpers/curl \
			helpers/menu helpers/text2pdf helpers/xvl

.PHONY: all install clean dist

all:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

install:
	@for dir in util lib src helpers/curl helpers/fits2gif helpers/img2gif helpers/svg2gif helpers/tif2gif; do \
		$(MAKE) -C $$dir install DESTDIR=$(DESTDIR); \
	done

# run with make dist 
dist: clean
	@echo "Creating distribution archive in /tmp..."
	rm -rf /tmp/$(DISTDIR)
	mkdir /tmp/$(DISTDIR)
	# copy all files - exclude git
	cp -R * /tmp/$(DISTDIR)/
	rm -rf /tmp/$(DISTDIR)/.git
	# create tar.gz
	tar -cvzf $(DISTDIR).tar.gz -C /tmp $(DISTDIR)
	# clean up temp folder
	rm -rf /tmp/$(DISTDIR)
	@echo "Finished: $(DISTDIR).tar.gz"

#todo - do not use
uninstall:
	@echo "Removing $(TARGET) from $(PREFIX)..."
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -rf $(DESTDIR)$(SHAREDIR)
	# rm -f $(DESTDIR)$(DESKTOPDIR)/chimera-browser.desktop
	# rm -f $(DESTDIR)$(MANDIR)/chimera-browser.1
	@echo "Uninstallation finished."
