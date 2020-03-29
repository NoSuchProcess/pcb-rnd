include ../../Makefile.conf

TRUNK=../..
IO=$(TRUNK)/src_plugins/io_pcb
CFLAGS = $(PCB_RND_C89FLAGS) -O3 -I$(TRUNK) -I$(TRUNK)/src -I$(IO) -I$(TRUNK)/src_3rd
LDLIBS = -lm
GDS= $(TRUNK)/src_3rd/genvector/gds_char.o

all: tester

ROOT=../..
include $(ROOT)/Makefile.conf

test: tester.diff
	@echo "strflags:            *** QC PASS ***"
	@rm tester.stdout

tester: tester.o $(GDS)
	$(CC) $(LDFLAGS) -o tester tester.o $(GDS) $(LDLIBS)

tester.o: tester.c
	$(CC) -c $(CFLAGS) -o $@ tester.c

tester.stdout: tester
	./tester > tester.stdout

tester.diff: tester.stdout
	diff -u tester.ref tester.stdout

clean:
	-$(SCCBOX) rm -f tester tester.o

