ROOT=../..
PLG=$(ROOT)/src_plugins/io_bxl
CFLAGS = -g -Wall -I$(PLG)

test_parse: test_parse.o $(PLG)/bxl_lex.o $(PLG)/bxl_gram.o

$(PLG)/bxl_lex.c $(PLG)/bxl_lex.h: $(PLG)/bxl_lex.ul $(PLG)/bxl_gram.h $(PLG)/bxl.h
	ureglex -l $(PLG)/bxl_lex.ul -c $(PLG)/bxl_lex.c -h $(PLG)/bxl_lex.h

$(PLG)/bxl_gram.c $(PLG)/bxl_gram.h: $(PLG)/bxl_gram.y
	byaccic -v $(PLG)/bxl_gram.desc -o $(PLG)/bxl_gram.c -H $(PLG)/bxl_gram.h $(PLG)/bxl_gram.y

test_parse.o: $(PLG)/bxl_gram.h $(PLG)/bxl_lex.h
