# plain old hand crafted Makefile

SCMDIR=$(DATADIR)/../gEDA/scheme

all:
	cd gsch2pcb-rnd && make all

include ../Makefile.conf

clean:
	cd gsch2pcb-rnd && make clean

install_:
	$(MKDIR) "$(BINDIR)" "$(LIBDIR)" "$(SCMDIR)"
	$(CPC) "`pwd`/fp2anim" "$(BINDIR)/fp2anim"
	$(CPC) "`pwd`/pcb-strip" "$(BINDIR)/pcb-strip"
	$(CPC) "`pwd`/gnet-pcbrndfwd.scm" "$(SCMDIR)/gnet-pcbrndfwd.scm"
	cd gsch2pcb-rnd && make install_ CPC="$(CPC)"

install:
	make install_ CPC="$(CP)"

linstall:
	make install_ CPC="$(LN)"

uninstall:
	$(RM) "$(BINDIR)/fp2anim"
	$(RM) "$(BINDIR)/pcb-strip"
	$(RM) "$(SCMDIR)/gnet-pcbrndfwd.scm"
	cd gsch2pcb-rnd && make uninstall
