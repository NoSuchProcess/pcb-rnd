ROOT=../..
ORD=$(ROOT)/src_plugins/order
include $(ROOT)/Makefile.conf

CFLAGS = $(PCB_RND_C89FLAGS) $(CFLAGS_LIBRND) -I$(ROOT) -I$(ROOT)/src_3rd -I$(LIBRND_INCDIR)/librnd/src_3rd
LDFLAGS = $(LDFLAGS_LIBRND)
ORDLIB = $(ORD)/constraint.o $(ORD)/const_gram.o $(ORD)/const_lex.o

LDLIBS = -lrnd-hid -lrnd-3rd $(LDFLAGS_LIBRND) -lm


all: tester

tester: tester.o $(ORDLIB)


clean:
	-rm tester.o tester
