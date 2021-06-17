# This Makefile is a plain old hand written one; all configuration settings
# are included from ../Makefile.conf which is scconfig generated

ROOT=..

all:

# NOTE: this rule is _not_ called from linstall
install_:
	$(MKDIR) "$(DATADIR)/footprint"
	$(MKDIR) "$(DATADIR)/footprint/connector"
	$(CPC) "`pwd`/connector"/* "$(DATADIR)/footprint/connector"
	$(MKDIR) "$(DATADIR)/footprint/parametric"
	$(CPC) "`pwd`/parametric"/* "$(DATADIR)/footprint/parametric"
	$(MKDIR) "$(DATADIR)/footprint/smd"
	$(CPC) "`pwd`/smd"/* "$(DATADIR)/footprint/smd"
	$(MKDIR) "$(DATADIR)/footprint/thru-hole"
	$(CPC) "`pwd`/thru-hole"/* "$(DATADIR)/footprint/thru-hole"

install:
	$(MAKE) install_ CPC="$(CP)"

# hack: pcb's chdir() based approach gets fooled on symlinks because of "cd .."
# returns to the wrong dir - rather symlink the whole dir
linstall:
	$(MAKE) uninstall
	$(MKDIR) "$(DATADIR)"
	$(LN) "`pwd`" "$(DATADIR)/footprint"

uninstall:
	$(RM) "$(DATADIR)/footprint"

clean:

include $(ROOT)/Makefile.conf

