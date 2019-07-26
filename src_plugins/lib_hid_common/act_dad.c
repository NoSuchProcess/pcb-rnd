/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <genht/htsp.h>
#include <genht/hash.h>
#include <genvector/vts0.h>
#include <ctype.h>
#include "actions.h"
#include "compat_misc.h"
#include "hid_dad.h"
#include "hid_dad_tree.h"
#include "error.h"

#include "act_dad.h"

#define MAX_ENUM 128

typedef union tmp_u tmp_t;

typedef struct tmp_str_s {
	tmp_t *next;
	char str[1];
} tmp_str_t;

typedef struct tmp_strlist_s {
	tmp_t *next;
	char *values[MAX_ENUM+1];
} tmp_strlist_t;

union tmp_u {
	struct {
		tmp_t *next;
	} list;
	tmp_str_t str;
	tmp_strlist_t strlist;
};

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	char *name;
	const char *row_domain;
	int level;
	tmp_t *tmp_str_head;
	vts0_t change_cb;
	unsigned running:1;
} dad_t;

htsp_t dads;

static int dad_new(const char *name)
{
	dad_t *dad;

	if (htsp_get(&dads, name) != NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't create named DAD dialog %s: already exists\n", name);
		return -1;
	}

	dad = calloc(sizeof(dad_t), 1);
	dad->name = pcb_strdup(name);
	dad->row_domain = dad->name;
	htsp_set(&dads, dad->name, dad);
	return 0;
}

static void dad_destroy(dad_t *dad)
{
	tmp_t *t, *tnext;
	for(t = dad->tmp_str_head; t != NULL; t = tnext) {
		tnext = t->list.next;
		free(t);
	}
	htsp_pop(&dads, dad->name);
	free(dad->name);
	free(dad);
}

static void dad_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	dad_t *dad = caller_data;
	PCB_DAD_FREE(dad->dlg);
	dad_destroy(dad);
}

static void dad_change_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	dad_t *dad = caller_data;
	int idx = attr - dad->dlg;
	char **act = vts0_get(&dad->change_cb, idx, 0);
	if ((act != NULL) && (*act != NULL))
		pcb_parse_command(*act, 1);
}

static void dad_row_free_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	dad_t *dad = tree->user_ctx;
	fgw_arg_t res;
	res.type = FGW_PTR | FGW_VOID;
	res.val.ptr_void = row;
	fgw_ptr_unreg(&pcb_fgw, &res, dad->row_domain);
}

static char *tmp_str_dup(dad_t *dad, const char *txt)
{
	size_t len = strlen(txt);
	tmp_str_t *tmp = malloc(sizeof(tmp_str_t) + len);
	tmp->next = dad->tmp_str_head;
	dad->tmp_str_head = (tmp_t *)tmp;
	memcpy(tmp->str, txt, len+1);
	return tmp->str;
}

static char **tmp_new_strlist(dad_t *dad)
{
	tmp_strlist_t *tmp = malloc(sizeof(tmp_strlist_t));
	tmp->next = dad->tmp_str_head;
	dad->tmp_str_head = (tmp_t *)tmp;
	return tmp->values;
}

static int split_tablist(dad_t *dad, char **values, const char *txt, const char *cmd)
{
	char *next, *s = tmp_str_dup(dad, txt);
	int len = 0;

	while(isspace(*s)) s++;

	for(len = 0; s != NULL; s = next) {
		if (len >= MAX_ENUM) {
			pcb_message(PCB_MSG_ERROR, "Too many DAD %s values\n", cmd);
			return -1;
		}
		next = strchr(s, '\t');
		if (next != NULL) {
			*next = '\0';
			next++;
			while(isspace(*next)) next++;
		}
		values[len] = s;
		len++;
	}
	values[len] = NULL;
	return 0;
}

const char pcb_acts_dad[] =
	"dad(dlgname, new) - create new dialog\n"
	"dad(dlgname, label, text) - append a label widget\n"
	"dad(dlgname, button, text) - append a button widget\n"
	"dad(dlgname, button_closes, label, retval, ...) - standard close buttons\n"
	"dad(dlgname, enum, choices) - append an enum (combo box) widget; choices is a tab separated list\n"
	"dad(dlgname, bool, [label]) - append an checkbox widget (default off)\n"
	"dad(dlgname, integer|real|coord, min, max, [label]) - append an input field\n"
	"dad(dlgname, string) - append a single line text input field\n"
	"dad(dlgname, progress) - append a progress bar (set to 0)\n"
	"dad(dlgname, tree, cols, istree, [header]) - append tree-table widget; header is like enum values\n"
	"dad(dlgname, tree_append, row, cells) - append after row (0 means last item of the root); cells is like enum values; returns a row pointer\n"
	"dad(dlgname, tree_append_under, row, cells) - append at the end of the list under row (0 means last item of the root); cells is like enum values; returns a row pointer\n"
	"dad(dlgname, tree_insert, row, cells) - insert before row (0 means first item of the root); cells is like enum values; returns a row pointer\n"
	"dad(dlgname, begin_hbox) - begin horizontal box\n"
	"dad(dlgname, begin_vbox) - begin vertical box\n"
	"dad(dlgname, begin_hpane) - begin horizontal paned box\n"
	"dad(dlgname, begin_vpane) - begin vertical paned box\n"
	"dad(dlgname, begin_table, cols) - begin table layout box\n"
	"dad(dlgname, begin_tabbed, tabnames) - begin a view with tabs; tabnames are like choices in an enum; must have as many children widgets as many names it has\n"
	"dad(dlgname, end) - end the last begin\n"
	"dad(dlgname, flags, flg1, flg2, ...) - change the flags of the last created widget\n"
	"dad(dlgname, onchange, action) - set the action to be called on widget change\n"
	"dad(dlgname, run, title) - present dlgname as a non-modal dialog\n"
	"dad(dlgname, run_modal, title) - present dlgname as a modal dialog\n"
	"dad(dlgname, exists) - returns wheter the named dialog exists (0 or 1)\n"
	"dad(dlgname, set, widgetID, val) - changes the value of a widget in a running dialog \n"
	"dad(dlgname, get, widgetID, [unit]) - return the current value of a widget\n"
	;
const char pcb_acth_dad[] = "Manipulate Dynamic Attribute Dialogs";
fgw_error_t pcb_act_dad(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *cmd, *dlgname, *txt;
	dad_t *dad;
	int rv = 0;

	PCB_ACT_CONVARG(1, FGW_STR, dad, dlgname = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, dad, cmd = argv[2].val.str);

	if (pcb_strcasecmp(cmd, "new") == 0) {
		PCB_ACT_IRES(dad_new(dlgname));
		return 0;
	}

	dad = htsp_get(&dads, dlgname);
	if (pcb_strcasecmp(cmd, "exists") == 0) {
		PCB_ACT_IRES(dad != NULL);
		return 0;
	}

	if (dad == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't find named DAD dialog %s\n", dlgname);
		PCB_ACT_IRES(-1);
		return 0;
	}


	if (pcb_strcasecmp(cmd, "label") == 0) {
		if (dad->running) goto cant_chg;
		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_DAD_LABEL(dad->dlg, txt);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "button") == 0) {
		if (dad->running) goto cant_chg;
		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_DAD_BUTTON(dad->dlg, tmp_str_dup(dad, txt));
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "button_closes") == 0) {
		int n, ret;

		if (dad->running) goto cant_chg;

		PCB_DAD_BEGIN_HBOX(dad->dlg);
		PCB_DAD_BEGIN_HBOX(dad->dlg);
		PCB_DAD_COMPFLAG(dad->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_END(dad->dlg);
		for(n = 3; n < argc; n+=2) {
			PCB_ACT_CONVARG(n+0, FGW_STR, dad, txt = argv[n+0].val.str);
			PCB_ACT_CONVARG(n+1, FGW_INT, dad, ret = argv[n+1].val.nat_int);
			
			PCB_DAD_BUTTON_CLOSE(dad->dlg, tmp_str_dup(dad, txt), ret);
				rv = PCB_DAD_CURRENT(dad->dlg);
		}
		PCB_DAD_END(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "bool") == 0) {
		if (dad->running) goto cant_chg;
		txt = "";
		PCB_ACT_MAY_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_DAD_BOOL(dad->dlg, txt);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "integer") == 0) {
		long vmin, vmax;
		if (dad->running) goto cant_chg;
		txt = "";
		PCB_ACT_CONVARG(3, FGW_LONG, dad, vmin = argv[3].val.nat_long);
		PCB_ACT_CONVARG(4, FGW_LONG, dad, vmax = argv[4].val.nat_long);
		PCB_ACT_MAY_CONVARG(5, FGW_STR, dad, txt = argv[5].val.str);
		PCB_DAD_INTEGER(dad->dlg, txt);
		PCB_DAD_MINMAX(dad->dlg, vmin, vmax);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "real") == 0) {
		double vmin, vmax;
		if (dad->running) goto cant_chg;
		txt = "";
		PCB_ACT_CONVARG(3, FGW_DOUBLE, dad, vmin = argv[3].val.nat_double);
		PCB_ACT_CONVARG(4, FGW_DOUBLE, dad, vmax = argv[4].val.nat_double);
		PCB_ACT_MAY_CONVARG(5, FGW_STR, dad, txt = argv[5].val.str);
		PCB_DAD_REAL(dad->dlg, txt);
		PCB_DAD_MINMAX(dad->dlg, vmin, vmax);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "coord") == 0) {
		pcb_coord_t vmin, vmax;
		if (dad->running) goto cant_chg;
		txt = "";
		PCB_ACT_CONVARG(3, FGW_COORD_, dad, vmin = fgw_coord(&argv[3]));
		PCB_ACT_CONVARG(4, FGW_COORD_, dad, vmax = fgw_coord(&argv[4]));
		PCB_ACT_MAY_CONVARG(5, FGW_STR, dad, txt = argv[5].val.str);
		PCB_DAD_COORD(dad->dlg, txt);
		PCB_DAD_MINMAX(dad->dlg, vmin, vmax);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "string") == 0) {
		if (dad->running) goto cant_chg;
		PCB_DAD_STRING(dad->dlg);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "progress") == 0) {
		if (dad->running) goto cant_chg;
		PCB_DAD_PROGRESS(dad->dlg);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if ((pcb_strcasecmp(cmd, "enum") == 0) || (pcb_strcasecmp(cmd, "begin_tabbed") == 0)) {
		char **values = tmp_new_strlist(dad);

		if (dad->running) goto cant_chg;

		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);

		if (split_tablist(dad, values, txt, cmd) == 0) {
			if (*cmd == 'b') {
				PCB_DAD_BEGIN_TABBED(dad->dlg, (const char **)values);
				dad->level++;
			}
			else
				PCB_DAD_ENUM(dad->dlg, (const char **)values);
			rv = PCB_DAD_CURRENT(dad->dlg);
		}
		else
			rv = -1;
	}
	else if (pcb_strcasecmp(cmd, "tree") == 0) {
		int cols, istree;
		char **values = tmp_new_strlist(dad);

		if (dad->running) goto cant_chg;

		txt = NULL;
		PCB_ACT_CONVARG(3, FGW_INT, dad, cols = argv[3].val.nat_int);
		PCB_ACT_CONVARG(4, FGW_INT, dad, istree = argv[4].val.nat_int);
		PCB_ACT_MAY_CONVARG(5, FGW_STR, dad, txt = argv[5].val.str);

		if ((txt == NULL) || (split_tablist(dad, values, txt, cmd) == 0)) {
			PCB_DAD_TREE(dad->dlg, cols, istree, (const char **)values);
			PCB_DAD_TREE_SET_CB(dad->dlg, free_cb, dad_row_free_cb);
			PCB_DAD_TREE_SET_CB(dad->dlg, ctx, dad);
			rv = PCB_DAD_CURRENT(dad->dlg);
		}
		else
			rv = -1;
	}
	else if ((pcb_strcasecmp(cmd, "tree_append") == 0) || (pcb_strcasecmp(cmd, "tree_append_under") == 0) || (pcb_strcasecmp(cmd, "tree_insert") == 0)) {
		void *row, *nrow = NULL;
		char **values = tmp_new_strlist(dad);

		if (dad->running) goto cant_chg;

		PCB_ACT_CONVARG(3, FGW_PTR, dad, row = argv[3].val.ptr_void);
		PCB_ACT_CONVARG(4, FGW_STR, dad, txt = argv[4].val.str);

		if (row != NULL) {
			if (!fgw_ptr_in_domain(&pcb_fgw, &argv[3], dad->row_domain)) {
				pcb_message(PCB_MSG_ERROR, "Invalid DAD row pointer\n");
				PCB_ACT_IRES(-1);
				return 0;
			}
		}

		if ((txt == NULL) || (split_tablist(dad, values, txt, cmd) == 0)) {
			if (cmd[5] == 'i')
				nrow = PCB_DAD_TREE_INSERT(dad->dlg, row, values);
			else if (cmd[11] == '_')
				nrow = PCB_DAD_TREE_APPEND_UNDER(dad->dlg, row, values);
			else
				nrow = PCB_DAD_TREE_APPEND(dad->dlg, row, values);
		}
		else
			nrow = NULL;
		fgw_ptr_reg(&pcb_fgw, res, dad->row_domain, FGW_PTR, nrow);
		return 0;
	}
	else if (pcb_strcasecmp(cmd, "begin_hbox") == 0) {
		if (dad->running) goto cant_chg;
		PCB_DAD_BEGIN_HBOX(dad->dlg);
		dad->level++;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "begin_vbox") == 0) {
		if (dad->running) goto cant_chg;
		PCB_DAD_BEGIN_VBOX(dad->dlg);
		dad->level++;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "begin_hpane") == 0) {
		if (dad->running) goto cant_chg;
		PCB_DAD_BEGIN_HPANE(dad->dlg);
		dad->level++;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "begin_vpane") == 0) {
		if (dad->running) goto cant_chg;
		PCB_DAD_BEGIN_VPANE(dad->dlg);
		dad->level++;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "begin_table") == 0) {
		int cols;

		if (dad->running) goto cant_chg;

		PCB_ACT_CONVARG(3, FGW_INT, dad, cols = argv[3].val.nat_int);
		PCB_DAD_BEGIN_TABLE(dad->dlg, cols);
		dad->level++;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "end") == 0) {
		if (dad->running) goto cant_chg;

		PCB_DAD_END(dad->dlg);
		dad->level--;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "flags") == 0) {
		int n;
		pcb_hatt_compflags_t tmp, flg = 0;

		if (dad->running) goto cant_chg;

		for(n = 3; n < argc; n++) {
			PCB_ACT_CONVARG(n, FGW_STR, dad, txt = argv[n].val.str);
			if ((*txt == '\0') || (*txt == '0'))
				continue;
			tmp = pcb_hid_compflag_name2bit(txt);
			if (tmp == 0)
				pcb_message(PCB_MSG_ERROR, "Invalid DAD flag: %s (ignored)\n", txt);
			flg |= tmp;
		}
		PCB_DAD_COMPFLAG(dad->dlg, flg);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "onchange") == 0) {
		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_DAD_CHANGE_CB(dad->dlg, dad_change_cb);
		vts0_set(&dad->change_cb, PCB_DAD_CURRENT(dad->dlg), tmp_str_dup(dad, txt));
		rv = 0;
	}
	else if (pcb_strcasecmp(cmd, "set") == 0) {
		int wid, i;
		double d;
		pcb_coord_t c;

		PCB_ACT_CONVARG(3, FGW_INT, dad, wid = argv[3].val.nat_int);
		if ((wid < 0) || (wid >= dad->dlg_len)) {
			pcb_message(PCB_MSG_ERROR, "Invalid widget ID %d (set ignored)\n", wid);
			PCB_ACT_IRES(-1);
			return 0;
		}

		switch(dad->dlg[wid].type) {
			case PCB_HATT_COORD:
				PCB_ACT_CONVARG(4, FGW_COORD, dad, c = fgw_coord(&argv[4]));
				PCB_DAD_SET_VALUE(dad->dlg_hid_ctx, wid, coord_value, c);
				break;
			case PCB_HATT_REAL:
			case PCB_HATT_PROGRESS:
				PCB_ACT_CONVARG(4, FGW_DOUBLE, dad, d = argv[4].val.nat_double);
				PCB_DAD_SET_VALUE(dad->dlg_hid_ctx, wid, real_value, d);
				break;
			case PCB_HATT_INTEGER:
				PCB_ACT_CONVARG(4, FGW_INT, dad, i = argv[4].val.nat_int);
				PCB_DAD_SET_VALUE(dad->dlg_hid_ctx, wid, lng, i);
				break;
			case PCB_HATT_STRING:
			case PCB_HATT_LABEL:
			case PCB_HATT_BUTTON:
				PCB_ACT_CONVARG(4, FGW_STR, dad, txt = argv[4].val.str);
				PCB_DAD_SET_VALUE(dad->dlg_hid_ctx, wid, str_value, txt);
				break;
			default:
				pcb_message(PCB_MSG_ERROR, "Invalid widget type %d - can not change value (set ignored)\n", wid);
				PCB_ACT_IRES(-1);
				return 0;
		}
		rv = 0;
	}
	else if (pcb_strcasecmp(cmd, "get") == 0) {
		int wid;

		PCB_ACT_CONVARG(3, FGW_INT, dad, wid = argv[3].val.nat_int);
		if ((wid < 0) || (wid >= dad->dlg_len)) {
			pcb_message(PCB_MSG_ERROR, "Invalid widget ID %d (get ignored)\n", wid);
			return FGW_ERR_NOT_FOUND;
		}

		switch(dad->dlg[wid].type) {
			case PCB_HATT_COORD:
				txt = NULL;
				PCB_ACT_MAY_CONVARG(4, FGW_STR, dad, txt = argv[4].val.str);
				if (txt != NULL) {
					const pcb_unit_t *u = get_unit_struct(txt);
					if (u == NULL) {
						pcb_message(PCB_MSG_ERROR, "Invalid unit %s (get ignored)\n", txt);
						return FGW_ERR_NOT_FOUND;
					}
					res->type = FGW_DOUBLE;
					res->val.nat_double = pcb_coord_to_unit(u, dad->dlg[wid].val.coord_value);
				}
				else {
					res->type = FGW_COORD;
					fgw_coord(res) = dad->dlg[wid].val.coord_value;
				}
				break;
			case PCB_HATT_INTEGER:
			case PCB_HATT_ENUM:
				res->type = FGW_INT;
				res->val.nat_int = dad->dlg[wid].val.lng;
				break;
			case PCB_HATT_STRING:
			case PCB_HATT_LABEL:
			case PCB_HATT_BUTTON:
				res->type = FGW_STR;
				res->val.str = (char *)dad->dlg[wid].val.str_value;
				break;
			default:
				pcb_message(PCB_MSG_ERROR, "Invalid widget type %d - can not retrieve value (get ignored)\n", wid);
				return FGW_ERR_NOT_FOUND;
		}
		return 0;
	}	else if ((pcb_strcasecmp(cmd, "run") == 0) || (pcb_strcasecmp(cmd, "run_modal") == 0)) {
		if (dad->running) goto cant_chg;

		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);

		if (dad->level != 0) {
			pcb_message(PCB_MSG_ERROR, "Invalid DAD dialog structure: %d levels not closed (missing 'end' calls)\n", dad->level);
			rv = -1;
		}
		else {
			PCB_DAD_NEW(dlgname, dad->dlg, txt, dad, (cmd[3] == '_'), dad_close_cb);
			rv = PCB_DAD_CURRENT(dad->dlg);
		}
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Invalid DAD dialog command: '%s'\n", cmd);
		rv = -1;
	}

	PCB_ACT_IRES(rv);
	return 0;

	cant_chg:;
	pcb_message(PCB_MSG_ERROR, "Can't find named DAD dialog %s\n", dlgname);
	PCB_ACT_IRES(-1);
	return 0;
}

void pcb_act_dad_init(void)
{
	htsp_init(&dads, strhash, strkeyeq);
}

void pcb_act_dad_uninit(void)
{
	htsp_entry_t *e;
	for(e = htsp_first(&dads); e != NULL; e = htsp_next(&dads, e))
		dad_destroy(e->value);
	htsp_uninit(&dads);
}
