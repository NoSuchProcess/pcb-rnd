append /local/pcb/autoplace/enable {}
append /local/pcb/autoplace/buildin {}

append /local/pcb/autoplace/OBJS [@ $(PLUGDIR)/autoplace/autoplace.o $(PLUGDIR)/autoplace/action.o @]

if /local/pcb/autoplace/enable then
	if /local/pcb/autoplace/buildin then
		append /local/pcb/buildin_init    { extern void hid_autoplace_init(); hid_autoplace_init(); plugin_register("autoplace", "<autoplace>", NULL, 0); }
		append /local/pcb/OBJS            /local/pcb/autoplace/OBJS
		append /local/pcb/RULES [@

mod_autoplace: all

@]

	else
		append /local/pcb/all   [@ $(PLUGDIR)/autoplace/autoplace.so @]
		append /local/pcb/RULES [@

$(PLUGDIR)/autoplace/autoplace.so: @/local/pcb/autoplace/OBJS@
	$(CC) $(LDFLAGS) -shared @cc/rdynamic@ -o $(PLUGDIR)/autoplace/autoplace.so @/local/pcb/autoplace/OBJS@

mod_autoplace: $(PLUGDIR)/autoplace/autoplace.so

@]
	end
end
