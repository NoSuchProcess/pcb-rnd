#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *help[] = {
	"dump", "native",
	"Dump all settings as ended up in the native database after the last merge/udpate.",

	"dump", "lihata role",
	"Dump all in-memory lihata settings of a given role. For roles, see: help roles",

	"roles", NULL,
	"Valid roles: system, defaultpcb, user, env, project, design, cli",

	NULL, NULL, NULL
};

void cmd_help(char *arg)
{
	const char **s;

	printf("\n");

	if (arg == NULL) {
		const char *last = "";
		printf("Try help topic. Available topics:\n");
		for(s = help; s[2] != NULL; s+=3) {
			if (strcmp(last, s[0]) != 0)
				printf("  %s\n", s[0]);
			last = s[0];
		}
		return;
	}

	for(s = help; s[2] != NULL; s+=3) {
		if (strcmp(arg, s[0]) == 0) {
			printf("%s %s\n", s[0], (s[1] == NULL ? "" : s[1]));
			printf("%s\n\n", s[2]);
		}
	}
}
