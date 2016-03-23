PCB_GPMI_ROOT=.

all:
	cd gpmi_plugin; make all

include ../../../Makefile.conf
PLUGIN_DIR=$(LIBDIR)/plugins

clean:
	cd gpmi_plugin; make clean

test: all
	cd host_lib; ./test.sh


# TODO: temporary code until gpmi-config is fixed to generate install rules
install:
	-mkdir -p $(PLUGIN_DIR)/pcb-rnd-gpmi/
	if test -f gpmi_plugin/gpmi_plugin.so; then cp gpmi_plugin/gpmi_plugin.so $(PLUGIN_DIR)/gpmi_plugin.so; fi
	cp `ls gpmi_plugin/gpmi_pkg/*.h gpmi_plugin/gpmi_pkg/*.so` $(PLUGIN_DIR)/pcb-rnd-gpmi/

linstall:
	-mkdir -p $(PLUGIN_DIR)/pcb-rnd-gpmi/
	if test -f gpmi_plugin/gpmi_plugin.so; then -ln -sf "`pwd`/gpmi_plugin/gpmi_plugin.so" "$(PLUGIN_DIR)/gpmi_plugin.so"; fi
	-for n in `ls gpmi_plugin/gpmi_pkg/*.h gpmi_plugin/gpmi_pkg/*.so`; do ln -sf "`pwd`/$$n"  "$(PLUGIN_DIR)/pcb-rnd-gpmi/`basename $$n`"; done

