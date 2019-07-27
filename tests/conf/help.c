#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *help[] = {
	"dump", "native",
	"Dump all settings as ended up in the native database after the last merge/udpate.",

	"dump", "lihata role",
	"Dump all in-memory lihata settings of a given role. For roles, see: help roles",

	"load", "role filename",
	"Load a lihata file as role",

	"load", "*",
	"Load all roles from disk - the same way as it happens on a normal pcb-rnd startup.",

	"paste", "role lhttxt",
	"Paste in-line lihata document lhttxt as role, like if it was loaded from a file",

	"policy", "pol",
	"Change current set-policy to pol. This only affects whether a set command inserts, appends or overwrites list-type settings. For valid policies, see: help policies",

	"role", "rol",
	"Change current set-policy to rol. This only affects the destination of subsequent set commands. For valid roles, see: help roles",

	"chprio", "prio",
	"Change the priority of the first confroot of the current role's in-memory lihata document to prio and merge. Prio is an integer value.",

	"chpolicy", "pol",
	"Change the policy of the first confroot of the current role's in-memory lihata document to pol and merge. Pol is a policy, see: help policies",

	"set", "path value",
	"Call pcb_conf_set() on a given path with the given value, using the current set-role and the current set-policy. See also: help role; help policy.",

	"watch", "path",
	"Announce changes of a given path. See also: help unwatch",

	"unwatch", "path",
	"Stop announcing changes of a given path. See also: help watch",

	"notify", "on",
	"Turn on global notification on config changes.",

	"notify", "off",
	"Turn off global notification on config changes.",

	"echo", "text...",
	"Print multi-word text.",

	"reset", "role",
	"Reset (make empty) the in-memory lihata document of role; see also: help roles",

	"reset", "*",
	"Reset (make empty) all in-memory lihata documents",

/* misc */
	"roles", NULL,
	"Valid roles: system, defaultpcb, user, env, project, design, cli",

	"policies", NULL,
	"Valid policies: prepend, append, overwrite",

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
