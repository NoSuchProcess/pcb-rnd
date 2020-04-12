ROOT=../..
PLG=$(ROOT)/src_plugins/io_bxl
#TODO: pick up config cflags and ldflags
CFLAGS = -g -Wall -I$(PLG) -I$(ROOT)/src -I$(ROOT)/src_3rd -I$(ROOT)
LDFLAGS = -lm

test_parse: test_parse.o glue.o $(PLG)/bxl_lex.o $(PLG)/bxl_gram.o $(ROOT)/src/librnd-hid.a $(ROOT)/src/librnd-3rd.a

$(PLG)/bxl_lex.c $(PLG)/bxl_lex.h: $(PLG)/bxl_lex.ul $(PLG)/bxl_gram.h $(PLG)/bxl.h
	ureglex -l $(PLG)/bxl_lex.ul -c $(PLG)/bxl_lex.c -h $(PLG)/bxl_lex.h

$(PLG)/bxl_gram.c $(PLG)/bxl_gram.h: $(PLG)/bxl_gram.y
	byaccic -v $(PLG)/bxl_gram.desc -o $(PLG)/bxl_gram.c -H $(PLG)/bxl_gram.h $(PLG)/bxl_gram.y

test_parse.o: $(PLG)/bxl_gram.h $(PLG)/bxl_lex.h
