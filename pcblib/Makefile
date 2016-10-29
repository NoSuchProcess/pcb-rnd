# This Makefile is a plain old hand written one; all configuration settings
# are included from ../Makefile.conf which is scconfig generated

all:

# NOTE: this rule is _not_ called from linstall
install_:
	$(MKDIR) "$(DATADIR)/pcblib"
	$(MKDIR) "$(DATADIR)/pcblib/connector"
	$(CPC) "`pwd`/connector"/* "$(DATADIR)/pcblib/connector"
	$(MKDIR) "$(DATADIR)/pcblib/parametric"
	$(CPC) "`pwd`/parametric"/* "$(DATADIR)/pcblib/parametric"
	$(MKDIR) "$(DATADIR)/pcblib/smd"
	$(CPC) "`pwd`/smd"/* "$(DATADIR)/pcblib/smd"
	$(MKDIR) "$(DATADIR)/pcblib/tru-hole"
	$(CPC) "`pwd`/tru-hole"/* "$(DATADIR)/pcblib/tru-hole"

install:
	make install_ CPC="$(CP)"

# hack: pcb's chdir() based approach gets fooled on symlinks because of "cd .."
# returns to the wrong dir - rather symlink the whole dir
linstall:
	make uninstall
	$(MKDIR) "$(DATADIR)"
	$(LN) "`pwd`" "$(DATADIR)/pcblib"

uninstall:
	$(RM) "$(DATADIR)/pcblib"

clean:

include ../Makefile.conf

