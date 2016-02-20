append /local/pcb/pcb_gpmi/enable {}
append /local/pcb/pcb_gpmi/buildin {}

append /local/pcb/CLEANRULES  {clean_gpmi}

append /local/pcb/TOPVARS [@
PCB_GPMI=${PLUGDIR}/gpmi/pcb-gpmi
@]

append /local/pcb/RULES [@
### gpmi_plugin
clean_gpmi: FORCE
	cd $(PCB_GPMI) && make clean
@]


append /local/pcb/rules/install     {	cd $(PCB_GPMI) && make install} {
}
append /local/pcb/rules/linstall    {	cd $(PCB_GPMI) && make linstall} {
}
append /local/pcb/rules/uninstall   {	cd $(PCB_GPMI) && make uninstall} {
}


if /target/libs/script/gpmi/presents then
	if /local/pcb/gpmi/buildin then
		append /local/pcb/RULES [@

mod_pcb_gpmi: all

@]

		append /local/pcb/LIBS        {$(PCB_GPMI)/gpmi_plugin/gpmi_buildin.a}
		append /local/pcb/EXEDEPS     {$(PCB_GPMI)/gpmi_plugin/gpmi_buildin.a}
		append /local/pcb/LDFLAGS      /target/libs/script/gpmi/ldflags

		append /local/pcb/buildin_init { extern void hid_gpmi_init(); hid_gpmi_init(); plugin_register("gpmi", "<gpmi>", NULL, 0); }

#		append /local/pcb/CFLAGS   /target/libs/script/gpmi/cflags
#		append /local/pcb/LDFLAGS  /target/libs/script/gpmi/ldflags
#		append /local/pcb/LIBS     /target/libs/script/gpmi/libs

		append /local/pcb/RULES [@

$(PCB_GPMI)/gpmi_plugin/gpmi_buildin.a: FORCE
	cd $(PCB_GPMI)/gpmi_plugin && make all_buildin

@]

	else
		append /local/pcb/all   [@ $(PCB_GPMI)/gpmi_plugin/pcb_gpmi.so @]
		append /local/pcb/RULES [@

mod_pcb_gpmi: $(PCB_GPMI)/gpmi_plugin/pcb_gpmi.so

@]

		append /local/pcb/RULES [@

$(PCB_GPMI)/gpmi_plugin/pcb_gpmi.so: FORCE
	cd $(PCB_GPMI)/gpmi_plugin && make all_plugin

@]

	end
end
