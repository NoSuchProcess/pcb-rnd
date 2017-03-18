all:

ROOT=../..
include $(ROOT)/Makefile.conf

test:
	@./test.sh && echo "pcbflags:            *** QC PASS ***"

clean:
	mv vanilla.pcb vanilla.save
	-$(SCCBOX) rm -f *.pcb *.log
	mv vanilla.save vanilla.pcb
