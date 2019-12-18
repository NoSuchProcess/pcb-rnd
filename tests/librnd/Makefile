TRUNK = ../..
SRC=$(TRUNK)/src

include $(TRUNK)/Makefile.conf
include $(SRC)/librnd.mak

CFLAGS = $(CFLAGS_LIBRND)
LDFLAGS = $(LDFLAGS_LIBRND)
LIBRND =$(HL_LIBDIR)/librnd-poly.a $(HL_LIBDIR)/librnd-hid.a  $(HL_LIBDIR)/librnd-3rd.a
LIBRND_ROOT=include/$(HL_LIBDIR)/..

all: librnd_test librnd_includes

test: librnd_test librnd_includes
	./librnd_test --gui batch
	./librnd_includes

librnd_test: librnd_test.o $(LIBRND)
	$(CC) -o librnd_test librnd_test.o $(LIBRND) $(LDFLAGS)

librnd_test.o: librnd_test.c includes
	$(CC) $(CFLAGS) -c -o librnd_test.o librnd_test.c

librnd_includes: librnd_includes.o $(LIBRND)
	$(CC) -o librnd_includes librnd_includes.o $(LIBRND) $(LDFLAGS)

librd_includes.o: librnd_includes.c includes

includes: FORCE
	TDIR=`pwd` && cd ../../src make && $(MAKE) install_librnd HOW="install -f" DESTDIR=$$TDIR/include PREFIX=

FORCE:

clean:
	-rm librnd_test librnd_test.o
