# plain old hand crafted Makefile
ROOT=..

all:
	cd gsch2pcb-rnd && $(MAKE) all
	cd bxl2txt && $(MAKE) all

include $(ROOT)/Makefile.conf

clean:
	cd gsch2pcb-rnd && $(MAKE) clean
	cd bxl2txt && $(MAKE) clean

distclean:
	cd gsch2pcb-rnd && $(MAKE) distclean
	cd bxl2txt && $(MAKE) distclean

install_all:
	$(SCCBOX) mkdir -p  "$(BINDIR)" "$(LIBDIR)"
	$(SCCBOX) $(HOW) "fp2preview" "$(BINDIR)/fp2preview"
	$(SCCBOX) $(HOW) "fp2subc" "$(BINDIR)/fp2subc"
	$(SCCBOX) $(HOW) "pcb-prj2lht" "$(BINDIR)/pcb-prj2lht"
	$(SCCBOX) $(HOW) "pcb-rnd-svg" "$(BINDIR)/pcb-rnd-svg"
	$(SCCBOX) $(HOW) "gnet-pcbrndfwd.scm" "$(LIBDIR)/gnet-pcbrndfwd.scm"
	$(SCCBOX) $(HOW) "gnet-pcbrndfwd_elem.scm" "$(LIBDIR)/gnet-pcbrndfwd_elem.scm"

install:
	$(MAKE) install_all HOW="install -f"
	cd gsch2pcb-rnd && $(MAKE) install
	cd bxl2txt && $(MAKE) install

linstall:
	$(MAKE) install_all HOW="linstall -f"
	cd gsch2pcb-rnd && $(MAKE) linstall
	cd bxl2txt && $(MAKE) linstall

uninstall:
	$(MAKE) install_all HOW="uninstall"
	cd gsch2pcb-rnd && $(MAKE) uninstall
	cd bxl2txt && $(MAKE) uninstall

