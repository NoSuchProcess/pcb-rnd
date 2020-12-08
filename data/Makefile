# This Makefile is a plain old hand written one; all configuration settings
# are included from ../Makefile.conf which is scconfig generated

theme = hicolor

app_icon = pcb

mime_icons = \
	application-x-pcb-layout \
	application-x-pcb-footprint \
	application-x-pcb-netlist \
	application-x-gerber \
	application-x-excellon

app_icon_files = \

mime_icon_files = \

mime_icon_sources = \

theme_icons = \
	$(mime_icon_files:%=mimetypes,%) \
	$(app_icon_files:%=apps,%)

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

