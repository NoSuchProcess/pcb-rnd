ROOT=../..
CFDIR=$(DOCDIR)/conf


all:

install_all:
	$(SCCBOX) mkdir -p $(CFDIR)/tree
	$(SCCBOX) $(HOW) *.html *.png $(CFDIR)/
	$(SCCBOX) $(HOW) tree/*.html $(CFDIR)/tree/


install:
	$(MAKE) install_all HOW="install -f -d"

linstall:
	$(MAKE) install_all HOW="install -f -l -d"

uninstall:
	$(MAKE) install_all HOW="install -u"

clean:

distclean:


include $(ROOT)/Makefile.conf
