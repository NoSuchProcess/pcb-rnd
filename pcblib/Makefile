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

# hack: pcb's chdir() based approach gets fooled on symlinks because of "cd .."
# returns to the wrong dir
linstall:
	make uninstall
	$(MKDIR) $(DATADIR)
	$(LN) `pwd` $(DATADIR)/pcblib

uninstall:
	$(RM) $(DATADIR)/pcblib

clean:

include ../Makefile.conf

