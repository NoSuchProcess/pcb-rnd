append /local/pcb/mincut/enable {}
append /local/pcb/mincut/buildin {}

put /local/pcb/mod {mincut}
append /local/pcb/mod/OBJS [@ $(PLUGDIR)/mincut/rats_mincut.o $(PLUGDIR)/mincut/pcb-mincut/graph.o $(PLUGDIR)/mincut/pcb-mincut/solve.o @]

if /local/pcb/mincut/enable then
	if /local/pcb/mincut/buildin then
		include {Makefile.in.mod/Buildin}
	else
		include {Makefile.in.mod/Plugin}
	end
end
