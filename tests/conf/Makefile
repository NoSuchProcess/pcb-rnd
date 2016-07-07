CFLAGS = -Wall -g -I../.. -I../../src_3rd -I../../src
LDFLAGS = -lm

LIB_PCB_OBJS= \
	../../src/hid_cfg.o \
	../../src/misc_util.o \
	../../src/unit.o \
	../../src/list_conf.o \
	../../src/paths.o \
	../../src/conf.o \
	../../src/conf_core.o \
	../../src/conf_hid.o \
	../../src/conf_internal.o \
	../../src/pcb-printf.o

LIB_PLUGIN_OBJS=\
	../../src_plugins/debug/debug_conf.o

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
	../../src_3rd/liblihata/genht/hash.o \
	../../src_3rd/liblihata/genht/htsp.o \
	../../src_3rd/liblihata/genht/htpi.o \
	../../src_3rd/liblihata/genht/htip.o \
	../../src_3rd/liblihata/genht/htpp.o \
	../../src_3rd/genvector/gds_char.o \
	../../src_3rd/genvector/vtp0.o

conftest: conftest.o $(LIB_3RD_OBJS) $(LIB_PCB_OBJS) $(LIB_PLUGIN_OBJS)

conftest.o: conftest.c

$(LIB_3RD_OBJS) $(LIB_PCB_OBJS):
	cd ../../src && make
