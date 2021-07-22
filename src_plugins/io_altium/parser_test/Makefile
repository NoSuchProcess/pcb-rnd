include ../../../Makefile.conf
include $(LIBRND_MAK)
SPHASH=$(LIBRND_LIBDIR)/librnd3/sphash
CFLAGS = -Wall -g -I.. -I. $(CFLAGS_LIBRND)

main: main.o pcbdoc_ascii.o ../altium_kw_sphash.o

main.o: main.c ../pcbdoc_ascii.h ../altium_kw_sphash.h

pcbdoc_ascii.o: ../pcbdoc_ascii.c ../pcbdoc_ascii.h ../altium_kw_sphash.h
	$(CC) -o $@ -c $(CFLAGS) ../pcbdoc_ascii.c

../altium_kw_sphash.c ../altium_kw_sphash.h: ../altium_kw.sphash
	$(SPHASH) $(VERBOSE) -i --multi --prefix altium_kw --out ../altium_kw < ../altium_kw_sphash.sphash

altium_kw_sphash.o: ../altium_kw_sphash.c ../altium_kw_sphash.h