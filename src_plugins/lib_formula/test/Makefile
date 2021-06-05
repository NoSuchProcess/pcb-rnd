ROOT=../../..
include $(ROOT)/Makefile.conf
include $(LIBRND_MAK)

CFLAGS = -Wall -I. -I$(ROOT)/src -I$(ROOT)/src_3rd -I$(ROOT) -I$(LIBRND_INCDIR)/librnd/src_3rd
LDFLAGS = -lm -ldl
LIBRND = -lrnd-hid -lrnd-3rd $(LDFLAGS_LIBRND_FUNGW)

main: main.o
	$(CC) -o main main.o $(LDFLAGS) $(LIBRND)

main.o: main.c ../impedance.c

