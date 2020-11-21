all: tester

ROOT=../..
include $(ROOT)/Makefile.conf

PED=$(ROOT)/src_plugins/propedit
LHT=$(ROOT)/src_3rd
SRC=$(ROOT)/src
SRC_3RD=$(ROOT)/src_3rd

CFLAGS = $(PCB_RND_C89FLAGS) -I$(PED) -I$(LHT) -I$(SRC) -I$(ROOT) -I$(ROOT)/src_3rd
LDLIBS = -lm
LIB_OBJS = \
	$(SRC_3RD)/librnd-local/src/librnd-hid.a \
	$(SRC_3RD)/librnd-local/src/librnd-3rd.a \
	glue.o \

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

glue.o: glue.c
	$(CC) -c $(CFLAGS) glue.c -o glue.o

clean:
	-$(SCCBOX) rm -f tester.out tester $(OBJS)
