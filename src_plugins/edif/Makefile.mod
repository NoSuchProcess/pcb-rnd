append /local/pcb/edif/enable {}
append /local/pcb/edif/buildin {}

put /local/pcb/mod {edif}
append /local/pcb/mod/OBJS [@ $(PLUGDIR)/edif/edif.o @]
append /local/pcb/YACC {edif}

if /local/pcb/edif/enable then
	if /local/pcb/edif/buildin then
		include {Makefile.in.mod/Buildin}
	else
		include {Makefile.in.mod/Plugin}
	end
end
