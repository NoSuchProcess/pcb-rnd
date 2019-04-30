/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Resolve paths, build paths using template */
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "paths.h"
#include "error.h"
#include "hidlib_conf.h"

/* don't include board.h or compat_misc.h because of gsch2pcb-rnd */
const char *pcb_board_get_filename(void);
const char *pcb_board_get_name(void);
int pcb_getpid(void);


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
			gds_append_str(s, (name != NULL) ? name : "PCB-anon");
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
	return pcb_strdup_subst(template, pcb_build_fn_cb, NULL, PCB_SUBST_ALL);
}

char *pcb_build_argfn(const char *template, pcb_build_argfn_t *arg)
{
	return pcb_strdup_subst(template, pcb_build_argfn_cb, arg, PCB_SUBST_ALL);
}

int pcb_subst_append(gds_t *s, const char *template, int (*cb)(void *ctx, gds_t *s, const char **input), void *ctx, pcb_strdup_subst_t flags, size_t extra_room)
{
	const char *curr, *next;

	if (template == NULL)
		return -1;

	gds_init(s);

	if (*template == '?')
		template++;

	if ((*template == '~') && (flags & PCB_SUBST_HOME)) {
		if (*pcbhlc_rc_path_home == NULL) {
			pcb_message(PCB_MSG_ERROR, "pcb_strdup_subst(): can't resolve home dir required for path %s\n", template);
			goto error;
		}
		gds_append_str(s, *pcbhlc_rc_path_home);
		template++;
	}

	for(curr = template;;) {
		next = strpbrk(curr, "%$\\");
		if (next == NULL) {
			gds_append_str(s, curr);
			gds_enlarge(s, gds_len(s) + extra_room);
			return 0;
		}
		if (next > curr)
			gds_append_len(s, curr, next-curr);

		switch(*next) {
			case '\\':
				if (flags & PCB_SUBST_BACKSLASH) {
					char c;
					next++;
					switch(*next) {
						case 'n': c = '\n'; break;
						case 'r': c = '\r'; break;
						case 't': c = '\t'; break;
						case '\\': c = '\\'; break;
						default: c = *next;
					}
					gds_append(s, c);
					curr = next;
				}
				else {
					gds_append(s, *next);
					curr = next+1;
				}
				break;
			case '%':
				if (flags & PCB_SUBST_PERCENT) {
					next++;
					switch(*next) {
						case '%':
							gds_append(s, '%');
							curr = next+1;
							break;
						default:
							if (cb(ctx, s, &next) != 0) {
								/* keep the directive intact */
								gds_append(s, '%');
							}
							curr = next;
					}
				}
				else {
					gds_append(s, '%');
					curr = next+1;
				}
				break;
			case '$':
				if (flags & PCB_SUBST_CONF) {
					const char *start, *end;
					next++;
					switch(*next) {
						case '(':
							next++;
							start = next;
							end = strchr(next, ')');
							if (end != NULL) {
								conf_native_t *cn;
								char path[256], *q;
								size_t len = end - start;
								if (len > sizeof(path) - 1) {
									if (!(flags & PCB_SUBST_QUIET))
										pcb_message(PCB_MSG_ERROR, "pcb_strdup_subst(): can't resolve $() conf var, name too long: %s\n", start);
									goto error;
								}
								memcpy(path, start, len);
								path[len] = '\0';
								for(q = path; *q != '\0'; q++)
									if (*q == '.')
										*q = '/';
								cn = conf_get_field(path);
								if (cn == NULL) {
									if (!(flags & PCB_SUBST_QUIET))
										pcb_message(PCB_MSG_ERROR, "pcb_strdup_subst(): can't resolve $(%s) conf var: not found in the conf tree\n", path);
									goto error;
								}
								if (cn->type != CFN_STRING) {
									if (!(flags & PCB_SUBST_QUIET))
										pcb_message(PCB_MSG_ERROR, "pcb_strdup_subst(): can't resolve $(%s) conf var: value type is not string\n", path);
									goto error;
								}
								if (cn->val.string[0] != NULL)
									gds_append_str(s, cn->val.string[0]);
								curr = end+1;
							}
							else {
								if (!(flags & PCB_SUBST_QUIET))
									pcb_message(PCB_MSG_ERROR, "pcb_strdup_subst(): unterminated $(%s)\n", start);
								goto error;
							}
							break;
						case '$':
							gds_append(s, '$');
							curr = next+1;
							break;
						default:
							gds_append(s, '$');
							curr = next;
							break;
					}
				}
				else {
					gds_append(s, '$');
					curr = next+1;
				}
				break;
		}
	}
	abort(); /* can't get here */

	error:;
	return -1;
}

static char *pcb_strdup_subst_(const char *template, int (*cb)(void *ctx, gds_t *s, const char **input), void *ctx, pcb_strdup_subst_t flags, size_t extra_room)
{
	gds_t s;

	if (template == NULL)
		return NULL;

	memset(&s, 0, sizeof(s)); /* shouldn't do gds_init() here, pcb_subst_append() will do that */

	if (pcb_subst_append(&s, template, cb, ctx, flags, extra_room) == 0)
		return s.array;

	gds_uninit(&s);
	return NULL;
}

char *pcb_strdup_subst(const char *template, int (*cb)(void *ctx, gds_t *s, const char **input), void *ctx, pcb_strdup_subst_t flags)
{
	return pcb_strdup_subst_(template, cb, ctx, flags, 0);
}

void pcb_paths_resolve(const char **in, char **out, int numpaths, unsigned int extra_room, int quiet)
{
	pcb_strdup_subst_t flags = PCB_SUBST_ALL;
	if (quiet)
		flags |= PCB_SUBST_QUIET;
	for (; numpaths > 0; numpaths--, in++, out++)
		*out = pcb_strdup_subst_(*in, pcb_build_fn_cb, NULL, flags, extra_room);
}

void pcb_path_resolve(const char *in, char **out, unsigned int extra_room, int quiet)
{
	pcb_paths_resolve(&in, out, 1, extra_room, quiet);
}

char *pcb_path_resolve_inplace(char *in, unsigned int extra_room, int quiet)
{
	char *out;
	pcb_path_resolve(in, &out, extra_room, quiet);
	free(in);
	return out;
}
