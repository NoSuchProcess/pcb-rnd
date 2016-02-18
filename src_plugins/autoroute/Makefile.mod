append /local/pcb/autorouter/enable {}
append /local/pcb/autorouter/buildin {}

if /local/pcb/autorouter/enable then
if /local/pcb/autorouter/buildin then
append /local/pcb/OBJS            [@ ${PLUGDIR}/autoroute/autoroute.o ${PLUGDIR}/autoroute/action.o @]
append /local/pcb/ACTION_REG_SRC  [@ ${PLUGDIR}/autoroute/action.c @]
end
end
