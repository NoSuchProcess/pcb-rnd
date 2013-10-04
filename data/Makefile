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
	$(app_icon:%=%-48.png) \
	$(app_icon:%=%.svg)
#	$(app_icon:%=%-16.png)
#	$(app_icon:%=%-22.png)
#	$(app_icon:%=%-24.png)
#	$(app_icon:%=%-32.png)

mime_icon_files = \
	$(mime_icons:%=%-16.png) \
	$(mime_icons:%=%-22.png) \
	$(mime_icons:%=%-24.png) \
	$(mime_icons:%=%-32.png) \
	$(mime_icons:%=%-48.png) \
	$(mime_icons:%=%.svg)

mime_icon_sources = \
	$(mime_icons:%=%-16.svg) \
	$(mime_icons:%=%-22.svg) \
	$(mime_icons:%=%-32.svg) \
	$(mime_icons:%=%-48.svg)

theme_icons = \
	$(mime_icon_files:%=mimetypes,%) \
	$(app_icon_files:%=apps,%)

all:

install_:
	./icon-theme-installer \
		-t $(theme) \
		-m "$(MKDIR)" \
		-s `pwd` \
		-d x \
		-b "$(themedir)" \
		-x "$(CPC)" \
		-i $(theme_icons)

install:
	make install_ CPC="$(CP)"

linstall:
	make install_ CPC="$(LN)"

uninstall:
	$(RM) $(DOCDIR)/examples/tut1.pcb

include ../Makefile.conf
themedir=$(DATADIR)/icons/$(theme)

