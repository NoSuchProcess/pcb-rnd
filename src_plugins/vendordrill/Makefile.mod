append /local/pcb/vendordrill/enable {}
append /local/pcb/vendordrill/buildin {}

append /local/pcb/vendordrill/OBJS [@ ${PLUGDIR}/vendordrill/vendor.o @]

if /local/pcb/vendordrill/enable then
	if /local/pcb/vendordrill/buildin then
		append /local/pcb/buildin_init    { extern void hid_vendordrill_init(); hid_vendordrill_init(); plugin_register("vendordrill", "<vendordrill>", NULL, 0); }
		append /local/pcb/OBJS            /local/pcb/vendordrill/OBJS
		append /local/pcb/RULES [@

mod_vendordrill: all

@]

	else
		append /local/pcb/all   [@ ${PLUGDIR}/vendordrill/vendordrill.so @]
		append /local/pcb/RULES [@

${PLUGDIR}/vendordrill/vendordrill.so: @/local/pcb/vendordrill/OBJS@
	$(CC) $(LDFLAGS) -shared @cc/rdynamic@ -o ${PLUGDIR}/vendordrill/vendordrill.so @/local/pcb/vendordrill/OBJS@

mod_vendordrill: ${PLUGDIR}/vendordrill/vendordrill.so

@]
	end
end
