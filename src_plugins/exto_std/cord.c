/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Extended object for a cords: insulated wires with 1 or 2 conductive end(s)
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "search.h"

#define LID_EDIT 0
#define LID_TARGET 1
#define COPPER_END 0
#define SILK_END 0


static const char *group_of(pcb_any_obj_t *floater)
{
	const char *grp = pcb_attribute_get(&floater->Attributes, "cord::group");
	if ((grp == NULL) || (*grp == '\0')) return NULL;
	return grp;
}

static void set_grp(pcb_any_obj_t *obj, const char *grp)
{
	pcb_attribute_put(&obj->Attributes, "cord::group", grp);
}

/* remove all existing graphics from the subc */
static void cord_clear(pcb_subc_t *subc, const char *group)
{
	pcb_layer_t *ly = &subc->data->Layer[LID_TARGET];
	pcb_line_t *l, *next;

	for(l = linelist_first(&ly->Line); l != NULL; l = next) {
		next = linelist_next(l);
		if (group != NULL) {
			const char *lg = group_of((pcb_any_obj_t *)l);
			if ((lg == NULL) || (strcmp(lg, group) != 0)) continue;
		}
		pcb_line_free(l);
	}
}

/* find the two endpoint padstacks of a group */
static void cord_get_ends(pcb_subc_t *subc, const char *group, pcb_pstk_t **end1, pcb_pstk_t **end2)
{
	pcb_pstk_t *p;

	*end1 = *end2 = NULL;

	for(p = padstacklist_first(&subc->data->padstack); p != NULL; p = padstacklist_next(p)) {
		const char *lg = group_of((pcb_any_obj_t *)p);
		if ((lg == NULL) || (strcmp(lg, group) != 0)) continue;
		if (*end1 == NULL) *end1 = p;
		else if (*end2 == NULL) {
			*end2 = p;
			break;
		}
	}
}

/* create the graphics */
static int cord_gen(pcb_subc_t *subc, const char *group)
{
	pcb_layer_t *ly = &subc->data->Layer[LID_TARGET];
	pcb_line_t *l;
	pcb_pstk_t *e1, *e2;

	cord_get_ends(subc, group, &e1, &e2);
	if ((e1 == NULL) || (e2 == NULL)) {
		pcb_message(PCB_MSG_ERROR, "extended object cord: failed to generate cord for #%ld group %s: missing endpoint\n", subc->ID, group);
		return -1;
	}

	pcb_exto_regen_begin(subc);

	l = pcb_line_new(ly, e1->x, e1->y, e2->x, e2->y,
		PCB_MM_TO_COORD(0.25), 0, pcb_flag_make(0));
	pcb_attribute_put(&l->Attributes, "extobj::role", "gfx");
	pcb_attribute_put(&l->Attributes, "extobj::group", group);

	return pcb_exto_regen_end(subc);
}


static void pcb_cord_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
}

static void pcb_cord_float_pre(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_trace("cord: float pre %ld %ld role=%s\n", subc->ID, floater->ID, floater->extobj_role);

	cord_clear(subc, group_of(floater));
}

static void pcb_cord_float_geo(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	const char *grp;
	pcb_trace("cord: float geo %ld %ld role=%s\n", subc->ID, floater->ID, floater->extobj_role);

	if (floater->extobj_role == NULL)
		return;

	grp = group_of(floater);
	if (grp == NULL)
		return;

	cord_gen(subc, grp);
}

static pcb_extobj_del_t pcb_cord_float_del(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
TODO("remove only the group");
	pcb_trace("cord: float del %ld %ld\n", subc->ID, floater->ID);
	return PCB_EXTODEL_SUBC;
}

static void pcb_cord_chg_attr(pcb_subc_t *subc, const char *key, const char *value)
{
	pcb_trace("cord chg_attr\n");
}

static pcb_cardinal_t endpt_pstk_proto(pcb_data_t *data, pcb_layer_type_t lyt)
{
	pcb_pstk_proto_t proto;
	pcb_pstk_shape_t shape;
	pcb_pstk_tshape_t tshp;

	memset(&proto, 0, sizeof(proto));
	memset(&tshp, 0, sizeof(tshp));

	tshp.shape = &shape;
	proto.tr.alloced = proto.tr.used = 1; /* has the canonical form only */
	proto.tr.array = &tshp;

	shape.shape = PCB_PSSH_CIRC;
	shape.data.circ.dia = PCB_MM_TO_COORD(0.5);
	shape.data.circ.x = shape.data.circ.y = 0;
	
	shape.layer_mask = lyt;
	shape.comb = 0;
	tshp.len = 1;

	proto.hdia = 0;
	proto.hplated = 0;

	pcb_pstk_proto_update(&proto);
	return pcb_pstk_proto_insert_dup(data, &proto, 1);
}

static pcb_pstk_t *endpt_pstk(pcb_data_t *data, pcb_cardinal_t pid, pcb_coord_t x, pcb_coord_t y, const char *grp)
{
	pcb_pstk_t *ps;

	ps = pcb_pstk_new(data, -1, pid, x, y, 0, pcb_flag_make(0));
	set_grp((pcb_any_obj_t *)ps, grp);
	PCB_FLAG_SET(PCB_FLAG_FLOATER, ps);
	pcb_attribute_put(&ps->Attributes, "extobj::role", "edit");
	return ps;
}


static pcb_subc_t *pcb_cord_conv_objs(pcb_data_t *dst, vtp0_t *objs, pcb_subc_t *copy_from)
{
	pcb_subc_t *subc;
	long n, grp = 0;
	pcb_coord_t ox = 0, oy = 0;
	pcb_dflgmap_t layers[] = {
		{"edit", PCB_LYT_DOC, "extobj", 0, 0},
		{"target", PCB_LYT_SILK | PCB_LYT_TOP, NULL, 0, 0},
		{NULL, 0, NULL, 0, 0}
	};

	pcb_trace("cord: conv_objs\n");

	/* set target layer from the first line object's layer */
	for(n = 0; n < objs->used; n++) {
		pcb_line_t *l = objs->array[n];
		if (l->type != PCB_OBJ_LINE) continue;

		layers[1].lyt = pcb_layer_flags_(l->parent.layer);
		pcb_layer_purpose_(l->parent.layer, &layers[1].purpose);
		ox = l->Point1.X;
		oy = l->Point1.Y;
		break;
	}

	subc = pcb_exto_create(dst, "cord", layers, ox, oy, 0, copy_from);

	/* create padstack prototypes */
	if (endpt_pstk_proto(subc->data, PCB_LYT_COPPER | PCB_LYT_TOP) != COPPER_END)
		pcb_message(PCB_MSG_WARNING, "extended object cord: wrong pstk proto ID for copper end\n");
	if (endpt_pstk_proto(subc->data, PCB_LYT_SILK | PCB_LYT_TOP) != SILK_END)
		pcb_message(PCB_MSG_WARNING, "extended object cord: wrong pstk proto ID for silk end\n");

	for(n = 0; n < objs->used; n++) {
		pcb_line_t *l = objs->array[n];
		char sgrp[16];

		if (l->type != PCB_OBJ_LINE) continue;

		sprintf(sgrp, "%ld", grp++);

		endpt_pstk(subc->data, COPPER_END, l->Point1.X, l->Point1.Y, sgrp);
		endpt_pstk(subc->data, COPPER_END, l->Point2.X, l->Point2.Y, sgrp);

		cord_gen(subc, sgrp);
	}

TODO("convert thru-hole pins to single ended cords, keepin term");
	for(n = 0; n < objs->used; n++) {
		pcb_pstk_t *ps = objs->array[n];

		if (ps->type != PCB_OBJ_PSTK) continue;
	}

	return subc;
}

static pcb_extobj_t pcb_cord = {
	"cord",
	pcb_cord_draw_mark,
	pcb_cord_float_pre,
	pcb_cord_float_geo,
	NULL,
	pcb_cord_float_del,
	pcb_cord_chg_attr,
	NULL,
	pcb_cord_conv_objs,
	NULL,
};
