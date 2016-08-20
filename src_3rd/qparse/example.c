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

		/* remove trailing newline (if we don't we just get an extra empty field at the end) */
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--)
			*end = '\0';

		/* split and print fields */
		printf("Splitting '%s':\n", s);
		argc = qparse(s, &argv);
		for(n = 0; n < argc; n++)
			printf(" [%d] '%s'\n", n, argv[n]);
		qparse_free(argc, &argv);
	}
	return 0;
}
