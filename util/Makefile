# plain old hand crafted Makefile

all:
	cd gsch2pcb-rnd && $(MAKE) all

include ../Makefile.conf

clean:
	cd gsch2pcb-rnd && $(MAKE) clean

install_:
	$(MKDIR) "$(BINDIR)" "$(LIBDIR)"
	$(CPC) "`pwd`/fp2anim" "$(BINDIR)/fp2anim"
	$(CPC) "`pwd`/pcb-strip" "$(BINDIR)/pcb-strip"
	$(CPC) "`pwd`/pcb-prj2lht" "$(BINDIR)/pcb-prj2lht"
	$(CPC) "`pwd`/gnet-pcbrndfwd.scm" "$(LIBDIR)/gnet-pcbrndfwd.scm"
	$(CPC) "`pwd`/gnet-pcbrndfwd_elem.scm" "$(LIBDIR)/gnet-pcbrndfwd_elem.scm"
	cd gsch2pcb-rnd && $(MAKE) install_ CPC="$(CPC)"

install:
	$(MAKE) install_ CPC="$(CP)"

linstall:
	$(MAKE) install_ CPC="$(LN)"

uninstall:
	$(RM) "$(BINDIR)/fp2anim"
	$(RM) "$(BINDIR)/pcb-strip"
	$(RM) "$(BINDIR)/pcb-prj2lht"
	$(RM) "$(LIBDIR)/gnet-pcbrndfwd.scm"
	$(RM) "$(LIBDIR)/gnet-pcbrndfwd_elem.scm"
	cd gsch2pcb-rnd && $(MAKE) uninstall
