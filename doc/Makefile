MENU_RES=../src/pcb-menu-gtk.lht ../src/pcb-menu-lesstif.lht
MENU_RES_MKEY=../src/pcb-menu-mkey.lht
KEYLIST=../util/keylist.sh
DEBLIST=../util/devhelpers/deblist.sh
ROOT=..

all: keys.html keys_mkey.html features/debian_list.html
	ROOT="" ./Autostyle.sh *.html
	ROOT="../" ./Autostyle.sh features/*.html

include ../Makefile.conf

keys.html: $(MENU_RES) $(KEYLIST)
	$(KEYLIST) $(MENU_RES) > keys.html

keys_mkey.html: $(MENU_RES_MKEY) $(KEYLIST)
	$(KEYLIST) $(MENU_RES_MKEY) > keys_mkey.html

install_main:
	$(SCCBOX) $(HOW) *.html *.txt TODO $(DOCDIR)/

install:
	$(SCCBOX) mkdir -p "$(DOCDIR)"
	cd man && $(MAKE) install
	cd user && $(MAKE) install
	cd tutorials && $(MAKE) install
	cd security && $(MAKE) install
	cd conf && $(MAKE) install
	cd developer && $(MAKE) install
	$(MAKE) install_main HOW="install -f -d"

linstall:
	cd man && $(MAKE) linstall
	cd user && $(MAKE) linstall
	cd tutorials && $(MAKE) linstall
	cd security && $(MAKE) linstall
	cd conf && $(MAKE) linstall
	cd developer && $(MAKE) linstall
	$(MAKE) install_main HOW="install -f -l -d"

uninstall:
	cd man && $(MAKE) uninstall
	cd user && $(MAKE) uninstall
	cd tutorials && $(MAKE) uninstall
	cd security && $(MAKE) uninstall
	cd conf && $(MAKE) uninstall
	cd developer && $(MAKE) uninstall
	$(MAKE) install_main HOW="install -f -u -d"

clean:
	cd man && $(MAKE) clean
	cd user && $(MAKE) clean
	cd tutorials && $(MAKE) clean
	cd security && $(MAKE) clean
	cd conf && $(MAKE) clean
	cd developer && $(MAKE) clean

distclean:
	cd man && $(MAKE) distclean
	cd user && $(MAKE) distclean
	cd tutorials && $(MAKE) distclean
	cd security && $(MAKE) distclean
	cd conf && $(MAKE) distclean
	cd developer && $(MAKE) distclean
