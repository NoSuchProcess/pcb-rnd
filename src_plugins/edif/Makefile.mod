append /local/pcb/edif/enable {}
append /local/pcb/edif/buildin {}

append /local/pcb/edif/OBJS [@ $(PLUGDIR)/edif/edif.o @]
append /local/pcb/YACC {edif}

if /local/pcb/edif/enable then
	if /local/pcb/edif/buildin then
		append /local/pcb/buildin_init    { extern void hid_edif_init(); hid_edif_init(); plugin_register("edif", "<edif>", NULL, 0); }
		append /local/pcb/OBJS            /local/pcb/edif/OBJS
		append /local/pcb/RULES [@

mod_edif: all

@]

	else
		append /local/pcb/all   [@ $(PLUGDIR)/edif/edif.so @]
		append /local/pcb/RULES [@

$(PLUGDIR)/edif/edif.so: @/local/pcb/edif/OBJS@
	$(CC) $(LDFLAGS) -shared @cc/rdynamic@ -o $(PLUGDIR)/edif/edif.so @/local/pcb/edif/OBJS@

mod_edif: $(PLUGDIR)/edif/edif.so

@]
	end
end
