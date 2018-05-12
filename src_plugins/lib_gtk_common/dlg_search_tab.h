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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* advanced search dialog - expressioin wizard tables; intended to be
   included from ghid-search.c only */

static struct {
	GtkWidget *entry_left;
	GtkWidget *tr_left, *tr_op;
	GtkWidget *right_str, *right_coord, *tr_right, *right_int, *right_double;
	GtkTreeStore *md_left;
	GtkAdjustment *right_adj, *right_adj2;
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
	RIGHT_DOUBLE,
	RIGHT_COORD,
	RIGHT_CONST
} right_type;

typedef struct expr_wizard_s expr_wizard_t;
struct expr_wizard_s {
	const char *left_var;
	const char *left_desc;
	const expr_wizard_op_t *ops;
	right_type rtype;
	const expr_wizard_op_t *right_const;
};

enum {
	OPS_ANY,
	OPS_EQ,
	OPS_STR
};

static const char *ops_any[] = {"==", "!=", ">=", "<=", ">", "<", NULL};
static const char *ops_eq[]  = {"==", "!=", NULL};
static const char *ops_str[] = {"==", "!=", "~", NULL};

static expr_wizard_op_t op_tab[] = {
	{ops_any, NULL},
	{ops_eq, NULL},
	{ops_str, NULL},
	{NULL, NULL}
};

static const char *right_const_objtype[] = { "POINT", "LINE", "TEXT", "POLYGON", "ARC", "RAT", "NET", "LAYER", NULL };
static const char *right_const_yesno[] = {"YES", "NO", NULL};
static const char *right_const_layerpos[] = {"TOP", "BOTTOM", "INTERNAL", NULL};
static const char *right_const_layertype[] = {"COPPER", "SILK", "MASK", "PASTE", "OUTLINE" , NULL};
static const char *right_const_side[] = {"TOP", "BOTTOM", NULL};

enum {
	RC_OBJTYPE,
	RC_YESNO,
	RC_LAYERPOS,
	RC_LAYERTYPE,
	RC_SIDE
};

static expr_wizard_op_t right_const_tab[] = {
	{right_const_objtype, NULL},
	{right_const_yesno, NULL},
	{right_const_layerpos, NULL},
	{right_const_layertype, NULL},
	{right_const_side, NULL},
	{NULL, NULL}
};

static const expr_wizard_t expr_tab[] = {
	{"@.id",              "object ID",        &op_tab[OPS_ANY], RIGHT_INT, NULL},
	{"@.type",            "object type",      &op_tab[OPS_EQ],  RIGHT_CONST, &right_const_tab[RC_OBJTYPE]},

	{NULL,                "bounding box",     NULL,             0, NULL},
	{"@.bbox.x1",         "X1",               &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.bbox.y1",         "Y1",               &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.bbox.x2",         "X2",               &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.bbox.y2",         "Y2",               &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.bbox.w",          "width",            &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.bbox.h",          "height",           &op_tab[OPS_ANY], RIGHT_COORD, NULL},


	{NULL,                "trace object's",   NULL,             0, NULL},
	{"@.thickness",       "thickness",        &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.clearance",       "clearance",        &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.length",          "length",           &op_tab[OPS_ANY], RIGHT_COORD, NULL},

	{NULL,                "line",             NULL,             0, NULL},
	{"@.x1",              "X1",               &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.y1",              "Y1",               &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.x2",              "X2",               &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.y2",              "Y2",               &op_tab[OPS_ANY], RIGHT_COORD, NULL},

	{NULL,                "arc",              NULL,             0, NULL},
	{"@.x",               "center X",         &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.y",               "center Y",         &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.angle.start",     "start angle",      &op_tab[OPS_ANY], RIGHT_DOUBLE, NULL},
	{"@.angle.delta",     "delta angle",      &op_tab[OPS_ANY], RIGHT_DOUBLE, NULL},

	{NULL,                "text",             NULL,             0, NULL},
	{"@.x",               "X",                &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.y",               "Y",                &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.scale",           "scale",            &op_tab[OPS_ANY], RIGHT_INT, NULL},
	{"@.string",          "string",           &op_tab[OPS_ANY], RIGHT_STR, NULL},
	{"@.rotation",        "rotation",         &op_tab[OPS_ANY], RIGHT_DOUBLE, NULL},

	{NULL,                "polygon",          NULL,             0, NULL},
	{"@.points",          "points",           &op_tab[OPS_ANY], RIGHT_INT, NULL},

	{NULL,                "padstack",         NULL,             0, NULL},
	{"@.x",               "X",                &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.y",               "Y",                &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.clearance",       "global clearance", &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.rotation",        "rotation",         &op_tab[OPS_ANY], RIGHT_DOUBLE, NULL},
	{"@.xmirror",         "x coords mirrored",&op_tab[OPS_ANY], RIGHT_INT, NULL},
	{"@.smirror",         "stackup mirrored", &op_tab[OPS_ANY], RIGHT_INT, NULL},
	{"@.name",            "name",             &op_tab[OPS_STR], RIGHT_STR, NULL},
	{"@.number",          "number",           &op_tab[OPS_STR], RIGHT_STR, NULL},
	{"@.hole",            "proto: hole dia",  &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.plated",          "proto: is plated?",&op_tab[OPS_ANY], RIGHT_INT, NULL},

	{NULL,                "subcircuit",       NULL,             0, NULL},
	{"@.x",               "X",                &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.y",               "Y",                &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.rotation",        "rotation",         &op_tab[OPS_ANY], RIGHT_DOUBLE, NULL},
	{"@.side",            "side",             &op_tab[OPS_ANY], RIGHT_CONST, &right_const_tab[RC_SIDE]},
	{"@.refdes",          "refdes",           &op_tab[OPS_STR], RIGHT_STR, NULL},
	{"@.footprint",       "footprint",        &op_tab[OPS_STR], RIGHT_STR, NULL},
	{"@.value",           "value",            &op_tab[OPS_STR], RIGHT_STR, NULL},

	{NULL,                "host layer's",     NULL,             0, NULL},
	{"@.layer.name",      "name",             &op_tab[OPS_STR], RIGHT_STR, NULL},
	{"@.layer.visible",   "visible",          &op_tab[OPS_EQ],  RIGHT_CONST, &right_const_tab[RC_YESNO]},
	{"@.layer.position",  "stack position",   &op_tab[OPS_EQ],  RIGHT_CONST, &right_const_tab[RC_LAYERPOS]},
	{"@.layer.type",      "type",             &op_tab[OPS_EQ],  RIGHT_CONST, &right_const_tab[RC_LAYERTYPE]},

	{NULL,                "host subcircuit's",NULL,             0, NULL},
	{"@.subc.x",          "X",                &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.subc.y",          "Y",                &op_tab[OPS_ANY], RIGHT_COORD, NULL},
	{"@.subc.rotation",   "rotation",         &op_tab[OPS_ANY], RIGHT_DOUBLE, NULL},
	{"@.subc.side",       "side",             &op_tab[OPS_ANY], RIGHT_CONST, &right_const_tab[RC_SIDE]},
	{"@.subc.refdes",     "refdes",           &op_tab[OPS_STR], RIGHT_STR, NULL},
	{"@.subc.footprint",  "a.footprint",      &op_tab[OPS_STR], RIGHT_STR, NULL},
	{"@.subc.value",      "a.value",          &op_tab[OPS_STR], RIGHT_STR, NULL},

	{NULL, NULL, NULL, 0, NULL}
};

