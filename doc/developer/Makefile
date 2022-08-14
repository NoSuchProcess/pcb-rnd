ROOT=../..
DEVDIR=$(DOCDIR)/developer

all:

install_all:
	$(SCCBOX) mkdir -p $(DEVDIR)/alien_formats $(DEVDIR)/alien_formats/io_altium $(DEVDIR)/ddrc $(DEVDIR)/mods3 $(DEVDIR)/API $(DEVDIR)/export_openems $(DEVDIR)/lihata_format $(DEVDIR)/lihata_format/examples $(DEVDIR)/plugin $(DEVDIR)/plugin/template $(DEVDIR)/plugin/template_fungw $(DEVDIR)/plugin/template_pup
	$(SCCBOX) $(HOW) *.txt *.html *.svg $(DEVDIR)/
	$(SCCBOX) $(HOW) alien_formats/*.txt $(DEVDIR)/alien_formats/
	$(SCCBOX) $(HOW) alien_formats/io_altium/*.txt $(DEVDIR)/alien_formats/io_altium/
	$(SCCBOX) $(HOW) alien_formats/io_kicad/*.txt $(DEVDIR)/alien_formats/io_kicad/
	$(SCCBOX) $(HOW) ddrc/*.txt $(DEVDIR)/ddrc/
	$(SCCBOX) $(HOW) mods3/*.html mods3/*.png $(DEVDIR)/mods3/
	$(SCCBOX) $(HOW) API/*.txt $(DEVDIR)/API/
	$(SCCBOX) $(HOW) export_openems/*.txt $(DEVDIR)/export_openems/
	$(SCCBOX) $(HOW) lihata_format/*.html lihata_format/*.txt lihata_format/*.svg $(DEVDIR)/lihata_format/
	$(SCCBOX) $(HOW) lihata_format/examples/*.lht $(DEVDIR)/lihata_format/examples/
	$(SCCBOX) $(HOW) plugin/*.html plugin/*.h plugin/*.conf $(DEVDIR)/plugin/
	$(SCCBOX) $(HOW) plugin/template/* $(DEVDIR)/plugin/template/
	$(SCCBOX) $(HOW) plugin/template_fungw/* $(DEVDIR)/plugin/template_fungw/
	$(SCCBOX) $(HOW) plugin/template_pup/* $(DEVDIR)/plugin/template_pup/

install:
	$(MAKE) install_all HOW="install -f -d"

linstall:
	$(MAKE) install_all HOW="install -f -l -d"

uninstall:
	$(MAKE) install_all HOW="install -u"

clean:

distclean:


include $(ROOT)/Makefile.conf
