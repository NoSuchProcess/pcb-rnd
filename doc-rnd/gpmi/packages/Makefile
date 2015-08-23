ROOT=../../..
ALL= actions dialogs hid layout

all: XREF $(ALL:%=%_ref.html)

include $(ROOT)/src/pcb-gpmi/Makefile.config

%_ref.html REF.% : $(ROOT)/src/pcb-gpmi/gpmi_plugin/gpmi_pkg/%.h
	$(ROOT)/util/genref.sh "$^" "$(ROOT)"  "../packages/$@" $(PCB_CFLAGS) >$*_ref.html

XREF: $(ALL:%=REF.%)
	cat $^ > $@


