ROOT=../..
RESDIR=$(DOCDIR)/resources

all:

install_all:
	$(SCCBOX) mkdir -p $(RESDIR)
	$(SCCBOX) $(HOW) -d *.png *.svg *.jpg $(RESDIR)

install:
	$(MAKE) install_all HOW="install -f -d"

linstall:
	$(MAKE) install_all HOW="install -f -l -d"

uninstall:
	$(MAKE) install_all HOW="install -u"

clean:

distclean:


include $(ROOT)/Makefile.conf
