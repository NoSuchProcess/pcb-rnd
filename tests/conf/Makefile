include ../../src/librnd.mak

CFLAGS = -Wall -g -I../.. -I../../src_3rd -I../../src $(CFLAGS_LIBRND_FUNGW)
LDLIBS = -lm $(LDFLAGS_LIBRND_FUNGW)

LIB_HIDLIB=\
	../../src/librnd-hid.a \
	../../src/librnd-3rd.a

#LIBGENHT=\
#	../../src_3rd/genht/hash.o \
#	../../src_3rd/genht/htsp.o \
#	../../src_3rd/genht/htsi.o \
#	../../src_3rd/genht/htpp.o \
#	../../src_3rd/genht/htpi.o \

LIB_PCBRND_OBJS=\
	../../src_plugins/diag/diag_conf.o \
	../../src/conf_core.o \
	../../src/conf_internal.o

DEPH = \
	../../src/conf.h \
	../../src/conf_hid.h \
	../../src/conf_core.h \
	../../src/compat_misc.h \
	../../src_plugins/diag/diag_conf.h \

OBJS = conftest.o help.o

all: conftest

ROOT=../..
include $(ROOT)/Makefile.conf

conftest: $(OBJS) $(LIB_HIDLIB) $(LIB_PCBRND_OBJS) $(LIBGENHT)
	$(CC) $(LDFLAGS) $(OBJS) $(LIB_PCBRND_OBJS) $(LIB_HIDLIB)  $(LIBGENHT) $(LDLIBS) -o conftest

conftest.o: conftest.c $(DEPH)
	$(CC) $(CFLAGS) -c conftest.c -o conftest.o

help.o: help.c

$(LIB_HIDLIB):
	cd ../../src && make

test:
	cd tests && $(MAKE) all

clean:
	-$(SCCBOX) rm -f conftest tests/*.out $(OBJS)
