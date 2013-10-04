all:

install_:
	$(MKDIR) $(DOCDIR)/examples
	$(CPC) `pwd`/tut1.pcb $(DOCDIR)/examples/tut1.pcb

install:
	make install_ CPC="$(CP)"

linstall:
	make install_ CPC="$(LN)"

uninstall:
	$(RM) $(DOCDIR)/examples/tut1.pcb

include ../Makefile.conf
