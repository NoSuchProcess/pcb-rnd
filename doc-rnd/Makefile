MENU_RES=../src/pcb-menu-gtk.lht ../src/pcb-menu-lesstif.lht
MENU_RES_MKEY=../src/pcb-menu-mkey.lht
KEYLIST=../util/keylist.sh

all: keys.html keys_mkey.html
	./Autostyle.sh *.html

keys.html: $(MENU_RES) $(KEYLIST)
	$(KEYLIST) $(MENU_RES) > keys.html

keys_mkey.html: $(MENU_RES_MKEY) $(KEYLIST)
	$(KEYLIST) $(MENU_RES_MKEY) > keys_mkey.html

