include ../mk.config

SCRIPTS = dotelnet do3270 pstopxm mailto.pl chimera-open

all:
	@echo "Nothing to build (only script install)"

# Scripts used by the default mailcap, convert and protocol files.
# Files named NAME.script are installed as NAME.
install:
	install -d $(DESTDIR)$(BINDIR)
	@for f in $(SCRIPTS); do \
		if [ -f $$f.script ]; then src=$$f.script; else src=$$f; fi; \
		echo "install $$src -> $(BINDIR)/$$f"; \
		install -m 755 $$src $(DESTDIR)$(BINDIR)/$$f; \
	done

clean:
	@echo "Nothing to clean in util"
