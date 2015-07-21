all:
	cd gts && make
	cd src && make
	cd util && make
	cd pcblib && make
	cd tutorial && make

clean:
	cd gts && make clean
	cd src && make clean
	cd util && make clean
	cd pcblib && make clean
	cd tutorial && make clean

install:
	cd src && make install
	cd util && make install
	cd pcblib && make install
	cd tutorial && make install

linstall:
	cd src && make linstall
	cd util && make linstall
	cd pcblib && make linstall
	cd tutorial && make linstall

uninstall:
	cd src && make uninstall
	cd util && make uninstall
	cd pcblib && make uninstall
	cd tutorial && make uninstall

deb:
	fakeroot debian/rules binary

debclean:
	fakeroot debian/rules clean
