SPHASH=src_3rd/sphash/sphash
CFLAGS = -Wall -g -Isrc_3rd

main: main.o pcbdoc_ascii.o altium_kw.o

main.o: main.c pcbdoc_ascii.h

pcbdoc_ascii.o: pcbdoc_ascii.c pcbdoc_ascii.h

altium_kw.c altium_kw.h: altium_kw.sphash
	$(SPHASH) $(VERBOSE) -i --multi --prefix altium_kw --out altium_kw < altium_kw.sphash

altium_kw.o: altium_kw.c altium_kw.h