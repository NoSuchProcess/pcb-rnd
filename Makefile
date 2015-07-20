all:
	cd gts && make
	cd src && make
#	cd lib && make
	cd tutorial && make
	cd util && make


clean:
	cd gts && make clean
	cd src && make clean
#	cd lib && make clean
	cd tutorial && make clean
	cd util && make clean

install:
	cd src && make install
	cd lib && make install
#	cd newlib && make install
	cd tutorial && make install
	cd util && make install

linstall:
	cd src && make linstall
	cd lib && make linstall
#	cd newlib && make linstall
	cd tutorial && make linstall
	cd util && make linstall

uninstall:
	cd src && make uninstall
	cd lib && make uninstall
#	cd newlib && make uninstall
	cd tutorial && make uninstall
	cd util && make uninstall

