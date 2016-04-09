ROOT=../../..
GPMIDIR=$(ROOT)/src_plugins/gpmi/pcb-gpmi
ALL= actions dialogs hid layout

all: XREF $(ALL:%=%_ref.html)

include $(GPMIDIR)/Makefile.config

%_ref.html REF.% : $(GPMIDIR)/gpmi_plugin/gpmi_pkg/%.h
	$(ROOT)/util/genref.sh "$^" "$(ROOT)"  "../packages/$@" $(PCB_CFLAGS) >$*_ref.html

XREF: $(ALL:%=REF.%)
	cat $^ > $@


