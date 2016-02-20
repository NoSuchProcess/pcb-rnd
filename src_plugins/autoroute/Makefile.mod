append /local/pcb/autoroute/enable {}
append /local/pcb/autoroute/buildin {}

put /local/pcb/mod {autoroute}
put /local/pcb/mod/OBJS [@ $(PLUGDIR)/autoroute/autoroute.o $(PLUGDIR)/autoroute/mtspace.o $(PLUGDIR)/autoroute/action.o @]

if /local/pcb/autoroute/enable then
	if /local/pcb/autoroute/buildin then
		include {Makefile.in.mod/Buildin}
	else
		include {Makefile.in.mod/Plugin}
	end
end
