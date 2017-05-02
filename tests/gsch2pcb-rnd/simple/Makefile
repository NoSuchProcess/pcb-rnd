TRUNK=../../..
all:

include ../Makefile.common

test: FORCE
	@make clean
	@$(GSCH2PCB) -m import $(CONF) main.sch > main.log
	@$(DIFF) main.cmd.ref main.cmd

clean:
	@rm main.pcb main.cmd main.log 2>/dev/null; true