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
#include "data_parent.h"

#define LID_EDIT 0
#define LID_TARGET 1
#define COPPER_END 0
#define SILK_END 1


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
	set_grp((pcb_any_obj_t *)l, group);

	return pcb_exto_regen_end(subc);
}


static void pcb_cord_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
	pcb_exto_draw_makr(info, subc);
}

static void pcb_cord_float_pre(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_trace("cord: float pre %ld %ld role=%s\n", subc->ID, floater->ID, floater->extobj_role);

	cord_clear(subc, group_of(floater));
}

static void pcb_cord_float_geo(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	const char *grp;

	if (floater == NULL) {
		/* post-floater-del update: noop */
		return;
	}

	pcb_trace("cord: float geo %ld %ld role=%s\n", subc->ID, floater->ID, floater->extobj_role);

	if (floater->extobj_role == NULL)
		return;

	grp = group_of(floater);
	if (grp == NULL)
		return;

	cord_gen(subc, grp);

	if ((floater->type == PCB_OBJ_PSTK) && (pcb_attribute_get(&subc->Attributes, "extobj::fixed_origin") == NULL)) {
		pcb_pstk_t *ps = (pcb_pstk_t *)floater;
		pcb_subc_move_origin_to(subc, ps->x + PCB_MM_TO_COORD(0.3), ps->y + PCB_MM_TO_COORD(0.3), 0);
	}
}

static pcb_extobj_del_t pcb_cord_float_del(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_pstk_t *p, *next;
	const char *group = group_of(floater);
	int has_other_grp = 0;

	pcb_trace("cord: float del %ld %ld\n", subc->ID, floater->ID);

	if ((floater->type != PCB_OBJ_PSTK) || (group == NULL))
		return PCB_EXTODEL_FLOATER; /* e.g. refdes text - none of our business */

	for(p = padstacklist_first(&subc->data->padstack); p != NULL; p = next) {
		const char *lg;
		next = padstacklist_next(p);
		if ((pcb_any_obj_t *)p == floater) continue;  /* do not remove the input floater, the caller may need it */
		lg = group_of((pcb_any_obj_t *)p);
		if ((lg == NULL) || (strcmp(lg, group) != 0)) {
			has_other_grp = 1;
			continue;
		}
		pcb_pstk_free(p);
	}

	return has_other_grp ? PCB_EXTODEL_FLOATER : PCB_EXTODEL_SUBC;
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

static pcb_pstk_t *endpt_pstk(pcb_data_t *data, pcb_cardinal_t pid, pcb_coord_t x, pcb_coord_t y, const char *term, const char *grp, int floater)
{
	pcb_pstk_t *ps;

	ps = pcb_pstk_new(data, -1, pid, x, y, 0, pcb_flag_make(0));
	set_grp((pcb_any_obj_t *)ps, grp);
	if (floater)
		PCB_FLAG_SET(PCB_FLAG_FLOATER, ps);
	pcb_attribute_put(&ps->Attributes, "extobj::role", "edit");
	pcb_attribute_put(&ps->Attributes, "intconn", grp);
	if (term != NULL)
		pcb_attribute_put(&ps->Attributes, "term", term);
	return ps;
}


static void conv_pstk(pcb_subc_t *subc, pcb_pstk_t *ps, long *grp, long *term, int *has_origin)
{
	char sgrp[16], sterm[16];

	sprintf(sgrp, "%ld", (*grp)++);

	endpt_pstk(subc->data, COPPER_END, ps->x, ps->y, ps->term, sgrp, 1);

	sprintf(sterm, "cord%ld", (*term)++);
	endpt_pstk(subc->data, SILK_END, ps->x, ps->y, ps->term, sgrp, 0);

	if (!*has_origin) {
		pcb_subc_move_origin_to(subc, ps->x + PCB_MM_TO_COORD(0.3), ps->y + PCB_MM_TO_COORD(0.3), 0);
		*has_origin = 1;
	}

	cord_gen(subc, sgrp);
}

static pcb_subc_t *pcb_cord_conv_objs(pcb_data_t *dst, vtp0_t *objs, pcb_subc_t *copy_from)
{
	pcb_subc_t *subc;
	long n, grp = 1, term = 0; /* for intconn grp needs to start from 1 */
	pcb_coord_t ox = 0, oy = 0, has_origin = 0, has_subc = 0;
	pcb_dflgmap_t layers[] = {
		{"edit", PCB_LYT_DOC, "extobj", 0, 0},
		{"target", PCB_LYT_SILK | PCB_LYT_TOP, NULL, 0, 0},
		{NULL, 0, NULL, 0, 0}
	};

	pcb_trace("cord: conv_objs\n");

	/* origin override: if converting subcircuits, keep the first subcircuits origin */
	for(n = 0; n < objs->used; n++) {
		pcb_subc_t *s = objs->array[n];
		if (s->type != PCB_OBJ_SUBC) continue;
		has_subc = 1;

		if (pcb_subc_get_origin(s, &ox, &oy) == 0) {
			has_origin = 1;
			break;
		}
	}

	/* set target layer from the first line object's layer */
	for(n = 0; n < objs->used; n++) {
		pcb_line_t *l = objs->array[n];
		if (l->type != PCB_OBJ_LINE) continue;

		layers[1].lyt = pcb_layer_flags_(l->parent.layer);
		pcb_layer_purpose_(l->parent.layer, &layers[1].purpose);
		if (!has_origin) {
			ox = l->Point1.X + PCB_MM_TO_COORD(0.3);
			oy = l->Point1.Y + PCB_MM_TO_COORD(0.3);
			has_origin = 1;
		}
		break;
	}

	subc = pcb_exto_create(dst, "cord", layers, ox, oy, 0, copy_from);

	/* create padstack prototypes */
	if (endpt_pstk_proto(subc->data, PCB_LYT_COPPER | PCB_LYT_TOP) != COPPER_END)
		pcb_message(PCB_MSG_WARNING, "extended object cord: wrong pstk proto ID for copper end\n");
	if (endpt_pstk_proto(subc->data, PCB_LYT_SILK | PCB_LYT_TOP) != SILK_END)
		pcb_message(PCB_MSG_WARNING, "extended object cord: wrong pstk proto ID for silk end\n");

	/* convert lines into 2-ended cords */
	for(n = 0; n < objs->used; n++) {
		char sgrp[16], sterm[16];
		pcb_line_t *l = objs->array[n];

		if (l->type != PCB_OBJ_LINE) continue;

		sprintf(sgrp, "%ld", grp++);

		sprintf(sterm, "cord%ld", term++);
		endpt_pstk(subc->data, COPPER_END, l->Point1.X, l->Point1.Y, sterm, sgrp, 1);

		sprintf(sterm, "cord%ld", term++);
		endpt_pstk(subc->data, COPPER_END, l->Point2.X, l->Point2.Y, sterm, sgrp, 1);

		cord_gen(subc, sgrp);
	}

	/* convert padstacks into single-ended cords, one end anchored at the original position */
	for(n = 0; n < objs->used; n++) {
		pcb_pstk_t *ps = objs->array[n];
		if (ps->type == PCB_OBJ_PSTK)
			conv_pstk(subc, ps, &grp, &term, &has_origin);
	}

	/* convert subcircuits (footprints) */
	/* step 1: convert padstacks */
	for(n = 0; n < objs->used; n++) {
		pcb_pstk_t *ps;
		pcb_subc_t *s = objs->array[n];
		if (s->type != PCB_OBJ_SUBC) continue;

		for(ps = padstacklist_first(&s->data->padstack); ps != NULL; ps = padstacklist_next(ps))
			conv_pstk(subc, ps, &grp, &term, &has_origin);
	}

	/* step 2: copy layer objects */
	for(n = 0; n < objs->used; n++) {
		int lid;
		pcb_subc_t *s = objs->array[n];
		if (s->type != PCB_OBJ_SUBC) continue;

		pcb_attribute_copy_all(&subc->Attributes, &s->Attributes);

		for(lid = 0; lid < s->data->LayerN; lid++) {
			gdl_iterator_t it;
			pcb_line_t *line;
			pcb_arc_t *arc;
			pcb_text_t *text;
			pcb_poly_t *poly;
			pcb_layer_t *sl = &s->data->Layer[lid], *dl;

			if (strcmp(sl->name, "subc-aux") == 0) continue;

			TODO("this layer copy is code dup from subc_dup_at(); make it common");
			dl = &subc->data->Layer[subc->data->LayerN++];
			dl->is_bound = 1;
			dl->type = PCB_OBJ_LAYER;
			memcpy(&dl->meta.bound, &sl->meta.bound, sizeof(sl->meta.bound));
			dl->name = pcb_strdup(sl->name);
			dl->comb = sl->comb;
			if (dl->meta.bound.real != NULL)
				pcb_layer_link_trees(dl, dl->meta.bound.real);

			linelist_foreach(&sl->Line, &it, line) {
				pcb_line_t *nline = pcb_line_dup_at(dl, line, 0, 0);
				if (nline != NULL)
					PCB_SET_PARENT(nline, layer, dl);
			}

			arclist_foreach(&sl->Arc, &it, arc) {
				pcb_arc_t *narc = pcb_arc_dup_at(dl, arc, 0, 0);
				if (narc != NULL)
					PCB_SET_PARENT(narc, layer, dl);
			}

			textlist_foreach(&sl->Text, &it, text) {
				pcb_text_t *ntext = pcb_text_dup_at(dl, text, 0, 0);
				if (ntext != NULL)
					PCB_SET_PARENT(ntext, layer, dl);
			}

			polylist_foreach(&sl->Polygon, &it, poly) {
				pcb_poly_t *npoly = pcb_poly_dup_at(dl, poly, 0, 0);
				if (npoly != NULL)
					PCB_SET_PARENT(npoly, layer, dl);
			}
		}
	}


	if (has_subc)
		pcb_attribute_put(&subc->Attributes, "extobj::fixed_origin", "(yes)");

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
