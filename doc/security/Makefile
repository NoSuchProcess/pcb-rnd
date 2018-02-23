ROOT=../..
SECDIR=$(DOCDIR)/security


all:

install_all:
	$(SCCBOX) mkdir -p $(SECDIR)
	$(SCCBOX) $(HOW) *.html $(SECDIR)/
	$(SCCBOX) $(HOW) README $(SECDIR)/

install:
	$(MAKE) install_all HOW="install -f -d"

linstall:
	$(MAKE) install_all HOW="install -f -l -d"

uninstall:
	$(MAKE) install_all HOW="install -u"

clean:

distclean:


include $(ROOT)/Makefile.conf
