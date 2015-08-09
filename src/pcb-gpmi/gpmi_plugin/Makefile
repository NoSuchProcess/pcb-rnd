include Makefile.gpmi
include ../Makefile.config

CFLAGS += -g -I$(PCB_SRC) $(PCB_CFLAGS)

COMMON_OBJS = scripts.o manage_scripts.o
BUILDIN_OBJS = gpmi_buildin.o $(COMMON_OBJS)
PLUGIN_OBJS = gpmi_plugin.o $(COMMON_OBJS)

all: gpmi_plugin.so gpmi_buildin.a
	cd gpmi_pkg; make

all_plugin: gpmi_plugin.so
	cd gpmi_pkg; make

all_buildin: gpmi_buildin.a
	cd gpmi_pkg; make

gpmi_buildin.a: $(BUILDIN_OBJS)
	ar rvu gpmi_buildin.a $(BUILDIN_OBJS)

gpmi_plugin.so: $(PLUGIN_OBJS)
	$(CC) $(LDFLAGS) -shared -rdynamic  -o gpmi_plugin.so $(PLUGIN_OBJS) -lgpmi

gpmi_plugin.o: gpmi_plugin.c scripts.h

gpmi_buildin.o: gpmi_plugin.c scripts.h
	$(CC) -DPLUGIN_INIT_NAME=hid_gpmi_init $(CFLAGS) -c gpmi_plugin.c -o gpmi_buildin.o

scripts.o: scripts.c

manage_scripts.o: manage_scripts.c

clean:
	rm gpmi_plugin.o gpmi_plugin.so gpmi_buildin.a gpmi_buildin.o $(COMMON_OBJS) 2>/dev/null ; true
	cd gpmi_pkg; make clean
