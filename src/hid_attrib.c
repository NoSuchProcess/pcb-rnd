/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2018 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include "hid_attrib.h"
#include "misc_util.h"
#include "pcb-printf.h"

pcb_hid_attr_node_t *hid_attr_nodes = 0;

typedef struct {
	pcb_hatt_compflags_t flag;
	const char *name;
} comflag_name_t;

static comflag_name_t compflag_names[] = {
	{PCB_HATF_FRAME,         "frame"},
	{PCB_HATF_SCROLL,        "scroll"},
	{PCB_HATF_HIDE_TABLAB,   "hide_tablab"},
	{PCB_HATF_LEFT_TAB,      "left_tab"},
	{PCB_HATF_TREE_COL,      "tree_col"},
	{PCB_HATF_EXPFILL,       "expfill"},
	{0, NULL}
};

const char *pcb_hid_compflag_bit2name(pcb_hatt_compflags_t bit)
{
	comflag_name_t *n;
	for(n = compflag_names; n->flag != 0; n++)
		if (n->flag == bit)
			return n->name;
	return NULL;
}

pcb_hatt_compflags_t pcb_hid_compflag_name2bit(const char *name)
{
	comflag_name_t *n;
	for(n = compflag_names; n->flag != 0; n++)
		if (strcmp(n->name, name) == 0)
			return n->flag;
	return 0;
}

void pcb_hid_register_attributes(pcb_hid_attribute_t * a, int n, const char *cookie, int copy)
{
	pcb_hid_attr_node_t *ha;

	/* printf("%d attributes registered\n", n); */
	ha = malloc(sizeof(pcb_hid_attr_node_t));
	ha->next = hid_attr_nodes;
	hid_attr_nodes = ha;
	ha->attributes = a;
	ha->n = n;
	ha->cookie = cookie;
}

void pcb_hid_attributes_uninit(void)
{
	pcb_hid_attr_node_t *ha, *next;
	for (ha = hid_attr_nodes; ha; ha = next) {
		next = ha->next;
		if (ha->cookie != NULL)
			fprintf(stderr, "WARNING: attribute %s by %s is not uninited, check your plugins' uninit!\n", ha->attributes->name, ha->cookie);
		free(ha);
	}
	hid_attr_nodes = NULL;
}

void pcb_hid_remove_attributes_by_cookie(const char *cookie)
{
	pcb_hid_attr_node_t *ha, *next, *prev = NULL;
	for (ha = hid_attr_nodes; ha; ha = next) {
		next = ha->next;
		if (ha->cookie == cookie) {
			if (prev == NULL)
				hid_attr_nodes = next;
			else
				prev->next = next;
			free(ha);
		}
		else
			prev = ha;
	}
}

int pcb_hid_parse_command_line(int *argc, char ***argv)
{
	pcb_hid_attr_node_t *ha;
	int i, e, ok;
	char *filename = NULL;

	for (ha = hid_attr_nodes; ha; ha = ha->next)
		for (i = 0; i < ha->n; i++) {
			pcb_hid_attribute_t *a = ha->attributes + i;
			switch (a->type) {
			case PCB_HATT_LABEL:
				break;
			case PCB_HATT_INTEGER:
				if (a->value)
					*(int *) a->value = a->default_val.int_value;
				break;
			case PCB_HATT_COORD:
				if (a->value)
					*(pcb_coord_t *) a->value = a->default_val.coord_value;
				break;
			case PCB_HATT_BOOL:
				if (a->value)
					*(char *) a->value = a->default_val.int_value;
				break;
			case PCB_HATT_REAL:
				if (a->value)
					*(double *) a->value = a->default_val.real_value;
				break;
			case PCB_HATT_STRING:
				if (a->value)
					*(const char **) a->value = a->default_val.str_value;
				break;
			case PCB_HATT_ENUM:
				if (a->value)
					*(int *) a->value = a->default_val.int_value;
				break;
			case PCB_HATT_UNIT:
				if (a->value)
					*(int *) a->value = a->default_val.int_value;
				break;
			default:
				pcb_message(PCB_MSG_ERROR, "Invalid attribute type %d for attribute %s\n", a->type, a->name);
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
		for (ha = hid_attr_nodes; ha; ha = ha->next)
			for (i = 0; i < ha->n; i++)
				if (strcmp((*argv)[0] + arg_ofs, ha->attributes[i].name) == 0) {
					pcb_hid_attribute_t *a = ha->attributes + i;
					char *ep;
					const pcb_unit_t *unit;
					switch (ha->attributes[i].type) {
					case PCB_HATT_LABEL:
						break;
					case PCB_HATT_INTEGER:
						if (a->value)
							*(int *) a->value = strtol((*argv)[1], 0, 0);
						else
							a->default_val.int_value = strtol((*argv)[1], 0, 0);
						(*argc)--;
						(*argv)++;
						break;
					case PCB_HATT_COORD:
						if (a->value)
							*(pcb_coord_t *) a->value = pcb_get_value((*argv)[1], NULL, NULL, NULL);
						else
							a->default_val.coord_value = pcb_get_value((*argv)[1], NULL, NULL, NULL);
						(*argc)--;
						(*argv)++;
						break;
					case PCB_HATT_REAL:
						if (a->value)
							*(double *) a->value = strtod((*argv)[1], 0);
						else
							a->default_val.real_value = strtod((*argv)[1], 0);
						(*argc)--;
						(*argv)++;
						break;
					case PCB_HATT_STRING:
						if (a->value)
							*(char **) a->value = (*argv)[1];
						else
							a->default_val.str_value = (*argv)[1];
						(*argc)--;
						(*argv)++;
						break;
					case PCB_HATT_BOOL:
						if (a->value)
							*(char *) a->value = bool_val;
						else
							a->default_val.int_value = bool_val;
						break;
					case PCB_HATT_ENUM:
						ep = (*argv)[1];
						ok = 0;
						for (e = 0; a->enumerations[e]; e++)
							if (strcmp(a->enumerations[e], ep) == 0) {
								ok = 1;
								a->default_val.int_value = e;
								a->default_val.str_value = ep;
								break;
							}
						if (!ok) {
							fprintf(stderr, "ERROR:  \"%s\" is an unknown value for the --%s option\n", (*argv)[1], a->name);
							exit(1);
						}
						(*argc)--;
						(*argv)++;
						break;
					case PCB_HATT_PATH:
						abort();
						a->default_val.str_value = (*argv)[1];
						(*argc)--;
						(*argv)++;
						break;
					case PCB_HATT_UNIT:
						unit = get_unit_struct((*argv)[1]);
						if (unit == NULL) {
							fprintf(stderr, "ERROR:  unit \"%s\" is unknown to pcb (option --%s)\n", (*argv)[1], a->name);
							exit(1);
						}
						a->default_val.int_value = unit->index;
						a->default_val.str_value = unit->suffix;
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
			pcb_message(PCB_MSG_ERROR, "Multiple filenames not supported. First filename was: %s; offending second filename: %s\n", filename, (*argv)[0]);
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

void pcb_hid_usage_option(const char *name, const char *help)
{
	fprintf(stderr, "%-20s %s\n", name, help);
}

void pcb_hid_usage(pcb_hid_attribute_t * a, int numa)
{
	for (; numa > 0; numa--,a++) {
		const char *help;
		if (a->help_text == NULL) help = "";
		else if (a->help_text == ATTR_UNDOCUMENTED) help = "<undocumented>";
		else help = a->help_text;
		pcb_hid_usage_option(a->name, help);
	}
}

int pcb_hid_attrdlg_num_children(pcb_hid_attribute_t *attrs, int start_from, int n_attrs)
{
	int n, level = 1, cnt = 0;

	for(n = start_from; n < n_attrs; n++) {
		if ((level == 1) && (attrs[n].type != PCB_HATT_END))
			cnt++;
		switch(attrs[n].type) {
			case PCB_HATT_END:
				level--;
				if (level == 0)
					return cnt;
				break;
			case PCB_HATT_BEGIN_TABLE:
			case PCB_HATT_BEGIN_HBOX:
			case PCB_HATT_BEGIN_VBOX:
				level++;
				break;
			default:
				break;
		}
	}
	return cnt;
}

int pcb_attribute_dialog_(const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, int *already_free, int defx, int defy, void **hid_ctx_out)
{
	int rv;
	void *hid_ctx;

	if ((pcb_gui == NULL) || (pcb_gui->attr_dlg_new == NULL))
		return -1;

	hid_ctx = pcb_gui->attr_dlg_new(id, attrs, n_attrs, results, title, caller_data, pcb_true, NULL, 0, 0);
	if (hid_ctx_out != NULL)
		*hid_ctx_out = hid_ctx;
	rv = pcb_gui->attr_dlg_run(hid_ctx);
	if ((already_free == NULL) || (*already_free == 0))
		pcb_gui->attr_dlg_free(hid_ctx);

	return rv ? 0 : 1;
}

int pcb_attribute_dialog(const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data)
{
	return pcb_attribute_dialog_(id, attrs, n_attrs, results, title, caller_data, NULL, 0, 0, NULL);
}
