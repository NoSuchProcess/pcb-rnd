append /local/pcb/puller/enable {}
append /local/pcb/puller/buildin {}

append /local/pcb/puller/OBJS [@ ${PLUGDIR}/puller/puller.o @]

if /local/pcb/puller/enable then
	if /local/pcb/puller/buildin then
		append /local/pcb/buildin_init    { extern void hid_puller_init(); hid_puller_init(); plugin_register("puller", "<puller>", NULL, 0); }
		append /local/pcb/OBJS            /local/pcb/puller/OBJS
		append /local/pcb/RULES [@

mod_puller: all

@]

	else
		append /local/pcb/all   [@ ${PLUGDIR}/puller/puller.so @]
		append /local/pcb/RULES [@

${PLUGDIR}/puller/puller.so: @/local/pcb/puller/OBJS@
	$(CC) $(LDFLAGS) -shared @cc/rdynamic@ -o ${PLUGDIR}/puller/puller.so @/local/pcb/puller/OBJS@

mod_puller: ${PLUGDIR}/puller/puller.so

@]
	end
end
