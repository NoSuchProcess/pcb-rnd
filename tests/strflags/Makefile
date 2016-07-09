TRUNK=../..
IO=$(TRUNK)/src_plugins/io_pcb
CFLAGS = -O3 -I$(TRUNK) -I$(TRUNK)/src -I$(IO) -I$(TRUNK)/src_3rd
GDS= $(TRUNK)/src_3rd/genvector/gds_char.o

all: tester

test: tester.diff
	@echo "*** all ok, QC PASS ***"
	@rm tester.stdout

tester: tester.o $(GDS)

tester.o: tester.c
	$(CC) -c $(CFLAGS) -o $@ tester.c

tester.stdout: tester
	./tester > tester.stdout

tester.diff: tester.stdout
	diff -u tester.ref tester.stdout

clean:
	rm tester tester.o 2>/dev/null ; true

