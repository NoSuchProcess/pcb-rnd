# This Makefile is a plain old hand written one; all configuration settings
# are included from ../Makefile.conf which is scconfig generated

all:

install_:
	$(MKDIR) $(DATADIR)/pcblib
	$(CPC) `pwd`/connector $(DATADIR)/pcblib/connector
	$(CPC) `pwd`/parametric $(DATADIR)/pcblib/parametric
	$(CPC) `pwd`/smd $(DATADIR)/pcblib/smd
	$(CPC) `pwd`/tru-hole $(DATADIR)/pcblib/tru-hole

install:
	make install_ CPC="$(CP)"

linstall:
	make uninstall
	make install_ CPC="$(LN)"

uninstall:
	$(RM) $(DATADIR)/pcblib

clean:

include ../Makefile.conf

