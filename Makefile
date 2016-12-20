all: FORCE
	cd src && $(MAKE)
	cd util && $(MAKE)
	cd pcblib && $(MAKE)
#	cd doc && $(MAKE)

test: FORCE
	cd tests && $(MAKE) test

clean: FORCE
	cd src && $(MAKE) clean
	cd util && $(MAKE) clean
	cd pcblib && $(MAKE) clean
#	cd doc && $(MAKE) clean
	cd tests && $(MAKE) clean
	cd src_3rd/sphash && $(MAKE) clean

distclean: FORCE
	$(MAKE) clean ; true
#	cd doc && $(MAKE) distclean
	cd src && $(MAKE) distclean
	cd util/gsch2pcb-rnd && $(MAKE) distclean
	cd src_3rd/genlist && $(MAKE) clean ; true
	cd src_3rd/genregex && $(MAKE) clean ; true
	cd src_3rd/genvector && $(MAKE) clean ; true
	cd src_3rd/gts && $(MAKE) clean ; true
	cd src_3rd/liblihata && $(MAKE) clean ; true
	cd src_3rd/liblihata/genht && $(MAKE) clean ; true
	cd src_3rd/qparse && $(MAKE) clean ; true
	cd scconfig && $(MAKE) distclean ; true


install: FORCE
	cd src && $(MAKE) install
	cd util && $(MAKE) install
	cd pcblib && $(MAKE) install
	cd doc && $(MAKE) install

linstall: FORCE
	cd src && $(MAKE) linstall
	cd util && $(MAKE) linstall
	cd pcblib && $(MAKE) linstall
	cd doc && $(MAKE) linstall

uninstall: FORCE
	cd src && $(MAKE) uninstall
	cd util && $(MAKE) uninstall
	cd pcblib && $(MAKE) uninstall
	cd doc && $(MAKE) uninstall

FORCE:
