MENU_RES=../src/pcb-menu-gtk.lht ../src/pcb-menu-lesstif.lht
KEYLIST=../util/keylist.sh

all: keys.html

keys.html: $(MENU_RES) $(KEYLIST)
	$(KEYLIST) $(MENU_RES) > keys.html

