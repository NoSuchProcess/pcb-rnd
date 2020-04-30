/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2019 Tibor 'Igor2' Palinkas
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
 *
 */

#include <librnd/config.h>
#include <string.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/compat_misc.h>

rnd_hid_attr_node_t *rnd_hid_attr_nodes = 0;

void rnd_export_register_opts(rnd_export_opt_t *a, int n, const char *cookie, int copy)
{
	rnd_hid_attr_node_t *ha;

	/* printf("%d attributes registered\n", n); */
	ha = malloc(sizeof(rnd_hid_attr_node_t));
	ha->next = rnd_hid_attr_nodes;
	rnd_hid_attr_nodes = ha;
	ha->opts = a;
	ha->n = n;
	ha->cookie = cookie;
}

void rnd_export_uninit(void)
{
	rnd_hid_attr_node_t *ha, *next;
	for (ha = rnd_hid_attr_nodes; ha; ha = next) {
		next = ha->next;
		if (ha->cookie != NULL)
			fprintf(stderr, "WARNING: attribute %s by %s is not uninited, check your plugins' uninit!\n", ha->opts->name, ha->cookie);
		free(ha);
	}
	rnd_hid_attr_nodes = NULL;
}

void rnd_export_remove_opts_by_cookie(const char *cookie)
{
	rnd_hid_attr_node_t *ha, *next, *prev = NULL;
	for (ha = rnd_hid_attr_nodes; ha; ha = next) {
		next = ha->next;
		if (ha->cookie == cookie) {
			if (prev == NULL)
				rnd_hid_attr_nodes = next;
			else
				prev->next = next;
			free(ha);
		}
		else
			prev = ha;
	}
}

int rnd_hid_parse_command_line(int *argc, char ***argv)
{
	rnd_hid_attr_node_t *ha;
	int i, e, ok;
	char *filename = NULL;

	for (ha = rnd_hid_attr_nodes; ha; ha = ha->next)
		for (i = 0; i < ha->n; i++) {
			rnd_export_opt_t *a = ha->opts + i;
			switch (a->type) {
			case RND_HATT_LABEL:
				break;
			case RND_HATT_INTEGER:
				if (a->value)
					*(int *) a->value = a->default_val.lng;
				break;
			case RND_HATT_COORD:
				if (a->value)
					*(rnd_coord_t *) a->value = a->default_val.crd;
				break;
			case RND_HATT_BOOL:
				if (a->value)
					*(char *) a->value = a->default_val.lng;
				break;
			case RND_HATT_REAL:
				if (a->value)
					*(double *) a->value = a->default_val.dbl;
				break;
			case RND_HATT_STRING:
				if (a->value)
					*(const char **) a->value = rnd_strdup(RND_EMPTY(a->default_val.str));
				break;
			case RND_HATT_ENUM:
				if (a->value)
					*(int *) a->value = a->default_val.lng;
				break;
			case RND_HATT_UNIT:
				if (a->value)
					*(int *) a->value = a->default_val.lng;
				break;
			default:
				if (RND_HATT_IS_COMPOSITE(a->type)) /* function callback */
					break;
				rnd_message(RND_MSG_ERROR, "Invalid attribute type %d for attribute %s\n", a->type, a->name);
				abort();
			}
		}

	while(*argc > 0)
	if (*argc && (*argv)[0][0] == '-' && (*argv)[0][1] == '-') {
		int bool_val;
		int arg_ofs;

		bool_val = 1;
		arg_ofs = 2;
	try_no_arg:
		for (ha = rnd_hid_attr_nodes; ha; ha = ha->next)
			for (i = 0; i < ha->n; i++)
				if (strcmp((*argv)[0] + arg_ofs, ha->opts[i].name) == 0) {
					rnd_export_opt_t *a = ha->opts + i;
					char *ep;
					const rnd_unit_t *unit;
					switch (ha->opts[i].type) {
					case RND_HATT_LABEL:
						break;
					case RND_HATT_INTEGER:
						if (a->value)
							*(int *) a->value = strtol((*argv)[1], 0, 0);
						else
							a->default_val.lng = strtol((*argv)[1], 0, 0);
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_COORD:
						if (a->value)
							*(rnd_coord_t *) a->value = rnd_get_value((*argv)[1], NULL, NULL, NULL);
						else
							a->default_val.crd = rnd_get_value((*argv)[1], NULL, NULL, NULL);
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_REAL:
						if (a->value)
							*(double *) a->value = strtod((*argv)[1], 0);
						else
							a->default_val.dbl = strtod((*argv)[1], 0);
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_STRING:
						if (a->value)
							*(char **) a->value = rnd_strdup(RND_EMPTY((*argv)[1]));
						else
							a->default_val.str = rnd_strdup(RND_EMPTY((*argv)[1]));
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_BOOL:
						if (a->value)
							*(char *) a->value = bool_val;
						else
							a->default_val.lng = bool_val;
						break;
					case RND_HATT_ENUM:
						ep = (*argv)[1];
						ok = 0;
						for (e = 0; a->enumerations[e]; e++)
							if (strcmp(a->enumerations[e], ep) == 0) {
								ok = 1;
								a->default_val.lng = e;
								a->default_val.str = ep;
								break;
							}
						if (!ok) {
							fprintf(stderr, "ERROR:  \"%s\" is an unknown value for the --%s option\n", (*argv)[1], a->name);
							exit(1);
						}
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_UNIT:
						unit = get_unit_struct((*argv)[1]);
						if (unit == NULL) {
							fprintf(stderr, "ERROR:  unit \"%s\" is unknown to pcb (option --%s)\n", (*argv)[1], a->name);
							exit(1);
						}
						a->default_val.lng = unit->index;
						a->default_val.str = unit->suffix;
						(*argc)--;
						(*argv)++;
						break;
					default:
						break;
					}
					(*argc)--;
					(*argv)++;
					ha = 0;
					goto got_match;
				}
		if (bool_val == 1 && strncmp((*argv)[0], "--no-", 5) == 0) {
			bool_val = 0;
			arg_ofs = 5;
			goto try_no_arg;
		}
		fprintf(stderr, "unrecognized option: %s\n", (*argv)[0]);
		return -1;
	got_match:;
	}
	else {
		/* Any argument without the "--" is considered a filename. Take only
		   one filename for now */
		if (filename == NULL) {
			filename = (*argv)[0];
			(*argc)--;
			(*argv)++;
		}
		else {
			rnd_message(RND_MSG_ERROR, "Multiple filenames not supported. First filename was: %s; offending second filename: %s\n", filename, (*argv)[0]);
			return -1;
		}
	}

	/* restore filename to the first argument */
	if (filename != NULL) {
		(*argv)[0] = filename;
		(*argc) = 1;
	}
	return 0;
}

void rnd_hid_usage_option(const char *name, const char *help)
{
	fprintf(stderr, "--%-20s %s\n", name, help);
}

void rnd_hid_usage(rnd_export_opt_t *a, int numa)
{
	for (; numa > 0; numa--,a++) {
		const char *help;
		if (a->help_text == NULL) help = "";
		else help = a->help_text;
		if (RND_HATT_IS_COMPOSITE(a->type)) {
			rnd_hid_export_opt_func_t fnc = a->default_val.func;
			if (fnc != NULL)
				fnc(RND_HIDEOF_USAGE, stderr, a);
		}
		else
			rnd_hid_usage_option(a->name, help);
	}
}

