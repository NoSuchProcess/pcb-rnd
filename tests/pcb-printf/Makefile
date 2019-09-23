TRUNK = ../..
SRC=$(TRUNK)/src
CFLAGS_OP = -O3
CFLAGS = -I$(TRUNK) -I$(SRC) -I$(TRUNK)/src_3rd -I$(TRUNK)/src_3rd/liblihata
LDLIBS = -lm

GDS= $(TRUNK)/src_3rd/genvector/gds_char.o
LIBPCB_BASE= $(SRC)/unit.o $(SRC)/compat_misc.o

all: tester tester_spd prcli prclimq

ROOT=../..
include $(ROOT)/Makefile.conf

test: tester.diff
	@echo "pcb-printf:          *** QC PASS ***"
	@rm tester.stdout ; true

prcli: prcli.o $(SRC)/pcb-printf.o $(SRC)/misc_util.o $(LIBPCB_BASE) $(GDS)
	$(CC) $(LDFLAGS) -o prcli prcli.o $(SRC)/pcb-printf.o $(SRC)/misc_util.o $(LIBPCB_BASE) $(GDS) $(LDLIBS)

prclimq: prclimq.o $(SRC)/pcb-printf.o $(SRC)/misc_util.o $(LIBPCB_BASE) $(GDS)
	$(CC) $(LDFLAGS) -o prclimq prclimq.o $(SRC)/pcb-printf.o $(SRC)/misc_util.o $(LIBPCB_BASE) $(GDS) $(LDLIBS)

prcli.o: prcli.c
	$(CC) -c $(CFLAGS) prcli.c -o prcli.o

prclimq.o: prclimq.c
	$(CC) -c $(CFLAGS) prclimq.c -o prclimq.o

tester: tester.o $(SRC)/pcb-printf.o $(LIBPCB_BASE)  $(GDS)
	$(CC) $(LDFLAGS) -o tester tester.o $(SRC)/pcb-printf.o $(LIBPCB_BASE)  $(GDS) $(LDLIBS)

tester_spd: tester_spd.o $(SRC)/pcb-printf_spd.o $(LIBPCB_BASE) $(GDS)
	$(CC) $(LDFLAGS) -o tester_spd tester_spd.o $(SRC)/pcb-printf_spd.o $(LIBPCB_BASE) $(GDS) $(LDLIBS)

tester.o: tester.c
	$(CC) -c $(CFLAGS) -o $@ tester.c

tester_spd.o: tester.c
	$(CC) -c $(CFLAGS_OP) -DSPEED $(CFLAGS) -o $@ tester.c

tester.stdout: tester
	./tester > tester.stdout

tester.diff: tester.stdout
	diff -u tester.ref tester.stdout

$(SRC)/pcb-printf.o: $(SRC)/pcb-printf.c $(SRC)/pcb-printf.h

$(SRC)/unit.o: $(SRC)/unit.c $(SRC)/unit.h

$(SRC)/pcb-printf_spd.o: $(SRC)/pcb-printf.c $(SRC)/pcb-printf.h
	$(CC) -c $(CFLAGS_OP) -DSPEED $(CFLAGS) -o $@ $(SRC)/pcb-printf.c

clean:
	-$(SCCBOX) rm -f tester tester.o prcli prcli.o tester_spd tester_spd.o prclimq prclimq.o $(SRC)/pcb-printf_spd.o

