all:

test:
	@./test.sh && echo "pcbflags:            *** QC PASS ***"

clean:
	mv vanilla.pcb vanilla.save
	-rm *.pcb *.log
	mv vanilla.save vanilla.pcb
