# glib: temporary
CFLAGS_OP = -O3
CFLAGS = -g \
  -I../.. -I../../.. -I../../../src_3rd \
  -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include
LDFLAGS = -lglib-2.0
GDS= ../../../src_3rd/genvector/gds_char.o

all: test

test: tester_spd tester.diff
	@echo "*** all ok, QC PASS ***"

tester: tester.o ../../pcb-printf.o $(GDS)

tester_spd: tester_spd.o ../../pcb-printf_spd.o $(GDS)

tester.o: tester.c
	$(CC) -c $(CFLAGS) -o $@ tester.c

tester_spd.o: tester.c
	$(CC) -c $(CFLAGS_OP) -DSPEED $(CFLAGS) -o $@ tester.c

tester.stdout: tester
	./tester > tester.stdout

tester.diff: tester.stdout
	diff -u tester.ref tester.stdout

../../pcb-printf.o: ../../pcb-printf.c ../../pcb-printf.h

../../pcb-printf_spd.o: ../../pcb-printf.c ../../pcb-printf.h
	$(CC) -c $(CFLAGS_OP) -DSPEED $(CFLAGS) -o $@ ../../pcb-printf.c

clean:
	rm tester tester.o 2>/dev/null ; true

