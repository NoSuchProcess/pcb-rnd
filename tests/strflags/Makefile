ROOT=../..
include $(ROOT)/Makefile.conf
include $(LIBRND_MAK)


IO=$(ROOT)/src_plugins/io_pcb
CFLAGS = $(PCB_RND_C89FLAGS) -O3 -I$(ROOT) -I$(ROOT)/src -I$(IO)  -I$(ROOT)/src_3rd -I$(LIBRND_INCDIR)
LDFLAGS = $(LDFLAGS_LIBRND)
LDLIBS = -lm
LIBRND_LIBS = -lrnd-3rd

all: tester

test: tester.diff
	@echo "strflags:            *** QC PASS ***"
	@rm tester.stdout

tester: tester.o
	$(CC) $(LDFLAGS) -o tester tester.o $(LIBRND_LIBS) $(LDLIBS)

tester.o: tester.c
	$(CC) -c $(CFLAGS) -o $@ tester.c

tester.stdout: tester
	./tester > tester.stdout

tester.diff: tester.stdout
	diff -u tester.ref tester.stdout

clean:
	-$(SCCBOX) rm -f tester tester.o

