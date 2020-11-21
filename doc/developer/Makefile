ROOT=../..
DEVDIR=$(DOCDIR)/developer

all:

install_all:
	$(SCCBOX) mkdir -p $(DEVDIR)/alien_formats $(DEVDIR)/ddrc $(DEVDIR)/mods3
	$(SCCBOX) $(HOW) alien_formats/*.txt $(DEVDIR)/alien_formats/
	$(SCCBOX) $(HOW) ddrc/*.txt $(DEVDIR)/ddrc/
	$(SCCBOX) $(HOW) mods3/*.html mods3/*.png $(DEVDIR)/mods3/

install:
	$(MAKE) install_all HOW="install -f -d"

linstall:
	$(MAKE) install_all HOW="install -f -l -d"

uninstall:
	$(MAKE) install_all HOW="install -u"

clean:

distclean:


include $(ROOT)/Makefile.conf
