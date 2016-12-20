ROOT=../..
REM=$(ROOT)/src_plugins/hid_remote
CFLAGS = -Wall -g -I$(REM) -I$(ROOT)/src -I$(ROOT)

all: test_parse

test_parse: test_parse.o
	$(CC) $(LDFLAGS) -o test_parse test_parse.o  $(LDLIBS)

test_parse.o: test_parse.c $(REM)/proto_lowcommon.h $(REM)/proto_lowparse.h $(REM)/proto_lowdbg.h

test:
	@./test_all.sh

clean:
	-rm test_parse test_parse.o
