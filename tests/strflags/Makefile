include ../../Makefile.conf

TRUNK=../..
IO=$(TRUNK)/src_plugins/io_pcb
CFLAGS = $(PCB_RND_C89FLAGS) -O3 -I$(TRUNK) -I$(TRUNK)/src -I$(IO)  -I$(TRUNK)/src_3rd
LDLIBS = -lm
LIBRND_LIBS = $(TRUNK)/src_3rd/librnd-local/src/librnd-3rd.a

all: tester

ROOT=../..
include $(ROOT)/Makefile.conf

test: tester.diff
	@echo "strflags:            *** QC PASS ***"
	@rm tester.stdout

tester: tester.o $(LIBRND_LIBS)
	$(CC) $(LDFLAGS) -o tester tester.o $(LIBRND_LIBS) $(LDLIBS)

tester.o: tester.c
	$(CC) -c $(CFLAGS) -o $@ tester.c

tester.stdout: tester
	./tester > tester.stdout

tester.diff: tester.stdout
	diff -u tester.ref tester.stdout

clean:
	-$(SCCBOX) rm -f tester tester.o

