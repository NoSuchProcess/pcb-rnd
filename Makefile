all:
	cd src && make
	cd tutorial && make

clean:
	cd src && make clean
	cd tutorial && make clean

install:
	cd src && make install
	cd tutorial && make install


