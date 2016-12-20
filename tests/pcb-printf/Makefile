TRUNK = ../..
SRC=$(TRUNK)/src
CFLAGS_OP = -O3
CFLAGS = -I$(TRUNK) -I$(SRC) -I$(TRUNK)/src_3rd -I$(TRUNK)/src_3rd/liblihata
LDFLAGS = -lm

GDS= $(TRUNK)/src_3rd/genvector/gds_char.o
LIBPCB_BASE= $(SRC)/unit.o $(SRC)/compat_misc.o

all: tester_spd prcli

test: tester.diff
	@echo "pcb-printf:          *** QC PASS ***"
	@rm tester.stdout ; true

prcli: prcli.o $(SRC)/pcb-printf.o $(SRC)/misc_util.o $(LIBPCB_BASE) $(GDS)

prcli.o: prcli.c

tester: tester.o $(SRC)/pcb-printf.o $(LIBPCB_BASE)  $(GDS)

tester_spd: tester_spd.o $(SRC)/pcb-printf_spd.o $(LIBPCB_BASE) $(GDS)

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
	-rm tester tester.o prcli prcli.o tester_spd tester_spd.o $(SRC)/pcb-printf_spd.o  2>/dev/null

