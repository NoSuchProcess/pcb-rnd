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

features/debian_list.html: ../debian/control
	$(DEBLIST) < ../debian/control > features/debian_list.html
	./Autostyle.sh features/debian_list.html

install:
	cd man && make install

linstall:
	cd man && make linstall

uninstall:
	cd man && make uninstall

clean:
	cd man && make clean

distclean:
	cd man && make distclean
