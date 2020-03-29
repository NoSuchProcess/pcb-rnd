TRUNK = ../..
LIBRND_ROOT=include/$(HL_LIBDIR)/..

include $(TRUNK)/Makefile.conf
include $(TRUNK)/src/librnd/core/librnd.mak

CFLAGS = $(CFLAGS_LIBRND)
LDFLAGS = $(LDFLAGS_LIBRND)
LIBRND = include/$(HL_LIBDIR)/librnd-poly.a include/$(HL_LIBDIR)/librnd-hid.a include/$(HL_LIBDIR)/librnd-3rd.a

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

$(LIBRND) includes: ../../src/Makefile ../../src/pcb-rnd
	TDIR=`pwd` && cd ../../src && $(MAKE) && $(MAKE) install_librnd HOW="install -f" DESTDIR=$$TDIR/include PREFIX=

clean:
	-rm librnd_test librnd_test.o librnd_includes librnd_includes.o
	TDIR=`pwd` && cd ../../src && $(MAKE) install_librnd_ HOW="uninstall -f" DESTDIR=$$TDIR/include PREFIX=
