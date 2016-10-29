all: FORCE
	cd src && make
	cd util && make
	cd pcblib && make
	cd doc-rnd && make

clean: FORCE
	cd src && make clean
	cd util && make clean
	cd pcblib && make clean
	cd doc-rnd && make clean

distclean: FORCE
	make clean ; true
	cd doc-rnd && make distclean
	cd scconfig && make clean ; true
	cd src_3rd/genlist && make clean ; true
	cd src_3rd/genregex && make clean ; true
	cd src_3rd/genvector && make clean ; true
	cd src_3rd/gts && make clean ; true
	cd src_3rd/liblihata && make clean ; true
	cd src_3rd/liblihata/genht && make clean ; true
	cd src_3rd/qparse && make clean ; true

install: FORCE
	cd src && make install
	cd util && make install
	cd pcblib && make install
	cd doc-rnd && make install

linstall: FORCE
	cd src && make linstall
	cd util && make linstall
	cd pcblib && make linstall
	cd doc-rnd && make linstall

uninstall: FORCE
	cd src && make uninstall
	cd util && make uninstall
	cd pcblib && make uninstall
	cd doc-rnd && make uninstall

deb: FORCE
	fakeroot debian/rules clean
	fakeroot debian/rules binary

debclean: FORCE
	fakeroot debian/rules clean

FORCE:
