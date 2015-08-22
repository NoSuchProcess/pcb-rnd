ROOT=../../..
all: actions_ref.html dialogs_ref.html hid_ref.html layout_ref.html

include $(ROOT)/src/pcb-gpmi/Makefile.config

%_ref.html: $(ROOT)/src/pcb-gpmi/gpmi_plugin/gpmi_pkg/%.h
	$(ROOT)/util/genref.sh $^ $(ROOT) $(PCB_CFLAGS) >$@


