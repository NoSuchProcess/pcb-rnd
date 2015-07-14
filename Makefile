all:
	cd gts && make
	cd src && make
	cd lib && make
	cd tutorial && make


clean:
	cd gts && make clean
	cd src && make clean
	cd lib && make clean
	cd tutorial && make clean

install:
	cd src && make install
	cd lib && make install
	cd tutorial && make install


