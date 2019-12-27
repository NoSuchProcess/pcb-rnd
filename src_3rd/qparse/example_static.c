#include <stdio.h>
#include <string.h>
#include "qparse.h"

/* Read lines of text from stdin and split them in fields using qparse.
   Uses static argv[] and buffer */

#define SYNTAX QPARSE_DOUBLE_QUOTE | QPARSE_SINGLE_QUOTE | QPARSE_TERM_SEMICOLON | QPARSE_SEP_COMMA | QPARSE_COLON_LAST | QPARSE_NO_ARGV_REALLOC

int main()
{
	char s[1024];
	char *tmp, *argv_static[8], **argv = argv_static;
	unsigned int argv_alloced = 8;
	size_t tmp_len;

	tmp_len = sizeof(s)+8;
	tmp = malloc(tmp_len);

	while(fgets(s, sizeof(s), stdin) != NULL) {
		int n, argc;
		char *end;

		size_t cons = 424242;

		/* remove trailing newline (if we don't we just get an extra empty field at the end) */
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--)
			*end = '\0';

		/* split and print fields */
		printf("Splitting '%s':\n", s);
		argc = qparse4(s, &argv, &argv_alloced, SYNTAX, &cons, &tmp, &tmp_len);
		for(n = 0; n < argc; n++)
			printf(" [%d] '%s'\n", n, argv[n]);
		qparse_free_strs(argc, &argv);
		printf("consumed: %ld bytes\n", cons);
	}
	qparse4_free(&argv, &argv_alloced, SYNTAX, &tmp, &tmp_len);

	return 0;
}
