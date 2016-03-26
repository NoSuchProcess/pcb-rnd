CFLAGS_OP = -O3
CFLAGS = -g \
  -I../.. -I../../.. -I../../../src_3rd

GDS= ../../../src_3rd/genvector/gds_char.o

all: test

test: tester.diff
	@echo "*** all ok, QC PASS ***"

tester: tester.o $(GDS)

tester.o: tester.c
	$(CC) -c $(CFLAGS) -o $@ tester.c

tester.stdout: tester
	./tester > tester.stdout

tester.diff: tester.stdout
	diff -u tester.ref tester.stdout

../../strflags.o: ../../strflags.c ../../strflags.h

clean:
	rm tester tester.o 2>/dev/null ; true

