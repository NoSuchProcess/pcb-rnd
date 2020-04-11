CFLAGS = -g -Wall

lex_test: lex_test.o bxl_lex.o bxl_gram.o

bxl_lex.c bxl_lex.h: bxl_lex.ul bxl_gram.h bxl.h
	ureglex -l bxl_lex.ul -c bxl_lex.c -h bxl_lex.h

bxl_gram.c bxl_gram.h: bxl_gram.y
	byaccic -v bxl_gram.desc -o bxl_gram.c -H bxl_gram.h bxl_gram.y

lex_test.o: bxl_gram.h bxl_lex.h
