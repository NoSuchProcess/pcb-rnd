MENU_RES=../src/pcb-menu-default.lht
KEYLIST=../util/keylist.sh
DEBLIST=../util/devhelpers/deblist.sh
ROOT=..

all: keys.html user/05_ui/04_common/keytree.svg user/05_ui/04_common/keytree.txt
	ROOT="" ./Autostyle.sh *.html

include ../Makefile.conf

user/05_ui/04_common/keytree.svg: $(MENU_RES) $(KEYLIST)
	$(KEYLIST) --dot user/05_ui/04_common/src/node_names.txt $(MENU_RES) > user/05_ui/04_common/keytree.dot
	dot -Tsvg < user/05_ui/04_common/keytree.dot >user/05_ui/04_common/keytree.svg

user/05_ui/04_common/keytree.txt: $(MENU_RES) $(KEYLIST)
	$(KEYLIST) --lst $(MENU_RES) > user/05_ui/04_common/keytree.txt


keys.html: $(MENU_RES) $(KEYLIST)
	$(KEYLIST) $(MENU_RES) > keys.html

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
