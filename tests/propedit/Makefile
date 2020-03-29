all: tester

ROOT=../..
include $(ROOT)/Makefile.conf

PED=$(ROOT)/src_plugins/propedit
LHT=$(ROOT)/src_3rd
SRC=$(ROOT)/src
LIBRND=$(SRC)/librnd

CFLAGS = $(PCB_RND_C89FLAGS) -I$(PED) -I$(LHT) -I$(SRC) -I$(ROOT) -I$(ROOT)/src_3rd
LDLIBS = -lm
LIB_OBJS = \
 $(LHT)/genht/htsp.o \
 $(LHT)/genht/hash.o \
 $(LIBRND)/core/compat_misc.o \
 $(LIBRND)/core/misc_util.o \
 $(LIBRND)/core/pcb-printf.o \
 $(LIBRND)/core/unit.o \
 $(ROOT)/src_3rd/genvector/gds_char.o

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
