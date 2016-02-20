append /local/pcb/vendordrill/enable {}
append /local/pcb/vendordrill/buildin {}

put /local/pcb/mod {vendordrill}
append /local/pcb/mod/OBJS [@ $(PLUGDIR)/vendordrill/vendor.o @]

if /local/pcb/vendordrill/enable then
	if /local/pcb/vendordrill/buildin then
		include {Makefile.in.mod/Buildin}
	else
		include {Makefile.in.mod/Plugin}
	end
end
