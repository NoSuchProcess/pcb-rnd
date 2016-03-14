MENU_RES=../src/gpcb-menu.res ../src/pcb-menu.res
KEYLIST=../util/keylist.sh

all: keys.html

keys.html: $(MENU_RES) $(KEYLIST)
	$(KEYLIST) $(MENU_RES) > keys.html
