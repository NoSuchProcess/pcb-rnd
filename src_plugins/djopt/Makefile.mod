append /local/pcb/djopt/enable {}
append /local/pcb/djopt/buildin {}

put /local/pcb/mod {djopt}
append /local/pcb/mod/OBJS [@ $(PLUGDIR)/djopt/djopt.o @]

if /local/pcb/djopt/enable then
		include {Makefile.in.mod/Buildin}
	else
		include {Makefile.in.mod/Plugin}
	end
end
