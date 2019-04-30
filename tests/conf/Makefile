CFLAGS = -Wall -g -I../.. -I../../src_3rd -I../../src
LDLIBS = -lm

LIB_PCB_OBJS= \
	../../src/hid_cfg.o \
	../../src/misc_util.o \
	../../src/color.o \
	../../src/unit.o \
	../../src/list_conf.o \
	../../src/paths.o \
	../../src/conf.o \
	../../src/conf_core.o \
	../../src/hidlib_conf.o \
	../../src/conf_hid.o \
	../../src/conf_internal.o \
	../../src/pcb-printf.o \
	../../src/compat_misc.o \
	../../src/compat_fs.o \
	../../src/safe_fs.o

LIB_PLUGIN_OBJS=\
	../../src_plugins/diag/diag_conf.o

LIB_3RD_OBJS= \
	../../src_3rd/liblihata/parser.o \
	../../src_3rd/liblihata/dom.o \
	../../src_3rd/liblihata/dom_list.o \
	../../src_3rd/liblihata/dom_hash.o \
	../../src_3rd/liblihata/dom_table.o \
	../../src_3rd/liblihata/lihata.o \
	../../src_3rd/liblihata/hash_str.o \
	../../src_3rd/liblihata/tree.o \
	../../src_3rd/liblihata/tree_list.o \
	../../src_3rd/liblihata/tree_hash.o \
	../../src_3rd/liblihata/tree_table.o \
	../../src_3rd/liblihata/tree_symlink.o \
	../../src_3rd/liblihata/tree_path.o \
	../../src_3rd/genht/hash.o \
	../../src_3rd/genht/htsp.o \
	../../src_3rd/genht/htpi.o \
	../../src_3rd/genht/htsi.o \
	../../src_3rd/genht/htip.o \
	../../src_3rd/genht/htpp.o \
	../../src_3rd/genvector/gds_char.o \
	../../src_3rd/genvector/vtp0.o

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

conftest: $(OBJS) $(LIB_3RD_OBJS) $(LIB_PCB_OBJS) $(LIB_PLUGIN_OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIB_3RD_OBJS) $(LIB_PCB_OBJS) $(LIB_PLUGIN_OBJS) $(LDLIBS) -o conftest

conftest.o: conftest.c $(DEPH)
	$(CC) $(CFLAGS) -c conftest.c -o conftest.o

help.o: help.c

$(LIB_3RD_OBJS) $(LIB_PCB_OBJS):
	cd ../../src && make

test:
	cd tests && $(MAKE) all

clean:
	-$(SCCBOX) rm -f conftest tests/*.out $(OBJS)
