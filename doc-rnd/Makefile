MENU_RES=../src/gpcb-menu.res ../src/pcb-menu.res

all: keys.html

keys.html: $(MENU_RES)
	../util/keylist.sh $(MENU_RES) > keys.html