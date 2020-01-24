ROOT=../..
UNM=$(ROOT)/src_plugins/io_kicad
LHT=$(ROOT)/src_3rd
SRC=$(ROOT)/src
LIBRND=$(SRC)/librnd

CFLAGS = -Wall -I$(UNM) -I$(LHT) -I$(SRC) -I$(ROOT)
LDLIBS = -lm
LIB_OBJS=$(LHT)/genht/htsp.o $(LHT)/genht/hash.o $(LIBRND)/core/compat_misc.o $(LHT)/genvector/gds_char.o

all: tester

ROOT=../..
include $(ROOT)/Makefile.conf

test: tester.diff
	@echo "uniq_name:           *** QC PASS ***"
	@rm tester.out

tester: tester.o $(UNM)/uniq_name.o $(LIB_OBJS)
	$(CC) $(LDFLAGS) -o tester tester.o $(UNM)/uniq_name.o $(LIB_OBJS)  $(LDLIBS)

tester.o: tester.c $(UNM)/uniq_name.h

$(UNM)/uniq_name.o: $(UNM)/uniq_name.c $(UNM)/uniq_name.h

tester.diff: tester.ref tester.out
	@diff -u tester.ref tester.out

tester.out: tester
	./tester > tester.out

clean:
	-$(SCCBOX) rm -f tester.out tester tester.o
