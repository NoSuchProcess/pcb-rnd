ROOT=../..
UNM=$(ROOT)/src_plugins/io_kicad_legacy
LHT=$(ROOT)/src_3rd/liblihata
SRC=$(ROOT)/src

CFLAGS = -Wall -I$(UNM) -I$(LHT)
LIB_OBJS=$(LHT)/genht/htsp.o $(LHT)/genht/hash.o $(SRC)/compat_misc.o

tester: tester.o $(UNM)/uniq_name.o $(LIB_OBJS)

tester.o: tester.c $(UNM)/uniq_name.h

$(UNM)/uniq_name.o: $(UNM)/uniq_name.c $(UNM)/uniq_name.h