all:
	cd src && make
	cd util && make
	cd pcblib && make

clean:
	cd src && make clean
	cd util && make clean
	cd pcblib && make clean

distclean:
	make clean ; true
	cd scconfig && make clean ; true
	cd src_3rd/genlist && make clean ; true
	cd src_3rd/genregex && make clean ; true
	cd src_3rd/genvector && make clean ; true
	cd src_3rd/gts && make clean ; true
	cd src_3rd/liblihata && make clean ; true
	cd src_3rd/liblihata/genht && make clean ; true
	cd src_3rd/qparse && make clean ; true

install:
	cd src && make install
	cd util && make install
	cd pcblib && make install

linstall:
	cd src && make linstall
	cd util && make linstall
	cd pcblib && make linstall

uninstall:
	cd src && make uninstall
	cd util && make uninstall
	cd pcblib && make uninstall

deb:
	fakeroot debian/rules clean
	fakeroot debian/rules binary

debclean:
	fakeroot debian/rules clean
