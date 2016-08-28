ROOT=../..
PED=$(ROOT)/src_plugins/propedit
LHT=$(ROOT)/src_3rd/liblihata
SRC=$(ROOT)/src

CFLAGS = -g -Wall -I$(PED) -I$(LHT) -I$(SRC) -I$(ROOT) -I$(ROOT)/src_3rd
LIB_OBJS = \
 $(LHT)/genht/htsp.o \
 $(LHT)/genht/hash.o \
 $(SRC)/compat_misc.o \
 $(SRC)/pcb-printf.o \
 $(SRC)/unit.o \
 $(ROOT)/src_3rd/genvector/gds_char.o

all: tester

test: tester.diff
	@echo "*** All ok, QC passed ***"
	@rm tester.out

tester: tester.o $(PED)/props.o $(LIB_OBJS)

tester.o: tester.c $(PED)/props.h

$(PED)/props.o: $(PED)/props.c $(PED)/props.h

tester.diff: tester.ref tester.out
	@diff -u tester.ref tester.out

tester.out: tester
	./tester > tester.out

