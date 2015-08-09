include Makefile.gpmi
include ../Makefile.config

CFLAGS += -g -I$(PCB_SRC) $(PCB_CFLAGS)

all: gpmi_plugin.so gpmi_buildin.o
	cd gpmi_pkg; make

gpmi_plugin.so: gpmi_plugin.o scripts.o manage_scripts.o
	$(CC) $(LDFLAGS) -shared -rdynamic  -o $@ $^ -lgpmi

gpmi_plugin.o: gpmi_plugin.c scripts.h

gpmi_buildin.o: gpmi_plugin.c scripts.h
	$(CC) -DPLUGIN_INIT_NAME=hid_gpmi_init $(CFLAGS) -c gpmi_plugin.c -o gpmi_buildin.o

scripts.o: scripts.c

manage_scripts.o: manage_scripts.c

clean:
	rm -f gpmi_plugin.o
	cd gpmi_pkg; make clean
