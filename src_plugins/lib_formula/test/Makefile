CFLAGS = -Wall -I. -I../../../src -I../../../src_3rd -I../../..
LDFLAGS = -lm -ldl
LIBRND = ../../../src/librnd-hid.a ../../../src/librnd-3rd.a

main: main.o $(LIBRND)

main.o: main.c ../impedance.c glue.c

