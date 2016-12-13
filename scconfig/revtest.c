#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Rev.h"

const char *act_names[] = {
	"distclean",
	"configure",
	"make clean",
	NULL
};

const char *act_desc[] = {
	"run 'make distclean' in trunk/",
	"rerun ./configure",
	"run 'make clean' in trunk/",
	NULL
};

int act_needed[16];

int main(int argc, char *argv[])
{
	char line[8192];
	int res = 0, stamp = 0, banner = 0;
	FILE *f = NULL;

	if (argc > 1) {
		f = fopen(argv[1], "r");
	}
	if (f != NULL) {
		fscanf(f, "%d", &stamp);
		fclose(f);
	}

	while(fgets(line, sizeof(line), stdin) != NULL) {
		char *end;
		int rev;

		if (*line == '#')
			continue;
		rev = strtol(line, &end, 10);
		if (*end != '\t')
			continue;

		if (rev > stamp) {
			char *act, *txt;
			const char **a;
			int idx;
			res = 1;
			act = end;
			while(*act == '\t')
				act++;
			txt = strchr(act, '\t');
			if (txt != NULL) {
				*txt = '\0';
				txt++;
				while(*txt == '\t')
					txt++;
			}
			if (!banner) {
				fprintf(stderr, "\nYour source tree is stale (last configured after config-milestone r%d).\nRecent new features:\n", stamp);
				banner = 1;
			}
			fprintf(stderr, "\nr%d: %s\n", rev, txt);
			for(idx = 0, a = act_names; *a != NULL; idx++, a++)
				if (strstr(act, *a) != NULL)
					act_needed[idx]++;
		}
	}

	if (res) {
		const char **a;
		int idx;

		fprintf(stderr, "(Stamp: %d myrev: %d)\n", stamp, myrev);
		fprintf(stderr, "\nBefore running make, please do the following actions in this order:\n");
		for(idx = 0, a = act_names; *a != NULL; idx++, a++)
			if (act_needed[idx])
				fprintf(stderr, "  %s\n", act_desc[idx]);
		fprintf(stderr, "\n");
	}

	return res;
}
