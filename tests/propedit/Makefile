all: tester

ROOT=../..
include $(ROOT)/Makefile.conf
include $(LIBRND_MAK)

PED=$(ROOT)/src_plugins/propedit
LHT=$(ROOT)/src_3rd
SRC=$(ROOT)/src
SRC_3RD=$(ROOT)/src_3rd

CFLAGS = -g $(PCB_RND_C89FLAGS) $(CFLAGS_LIBRND) -I$(PED) -I$(LHT) -I$(SRC) -I$(ROOT) -I$(ROOT)/src_3rd -I$(LIBRND_INCDIR)/librnd/src_3rd
LDFLAGS = $(LDFLAGS_LIBRND)
LDLIBS = -lrnd-core -lrnd-3rd  $(LDFLAGS_LIBRND) -lm

OBJS = tester.o $(PED)/props.o

test: tester.diff
	@echo "propedit:            *** QC PASS ***"
	@rm tester.out

tester: $(OBJS) $(LIB_OBJS)
	$(CC) $(LDFLAGS) -o tester $(OBJS) $(LIB_OBJS)  $(LDLIBS)

tester.o: tester.c $(PED)/props.h

$(PED)/props.o: $(PED)/props.c $(PED)/props.h

tester.diff: tester.ref tester.out
	@diff -u tester.ref tester.out

tester.out: tester
	./tester > tester.out

clean:
	-$(SCCBOX) rm -f tester.out tester $(OBJS)
