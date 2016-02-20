append /local/pcb/djopt/enable {}
append /local/pcb/djopt/buildin {}

append /local/pcb/djopt/OBJS [@ $(PLUGDIR)/djopt/djopt.o @]

if /local/pcb/djopt/enable then
	if /local/pcb/djopt/buildin then
		append /local/pcb/buildin_init    { extern void hid_djopt_init(); hid_djopt_init(); plugin_register("djopt", "<djopt>", NULL, 0); }
		append /local/pcb/OBJS            /local/pcb/djopt/OBJS
		append /local/pcb/RULES [@

mod_djopt: all

@]

	else
		append /local/pcb/all   [@ $(PLUGDIR)/djopt/djopt.so @]
		append /local/pcb/RULES [@

$(PLUGDIR)/djopt/djopt.so: @/local/pcb/djopt/OBJS@
	$(CC) $(LDFLAGS) -shared @cc/rdynamic@ -o $(PLUGDIR)/djopt/djopt.so @/local/pcb/djopt/OBJS@

mod_djopt: $(PLUGDIR)/djopt/djopt.so

@]
	end
end
