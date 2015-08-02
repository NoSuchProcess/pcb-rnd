include Makefile.gpmi
include ../Makefile.config

CFLAGS += -g -I$(PCB_SRC) $(PCB_CFLAGS)

all: gpmi_plugin.so
	cd gpmi_pkg; make

gpmi_plugin.so: gpmi_plugin.o scripts.o manage_scripts.o
	$(CC) $(LDFLAGS) -shared -rdynamic  -o $@ $^ -lgpmi

gpmi_plugin.o: gpmi_plugin.c scripts.h

scripts.o: scripts.c

manage_scripts.o: manage_scripts.c

clean:
	rm -f gpmi_plugin.o
	cd gpmi_pkg; make clean
