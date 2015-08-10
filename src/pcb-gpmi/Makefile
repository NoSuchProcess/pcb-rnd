PCB_GPMI_ROOT=.

all:
	cd gpmi_plugin; make all


clean:
	cd gpmi_plugin; make clean

test: all
	cd host_lib; ./test.sh

install:
	cp gpmi_plugin/gpmi_plugin.so .pcb/plugins
	cd gpmi_plugin/gpmi_pkg; ./Install.sh

linstall:
	ln -s `pwd`/gpmi_plugin/gpmi_plugin.so .pcb/plugins
	cd gpmi_plugin/gpmi_pkg; ./Install.sh -l
