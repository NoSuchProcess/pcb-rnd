all:
	cd ../../src && make mod_import_hyp

hyp_l.o: hyp_y.h

parser.o: hyp_l.h hyp_y.h

clean:
	rm *.o *.so hyp_l.c hyp_l.h hyp_y.c hyp_y.h 2>/dev/null ; true

