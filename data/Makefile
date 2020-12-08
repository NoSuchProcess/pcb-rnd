# This Makefile is a plain old hand written one; all configuration settings
# are included from ../Makefile.conf which is scconfig generated

all:

install_:
	echo "No installation script. Please contribute!"

install:
	$(MAKE) install_ CPC="$(CP)"

linstall:
	$(MAKE) install_ CPC="$(LN)"

uninstall:
	$(RM) $(DOCDIR)/examples/tut1.pcb

include ../Makefile.conf
themedir=$(DATADIR)/icons/$(theme)

