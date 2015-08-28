#include <stdlib.h>
#include <string.h>
#include "paths.h"

char *homedir;

void paths_init_homedir(void)
{
	const char *tmps;
	tmps = getenv ("HOME");
	if (tmps == NULL)
		tmps = getenv ("USERPROFILE");
	if (tmps != NULL)
		homedir = strdup (tmps);
	else
		homedir = NULL;
}


void resolve_paths(const char **in, char **out, int numpaths)
{
	for(out; numpaths > 0; numpaths--,in++,out++) {
		if (*in != NULL) {
			if (**in == '~') {
				int l1, l2;
				if (homedir == NULL) {
					Message("can't resolve home dir required for path %s\n", *in);
					exit(1);
				}
				/* avoid Concat() here to reduce dependencies for external tools */
				l1 = strlen(homedir);
				l2 = strlen((*in)+1);
				*out = malloc(l1+l2+4);
				sprintf(*out, "%s/%s", homedir, (*in)+1);
			}
			else 
				*out = strdup(*in);
		}
		else
			*out = NULL;
	}
}

void resolve_path(const char *in, char **out)
{
	resolve_paths(&in, out, 1);
}

char *resolve_path_inplace(char *in)
{
	char *out;
	resolve_path(in, &out);
	free(in);
	return out;
}
