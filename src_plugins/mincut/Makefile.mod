append /local/pcb/mincut/enable {}
append /local/pcb/mincut/buildin {}

append /local/pcb/mincut/OBJS [@ ${PLUGDIR}/mincut/rats_mincut.o ${PLUGDIR}/mincut/pcb-mincut/graph.o ${PLUGDIR}/mincut/pcb-mincut/solve.o @]

if /local/pcb/mincut/enable then
	if /local/pcb/mincut/buildin then
		append /local/pcb/buildin_init    { extern void hid_mincut_init(); hid_mincut_init(); plugin_register("mincut", "<mincut>", NULL, 0); }
		append /local/pcb/OBJS            /local/pcb/mincut/OBJS
		append /local/pcb/RULES [@

mod_mincut: all

@]

	else
		append /local/pcb/all   [@ ${PLUGDIR}/mincut/mincut.so @]
		append /local/pcb/RULES [@

${PLUGDIR}/mincut/mincut.so: @/local/pcb/mincut/OBJS@
	$(CC) $(LDFLAGS) -shared @cc/rdynamic@ -o ${PLUGDIR}/mincut/mincut.so @/local/pcb/mincut/OBJS@

mod_mincut: ${PLUGDIR}/mincut/mincut.so

@]
	end
end
