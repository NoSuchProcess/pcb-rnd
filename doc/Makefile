MENU_RES=../src/pcb-menu-gtk.lht ../src/pcb-menu-lesstif.lht
MENU_RES_MKEY=../src/pcb-menu-mkey.lht
KEYLIST=../util/keylist.sh
DEBLIST=../util/devhelpers/deblist.sh

all: keys.html keys_mkey.html features/debian_list.html
	./Autostyle.sh *.html features/*.html

keys.html: $(MENU_RES) $(KEYLIST)
	$(KEYLIST) $(MENU_RES) > keys.html

keys_mkey.html: $(MENU_RES_MKEY) $(KEYLIST)
	$(KEYLIST) $(MENU_RES_MKEY) > keys_mkey.html

install:
	cd man && $(MAKE) install

linstall:
	cd man && $(MAKE) linstall

uninstall:
	cd man && $(MAKE) uninstall

clean:
	cd man && $(MAKE) clean

distclean:
	cd man && $(MAKE) distclean
