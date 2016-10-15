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

/* advanced search dialog - expressioin wizard tables; intended to be
   included from ghid-search.c only */

static struct {
	GtkWidget *entry_left;
	GtkWidget *tr_left, *tr_op;
	GtkWidget *right_str, *right_coord, *tr_right, *right_int;
	GtkTreeStore *md_left;
	GtkListStore *md_objtype;
	GtkAdjustment *right_adj;
} expr_wizard_dlg;

static GType model_op[2] = { G_TYPE_STRING, G_TYPE_POINTER };

typedef struct expr_wizard_op_s expr_wizard_op_t;
struct expr_wizard_op_s {
	const char **ops;
	GtkListStore *model;
};

typedef enum {
	RIGHT_STR,
	RIGHT_INT,
	RIGHT_COORD,
	RIGHT_OBJTYPE
} right_type;

typedef struct expr_wizard_s expr_wizard_t;
struct expr_wizard_s {
	const char *left_var;
	const char *left_desc;
	const expr_wizard_op_t *ops;
	right_type rtype;
};

enum {
	OPS_ANY,
	OPS_EQ,
	OPS_STR
};

const char *ops_any[] = {"==", "!=", ">=", "<=", ">", "<", NULL};
const char *ops_eq[]  = {"==", "!=", NULL};
const char *ops_str[] = {"==", "!=", "~", NULL};

expr_wizard_op_t op_tab[] = {
	{ops_any, NULL},
	{ops_eq, NULL},
	{ops_str, NULL},
	{NULL, NULL}
};

static const expr_wizard_t expr_tab[] = {
	{"@.id",              "object ID",        &op_tab[OPS_ANY], RIGHT_INT},
	{"@.type",            "object type",      &op_tab[OPS_EQ],  RIGHT_OBJTYPE},

	{NULL,                "bounding box",     NULL,             0},
	{"@.bbox.x1",         "X1",               &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.bbox.y1",         "Y1",               &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.bbox.x2",         "X2",               &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.bbox.y2",         "Y2",               &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.bbox.w",          "width",            &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.bbox.h",          "height",           &op_tab[OPS_ANY], RIGHT_COORD},


	{NULL,                "trace",            NULL,             0},
	{"@.thickness",       "thickness",        &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.clearance",       "clearance",        &op_tab[OPS_ANY], RIGHT_COORD},

	{NULL,                "line",             NULL,             0},
	{"@.x1",              "X1",               &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.y1",              "Y1",               &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.x2",              "X2",               &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.y2",              "Y2",               &op_tab[OPS_ANY], RIGHT_COORD},

	{NULL,                "arc",              NULL,             0},
	{"@.x",               "center X",         &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.y",               "center Y",         &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.angle.start",     "start angle",      &op_tab[OPS_ANY], RIGHT_INT},
	{"@.angle.delta",     "delta angle",      &op_tab[OPS_ANY], RIGHT_INT},

	{NULL,                "text",             NULL,             0},
	{"@.x",               "X",                &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.y",               "Y",                &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.scale",           "scale",            &op_tab[OPS_ANY], RIGHT_INT},
	{"@.string",          "string",           &op_tab[OPS_ANY], RIGHT_STR},
#warning TODO
/*	{"@.rotation",           "rotation",            &op_tab[OPS_ANY], RIGHT_INT},*/

	{NULL,                "polygon",          NULL,             0},
	{"@.points",          "points",           &op_tab[OPS_ANY], RIGHT_INT},

	{NULL,                "pin or via",       NULL,             0},
	{"@.x",               "X",                &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.y",               "Y",                &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.hole",            "drilling hole dia",&op_tab[OPS_ANY], RIGHT_COORD},
	{"@.mask",            "mask",             &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.name",            "name",             &op_tab[OPS_STR], RIGHT_STR},
	{"@.number",          "number",           &op_tab[OPS_STR], RIGHT_STR},

	{NULL,                "element",          NULL,             0},
	{"@.x",               "X",                &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.y",               "Y",                &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.name",            "name",             &op_tab[OPS_STR], RIGHT_STR},
	{"@.refdes",          "refdes",           &op_tab[OPS_STR], RIGHT_STR},
	{"@.description",     "description",      &op_tab[OPS_STR], RIGHT_STR},
	{"@.value",           "value",            &op_tab[OPS_STR], RIGHT_STR},

	{NULL,                "host layer's",     NULL,             0},
	{"@.layer.name",      "name",             &op_tab[OPS_STR], RIGHT_STR},
	{"@.layer.visible",   "visible",          &op_tab[OPS_EQ],  RIGHT_INT},
#warning TODO
/*	{"@.layer.position",  "stack position",  &op_tab[OPS_EQ],  RIGHT_INT},*/
/*	{"@.layer.type",      "type",            &op_tab[OPS_EQ],  RIGHT_INT},*/

	{NULL,                "host element's",   NULL,             0},
	{"@.element.x",       "X",                &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.element.y",       "Y",                &op_tab[OPS_ANY], RIGHT_COORD},
	{"@.element.refdes",  "refdes",           &op_tab[OPS_STR], RIGHT_STR},
	{"@.element.name",    "name",             &op_tab[OPS_STR], RIGHT_STR},
	{"@.element.description","description",   &op_tab[OPS_STR], RIGHT_STR},
	{"@.element.value",   "value",            &op_tab[OPS_STR], RIGHT_STR},

	{NULL, NULL, NULL, 0}
};

const char *right_objtype[] = {
	"POINT", "LINE", "TEXT", "POLYGON", "ARC", "RAT", "PAD", "PIN", "VIA",
	"ELEMENT", "NET", "LAYER", "ELINE", "EARC", "ETEXT",
	NULL
};
