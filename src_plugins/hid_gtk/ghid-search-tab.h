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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* advanced search dialog - expressioin wizard tables */

static struct {
	GtkWidget *entry_left, *entry_right;
	GtkWidget *tr_left, *tr_op, *tr_right;
	GtkListStore *md_left;
} expr_wizard_dlg;

static GType model_op[2] = { G_TYPE_STRING, G_TYPE_POINTER };

typedef struct expr_wizard_op_s expr_wizard_op_t;
struct expr_wizard_op_s {
	const char **ops;
	GtkListStore *model;
};

typedef struct expr_wizard_s expr_wizard_t;
struct expr_wizard_s {
	const char *left_var;
	const char *left_desc;
	const expr_wizard_op_t *ops;
};

enum {
	OPS_ANY,
	OPS_EQ,
};

const char *ops_any[] = {"==", "!=", ">=", "<=", ">", "<", NULL};
const char *ops_eq[]  = {"==", "!=", NULL};

expr_wizard_op_t op_tab[] = {
	{ops_any, NULL},
	{ops_eq, NULL},
	{NULL, NULL}
};

static const expr_wizard_t expr_tab[] = {
	{"@.id",        "object ID",             &op_tab[OPS_ANY]},
	{"@.type",      "object type",           &op_tab[OPS_EQ]},
	{"@.bbox.x1",   "bounding box X1",       &op_tab[OPS_ANY]},
	{"@.bbox.y1",   "bounding box Y1",       &op_tab[OPS_ANY]},
	{"@.bbox.x2",   "bounding box X2",       &op_tab[OPS_ANY]},
	{"@.bbox.y2",   "bounding box Y2",       &op_tab[OPS_ANY]},
	{"@.bbox.w",    "bounding box width",    &op_tab[OPS_ANY]},
	{"@.bbox.h",    "bounding box height",   &op_tab[OPS_ANY]},
	{NULL, NULL}
};
