append /local/pcb/autoplace/enable {}
append /local/pcb/autoplace/buildin {}

put /local/pcb/mod {autoplace}
put /local/pcb/mod/OBJS [@ $(PLUGDIR)/autoplace/autoplace.o $(PLUGDIR)/autoplace/action.o @]

if /local/pcb/autoplace/enable then
	if /local/pcb/autoplace/buildin then
		include {Makefile.in.mod/Buildin}
	else
		include {Makefile.in.mod/Plugin}
	end
end



