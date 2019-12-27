#include <stdio.h>
#include <string.h>
#include "qparse.h"

/* Read lines of text from stdin and split them in fields using qparse. */

int main()
{
	char s[1024];
	while(fgets(s, sizeof(s), stdin) != NULL) {
		int n, argc;
		char *end, **argv;
		size_t cons = 424242;

		/* remove trailing newline (if we don't we just get an extra empty field at the end) */
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--)
			*end = '\0';

		/* split and print fields */
		printf("Splitting '%s':\n", s);
		argc = qparse3(s, &argv, QPARSE_DOUBLE_QUOTE | QPARSE_SINGLE_QUOTE | QPARSE_TERM_SEMICOLON | QPARSE_SEP_COMMA | QPARSE_COLON_LAST, &cons);
		for(n = 0; n < argc; n++)
			printf(" [%d] '%s'\n", n, argv[n]);
		qparse_free(argc, &argv);
		printf("consumed: %ld bytes\n", cons);
	}
	return 0;
}
