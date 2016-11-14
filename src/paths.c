/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Resolve paths, build paths using template */
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "paths.h"
#include "error.h"
#include "conf_core.h"

/* don't include board.h or compat_misc.h because of gsch2pcb-rnd */
const char *pcb_board_get_filename(void);
const char *pcb_board_get_name(void);
int pcb_getpid(void);


void resolve_paths(const char **in, char **out, int numpaths, unsigned int extra_room)
{
	const char *subst_to;
	int subst_offs;
	for (; numpaths > 0; numpaths--, in++, out++) {
		if (*in != NULL) {
			if (**in == '~') {
				int l1, l2;
				if (conf_core.rc.path.home == NULL) {
					pcb_message(PCB_MSG_DEFAULT, "can't resolve home dir required for path %s\n", *in);
					exit(1);
				}
				subst_to = conf_core.rc.path.home;
				subst_offs = 1;
				replace:;
				/* avoid pcb_concat() here to reduce dependencies for external tools */
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
							const char *si;
							char *so;
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
					pcb_message(PCB_MSG_DEFAULT, "can't resolve conf-hash dir required for path %s\n", *in);
					*out = NULL;
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

int pcb_build_fn_cb(void *ctx, gds_t *s, const char **input)
{
	char buff[20];
	const char *name;



	switch(**input) {
		case 'P':
			sprintf(buff, "%.8i", pcb_getpid());
			gds_append_str(s, buff);
			(*input)++;
			return 0;
		case 'F':
			name = pcb_board_get_filename();
			gds_append_str(s, (name != NULL) ? name : "no_file_name");
			(*input)++;
			return 0;
		case 'B':
			name = pcb_board_get_filename();
			if (name != NULL) {
				const char *bn = strrchr(name, '/');
				if (bn != NULL)
					bn++;
				else
					bn = name;
				gds_append_str(s, bn);
			}
			else
				gds_append_str(s, "no_file_name");
			(*input)++;
			return 0;
		case 'D':
			name = pcb_board_get_filename();
			if (name != NULL) {
				char *bn = strrchr(name, '/');
				if (bn != NULL)
					gds_append_len(s, name, bn-name+1);
				else
					gds_append_str(s, "./");
			}
			else
				gds_append_str(s, "./");
			(*input)++;
			return 0;
		case 'N':
			name = pcb_board_get_name();
			gds_append_str(s, (name != NULL) ? name : "no_name");
			(*input)++;
			return 0;
		case 'T':
			sprintf(buff, "%lu", (unsigned long int)time(NULL));
			gds_append_str(s, buff);
			(*input)++;
			return 0;
	}
	return -1;
}

int pcb_build_argfn_cb(void *ctx_, gds_t *s, const char **input)
{
	if ((**input >= 'a') && (**input <= 'z')) {
		int idx = **input - 'a';
		pcb_build_argfn_t *ctx = ctx_;
		if (ctx->params[idx] == NULL)
			return -1;
		gds_append_str(s, ctx->params[idx]);
		(*input)++;
		return 0;
	}
	return pcb_build_fn_cb(NULL, s, input);
}

char *pcb_build_fn(const char *template)
{
	return pcb_strdup_subst(template, pcb_build_fn_cb, NULL);
}

char *pcb_build_argfn(const char *template, pcb_build_argfn_t *arg)
{
	return pcb_strdup_subst(template, pcb_build_argfn_cb, arg);
}
