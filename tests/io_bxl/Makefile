ROOT=../..
PLG=$(ROOT)/src_plugins/io_bxl

include $(ROOT)/Makefile.conf

CFLAGS = $(PCB_RND_C89FLAGS) -I$(ROOT) -I$(ROOT)/src_3rd -I$(ROOT)/src -I$(PLG) $(CFLAGS_LIBRND_FUNGW)
LDLIBS = -lm $(LDFLAGS_LIBRND_FUNGW)

test_parse: test_parse.o glue.o $(PLG)/bxl_lex.o $(PLG)/bxl_gram.o $(ROOT)/src_3rd/librnd-local/src/librnd-hid.a $(ROOT)/src_3rd/librnd-local/src/librnd-3rd.a

#$(PLG)/bxl_lex.c $(PLG)/bxl_lex.h: $(PLG)/bxl_lex.ul $(PLG)/bxl_gram.h $(PLG)/bxl.h
#	ureglex -l $(PLG)/bxl_lex.ul -c $(PLG)/bxl_lex.c -h $(PLG)/bxl_lex.h

#$(PLG)/bxl_gram.c $(PLG)/bxl_gram.h: $(PLG)/bxl_gram.y
#	byaccic -v $(PLG)/bxl_gram.desc -o $(PLG)/bxl_gram.c -H $(PLG)/bxl_gram.h $(PLG)/bxl_gram.y

test_parse.o: $(PLG)/bxl_gram.h $(PLG)/bxl_lex.h
