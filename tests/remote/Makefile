ROOT=../..
REM=$(ROOT)/src_plugins/hid_remote
CFLAGS = -Wall -g -I$(REM) -I$(ROOT)/src -I$(ROOT)

test_parse: test_parse.o

test_parse.o: test_parse.c $(REM)/proto_lowcommon.h $(REM)/proto_lowparse.h $(REM)/proto_lowdbg.h