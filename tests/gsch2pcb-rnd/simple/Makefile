TRUNK=../../..
all:

include ../Makefile.common

test: FORCE
	@rm main.pcb main.pcb.new main.cmd main.net main.out 2>/dev/null; true
	@$(GSCH2PCB) $(CONF) main.sch > main.out
	@$(DIFF) main.pcb.ref main.pcb
