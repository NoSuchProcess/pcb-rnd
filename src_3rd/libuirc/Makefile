CFLAGS = -Wall -g -I..
LDFLAGS = -lm
LIBS = ../libporty_net/libportytcp4.o ../genvector/gds_char.o ../../src/librnd/core/compat_misc.o

main: main.o libuirc.o $(LIBS)

main.o: main.c

libuirc.o: libuirc.c libuirc.h
