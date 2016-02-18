append /local/pcb/autorouter/enable {}
append /local/pcb/autorouter/buildin {}

append /local/pcb/autorouter/OBJS [@ ${PLUGDIR}/autoroute/autoroute.o ${PLUGDIR}/autoroute/action.o @]

if /local/pcb/autorouter/enable then
	if /local/pcb/autorouter/buildin then
		append /local/pcb/OBJS            /local/pcb/autorouter/OBJS
		append /local/pcb/RULES [@

mod_autoroute: all

@]

	else
		append /local/pcb/all   [@ ${PLUGDIR}/autoroute/autoroute.so @]
		append /local/pcb/RULES [@

${PLUGDIR}/autoroute/autoroute.so: @/local/pcb/autorouter/OBJS@
	$(CC) $(LDFLAGS) -shared @cc/rdynamic@ -o ${PLUGDIR}/autoroute/autoroute.so @/local/pcb/autorouter/OBJS@

mod_autoroute: ${PLUGDIR}/autoroute/autoroute.so

@]
	end
end
