TRUNK = ../..
SRC=$(TRUNK)/src
LIBRND=$(SRC)/librnd
CFLAGS_OP = -O3
CFLAGS = -I$(TRUNK) -I$(SRC) -I$(TRUNK)/src_3rd -I$(TRUNK)/src_3rd/liblihata
LDLIBS = -lm

GDS= $(TRUNK)/src_3rd/genvector/gds_char.o
LIBPCB_BASE= $(LIBRND)/core/unit.o $(LIBRND)/core/compat_misc.o

all: tester tester_spd prcli prclimq

ROOT=../..
include $(ROOT)/Makefile.conf

test: tester.diff
	@echo "pcb-printf:          *** QC PASS ***"
	@rm tester.stdout ; true

prcli: prcli.o $(LIBRND)/core/pcb-printf.o $(LIBRND)/core/misc_util.o $(LIBPCB_BASE) $(GDS)
	$(CC) $(LDFLAGS) -o prcli prcli.o $(LIBRND)/core/pcb-printf.o $(LIBRND)/core/misc_util.o $(LIBPCB_BASE) $(GDS) $(LDLIBS)

prclimq: prclimq.o $(LIBRND)/core/pcb-printf.o $(LIBRND)/core/misc_util.o $(LIBPCB_BASE) $(GDS)
	$(CC) $(LDFLAGS) -o prclimq prclimq.o $(LIBRND)/core/pcb-printf.o $(LIBRND)/core/misc_util.o $(LIBPCB_BASE) $(GDS) $(LDLIBS)

prcli.o: prcli.c
	$(CC) -c $(CFLAGS) prcli.c -o prcli.o

prclimq.o: prclimq.c
	$(CC) -c $(CFLAGS) prclimq.c -o prclimq.o

tester: tester.o $(LIBRND)/core/pcb-printf.o $(LIBPCB_BASE)  $(GDS)
	$(CC) $(LDFLAGS) -o tester tester.o $(LIBRND)/core/pcb-printf.o $(LIBPCB_BASE)  $(GDS) $(LDLIBS)

tester_spd: tester_spd.o $(LIBRND)/core/pcb-printf_spd.o $(LIBPCB_BASE) $(GDS)
	$(CC) $(LDFLAGS) -o tester_spd tester_spd.o $(LIBRND)/core/pcb-printf_spd.o $(LIBPCB_BASE) $(GDS) $(LDLIBS)

tester.o: tester.c
	$(CC) -c $(CFLAGS) -o $@ tester.c

tester_spd.o: tester.c
	$(CC) -c $(CFLAGS_OP) -DSPEED $(CFLAGS) -o $@ tester.c

tester.stdout: tester
	./tester > tester.stdout

tester.diff: tester.stdout
	diff -u tester.ref tester.stdout

$(LIBRND)/core/pcb-printf.o: $(LIBRND)/core/pcb-printf.c $(LIBRND)/core/pcb-printf.h

$(LIBRND)/core/unit.o: $(LIBRND)/core/unit.c $(LIBRND)/core/unit.h

$(LIBRND)/core/pcb-printf_spd.o: $(LIBRND)/core/pcb-printf.c $(LIBRND)/core/pcb-printf.h
	$(CC) -c $(CFLAGS_OP) -DSPEED $(CFLAGS) -o $@ $(LIBRND)/core/pcb-printf.c

clean:
	-$(SCCBOX) rm -f tester tester.o prcli prcli.o tester_spd tester_spd.o prclimq prclimq.o $(LIBRND)/core/pcb-printf_spd.o

