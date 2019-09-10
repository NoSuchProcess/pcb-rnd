TRUNK = ../..
SRC=$(TRUNK)/src

include $(SRC)/librnd.mak

CFLAGS = -I$(SRC) -I$(TRUNK) -I$(TRUNK)/src_3rd $(CFLAGS_LIBRND)
LDFLAGS = $(LDFLAGS_LIBRND)
LIBRND =$(SRC)/librnd-poly.a $(SRC)/librnd-hid.a  $(SRC)/librnd-3rd.a

all: librnd_test

test: librnd_test
	./librnd_test --gui batch

librnd_test: librnd_test.o $(LIBRND)
	$(CC) -o librnd_test librnd_test.o $(LIBRND) $(LDFLAGS)

librnd_test.o: librnd_test.c
	$(CC) $(CFLAGS) -c -o librnd_test.o librnd_test.c

clean:
	-rm librnd_test librnd_test.o
