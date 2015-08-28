#include <stdlib.h>
#include <string.h>
#include "paths.h"

extern char *homedir;

void resolve_paths(const char **in, char **out, int numpaths)
{
	for(out; numpaths > 0; numpaths--,in++,out++) {
		if (*in != NULL) {
			if (**in == '~') {
				if (homedir == NULL) {
					Message("can't resolve home dir required for path %s\n", *in);
					exit(1);
				}
				*out = Concat(homedir, (*in)+1, NULL);
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
