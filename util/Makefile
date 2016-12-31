# plain old hand crafted Makefile
ROOT=..

all:
	cd gsch2pcb-rnd && $(MAKE) all

include $(ROOT)/Makefile.conf

clean:
	cd gsch2pcb-rnd && $(MAKE) clean

distclean:
	cd gsch2pcb-rnd && $(MAKE) distclean

install_all:
	$(SCCBOX) mkdir -p  "$(BINDIR)" "$(LIBDIR)"
	$(SCCBOX) $(HOW) "fp2anim" "$(BINDIR)/fp2anim"
	$(SCCBOX) $(HOW) "pcb-strip" "$(BINDIR)/pcb-strip"
	$(SCCBOX) $(HOW) "pcb-prj2lht" "$(BINDIR)/pcb-prj2lht"
	$(SCCBOX) $(HOW) "gnet-pcbrndfwd.scm" "$(LIBDIR)/gnet-pcbrndfwd.scm"
	$(SCCBOX) $(HOW) "gnet-pcbrndfwd_elem.scm" "$(LIBDIR)/gnet-pcbrndfwd_elem.scm"

install:
	$(MAKE) install_all HOW="install -f"
	cd gsch2pcb-rnd && $(MAKE) install

linstall:
	$(MAKE) install_all HOW="linstall -f"
	cd gsch2pcb-rnd && $(MAKE) linstall

uninstall:
	$(MAKE) install_all HOW="uninstall"
	cd gsch2pcb-rnd && $(MAKE) uninstall

