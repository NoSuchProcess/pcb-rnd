all:
	cd ../../src && make mod_fp_wget

CFLAGS = -Wall -g -I../../src -I../.. -I../../src_3rd

test: tester

tester: tester.o gedasymbols.o wget_common.o ../../src_3rd/genvector/gds_char.o

clean:
	rm *.o *.so 2>/dev/null ; true

