CFLAGS=-Wall -O0 -g3 -I. -I../../../src_3rd
LDFLAGS=-lm

cdt_test: cdt_test.o cdt.o point.o edge.o triangle.o

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@
