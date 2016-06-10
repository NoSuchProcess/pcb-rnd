#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "paths.h"
#include "error.h"
#include "conf.h"

#warning TODO: move this in conf_core
char *homedir;

void paths_init_homedir(void)
{
	const char *tmps;
	tmps = getenv("HOME");
	if (tmps == NULL)
		tmps = getenv("USERPROFILE");
	if (tmps != NULL)
		homedir = strdup(tmps);
	else
		homedir = NULL;
}


void resolve_paths(const char **in, char **out, int numpaths, unsigned int extra_room)
{
	char *subst_to;
	int subst_offs;
	for (out; numpaths > 0; numpaths--, in++, out++) {
		if (*in != NULL) {
			if (**in == '~') {
				int l1, l2;
				if (homedir == NULL) {
					Message("can't resolve home dir required for path %s\n", *in);
					exit(1);
				}
				subst_to = homedir;
				subst_offs = 1;
				replace:;
				/* avoid Concat() here to reduce dependencies for external tools */
				l1 = strlen(subst_to);
				l2 = strlen((*in) + 1);
				*out = malloc(l1 + l2 + 4 + extra_room);
				sprintf(*out, "%s%s", subst_to, (*in) + subst_offs);
			}
			else if (**in == '$') {
				if ((*in)[1] == '(') {
					char *end = strchr((*in)+2, ')');
					if (end != NULL) {
						char hash_path[128];
						int len = end - (*in);
						if (len < sizeof(hash_path)-1) {
							conf_native_t *cn;
							char *si, *so;
							int n;

							(*in) += 2;
							len -= 2;
							for(si = *in, so = hash_path, n=0; n < len; n++,si++,so++) {
								if (*si == '.')
									*so = '/';
								else
									*so = *si;
							}
							*so = 0;
							cn = conf_get_field(hash_path);
							if ((cn != NULL) && (cn->type == CFN_STRING)) {
								subst_to = cn->val.string[0];
								subst_offs = len+1;
								goto replace;
							}
						}
					}
				}
				else
					*out = NULL;
			}
			else {
				*out = malloc(strlen(*in) + 1 + extra_room);
				strcpy(*out, *in);
			}
		}
		else
			*out = NULL;
	}
}

void resolve_path(const char *in, char **out, unsigned int extra_room)
{
	resolve_paths(&in, out, 1, extra_room);
}

char *resolve_path_inplace(char *in, unsigned int extra_room)
{
	char *out;
	resolve_path(in, &out, extra_room);
	free(in);
	return out;
}
