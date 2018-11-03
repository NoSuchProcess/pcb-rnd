ROOT=../..
REM=$(ROOT)/src_plugins/hid_remote
CFLAGS = -g -I$(REM) -I$(ROOT)/src -I$(ROOT)

all: test_parse

ROOT=../..
include $(ROOT)/Makefile.conf

test_parse: test_parse.o
	$(CC) $(LDFLAGS) -o test_parse test_parse.o  $(LDLIBS)

test_parse.o: test_parse.c $(REM)/proto_lowcommon.h $(REM)/proto_lowparse.h $(REM)/proto_lowdbg.h

test:
	@./test_all.sh

clean:
	-$(SCCBOX) rm -f test_parse test_parse.o
