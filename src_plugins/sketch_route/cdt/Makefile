CFLAGS=-Wall -O0 -g3 -I.
LDFLAGS=-lm

cdt_test: cdt_test.o cdt.o point.o edge.o triangle.o

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@
