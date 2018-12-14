/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "pstk_help.h"
#include "obj_pstk_inlines.h"
#include "compat_misc.h"
#include "search.h"
#include "remove.h"
#include "find.h"


pcb_pstk_t *pcb_pstk_new_hole(pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_bool plated)
{
	pcb_pstk_proto_t proto;
	pcb_cardinal_t pid;

	memset(&proto, 0, sizeof(proto));
	proto.hdia = drill_dia;
	proto.hplated = plated;

	pcb_pstk_proto_update(&proto);
	pid = pcb_pstk_proto_insert_dup(data, &proto, 1);

	return pcb_pstk_new(data, -1, pid, x, y, 0, pcb_flag_make(PCB_FLAG_CLEARLINE));
}

pcb_layer_type_t lyts[] = {
	/* primary for thru-hole */
	PCB_LYT_TOP    | PCB_LYT_COPPER,
	PCB_LYT_BOTTOM | PCB_LYT_COPPER,

	/* secondary */
	PCB_LYT_INTERN | PCB_LYT_COPPER,
	PCB_LYT_TOP    | PCB_LYT_PASTE,
	PCB_LYT_TOP    | PCB_LYT_SILK,
	PCB_LYT_TOP    | PCB_LYT_MASK,
	PCB_LYT_BOTTOM | PCB_LYT_MASK,
	PCB_LYT_BOTTOM | PCB_LYT_SILK,
	PCB_LYT_BOTTOM | PCB_LYT_PASTE
};

#define NUM_LYTS (sizeof(lyts) / sizeof(lyts[0]))


static int vect2pstk_conv_cand(pcb_data_t *data, vtp0_t *objs, pcb_bool_t quiet, pcb_any_obj_t **cand, int *num_cand, pcb_coord_t cx, pcb_coord_t cy, int plated, pcb_any_obj_t *extrao)
{
	pcb_pstk_proto_t proto;
	int l, ci, res = -1;
	vtp0_t tmp;
	char *term = NULL;

	vtp0_init(&tmp);
	if (extrao != NULL)
		vtp0_append(&tmp, extrao);
	for(l = 0; l < NUM_LYTS; l++) {
		if (num_cand[l] == 1) {
			vtp0_append(&tmp, cand[l]);
			if (term == NULL)
				term = pcb_attribute_get(&cand[l]->Attributes, "term");
		}
	}

	pcb_trace("Converting %d objects into a pstk\n", tmp.used);

	memset(&proto, 0, sizeof(proto));
	if (pcb_pstk_proto_conv(data, &proto, quiet, &tmp, cx, cy) == 0) {
		pcb_cardinal_t pid;
		if (plated != -1)
			proto.hplated = plated;
		pid = pcb_pstk_proto_insert_or_free(data, &proto, quiet);
		if (pid != PCB_PADSTACK_INVALID) {
			pcb_pstk_t *ps = pcb_pstk_new(data, -1, pid, 0, 0, 0, pcb_flag_make(PCB_FLAG_CLEARLINE | PCB_FLAG_FOUND));
			vtp0_append(objs, ps);
			if (term != NULL)
				pcb_attribute_put(&ps->Attributes, "term", term);
			/* got our new proto - remove the objects we used */
			for(ci = 0; ci < tmp.used; ci++) {
				int i;
				pcb_any_obj_t *o = tmp.array[ci];
				for(i = 0; i < objs->used; i++) {
					if (objs->array[i] == o) {
						vtp0_remove(objs, i, 1);
						pcb_destroy_object(data, o->type, o->parent.any, o, o);
						break;
					}
				}
			}
		}
		res = 0;
	}
	vtp0_uninit(&tmp);
	return res;
}

int pcb_pstk_vect2pstk_thr(pcb_data_t *data, vtp0_t *objs, pcb_bool_t quiet)
{
	int l, n, plated, done = 0, ci;
	pcb_pstk_proto_t *p;
	pcb_coord_t cx, cy, d, r, valid;
	pcb_any_obj_t *cand[NUM_LYTS];
	int num_cand[NUM_LYTS];

	/* output padstacks are going to be marked with the FOUND flag */
	for(n = 0; n < objs->used; n++) PCB_FLAG_CLEAR(PCB_FLAG_FOUND, (pcb_any_obj_t *)(objs->array[n]));

	for(n = 0; n < objs->used; n++) {
		pcb_any_obj_t *h = objs->array[n];

		/* find holes */
		switch(h->type) {
			case PCB_OBJ_PSTK:
				p = pcb_pstk_get_proto((pcb_pstk_t *)h);
				if ((p == NULL) || (p->hdia <= 0))
					continue;
				cx = ((pcb_pstk_t *)h)->x;
				cy = ((pcb_pstk_t *)h)->y;
				d = p->hdia;
				plated = p->hplated;
				break;
			default:
				continue;
		}
		r = d/2;

		/* we have a hole center and dia - look for objects that could be pads for this hole */
		memset(cand, 0, sizeof(cand));
		memset(num_cand, 0, sizeof(num_cand));
		for(ci = 0; ci < objs->used; ci++) {
			pcb_layer_type_t lyt;
			pcb_any_obj_t *c = objs->array[ci]; /* candidate */
			if (PCB_FLAG_TEST(PCB_FLAG_FOUND, c)) continue; /* already used */
			switch(c->type) {
				case PCB_OBJ_LINE:
					if (!pcb_is_point_in_line(cx, cy, r, (pcb_any_line_t *)c)) continue;
					break;
				case PCB_OBJ_POLY:
					if (!pcb_poly_is_point_in_p(cx, cy, r, (pcb_poly_t *)c)) continue;
					break;
				default: continue; /* this type can not be used */
			}

			/* candidate is around the hole - figure the indexing from lyt */
			assert(c->parent_type == PCB_PARENT_LAYER);
			lyt = pcb_layer_flags_(c->parent.layer);
			for(l = 0; l < NUM_LYTS; l++) {
				if (lyts[l] == lyt) {
					cand[l] = c;
					num_cand[l]++;
					break;
				}
			}
		}


		/* all candidates are in cand[] and num_cand[]; check if there's any
		   usable candidate (layer type with only one candidate); allow multiple
		   intern coppers for now */
		valid = 0;
		for(l = 0; l < NUM_LYTS; l++) {
			if ((num_cand[l] == 1) || ((num_cand[l] > 1) && (lyts[l] & PCB_LYT_INTERN))) {
				num_cand[l] = 1;
				valid = 1;
				break;
			}
		}

		if (!valid) continue; /* this hole can not be converted */

		if (vect2pstk_conv_cand(data, objs, quiet, cand, num_cand, cx, cy, plated, h) == 0) {
			/* we have deleted from objs, need to start over the main loop */
			n = 0;
		}
	}

	/* output padstacks are going to be marked with the FOUND flag */
	for(n = 0; n < objs->used; n++) PCB_FLAG_CLEAR(PCB_FLAG_FOUND, (pcb_any_obj_t *)(objs->array[n]));

	return done;
}

int pcb_pstk_vect2pstk_smd(pcb_data_t *data, vtp0_t *objs, pcb_bool_t quiet)
{
	int l, n, done = 0, ci;
	pcb_coord_t cx, cy;
	pcb_any_obj_t *cand[NUM_LYTS];
	int num_cand[NUM_LYTS];

	/* output padstacks are going to be marked with the FOUND flag */
	for(n = 0; n < objs->used; n++) PCB_FLAG_CLEAR(PCB_FLAG_FOUND, (pcb_any_obj_t *)(objs->array[n]));

	for(n = 0; n < objs->used; n++) {
		pcb_any_obj_t *o = objs->array[n];

		if ((o->type == PCB_OBJ_LINE) || (o->type == PCB_OBJ_ARC) || (o->type == PCB_OBJ_POLY)) {
			pcb_layer_type_t olyt;

			assert(o->parent_type == PCB_PARENT_LAYER);
			olyt = pcb_layer_flags_(o->parent.layer);
			if ((!(olyt & PCB_LYT_COPPER)) || (olyt & PCB_LYT_INTERN))
				continue; /* deal with outer copper objects only */

			/* assume padstack origin is middle of the object, which is approximated here */
			cx = (o->BoundingBox.X1 + o->BoundingBox.X2) / 2;
			cy = (o->BoundingBox.Y1 + o->BoundingBox.Y2) / 2;

			memset(cand, 0, sizeof(cand));
			memset(num_cand, 0, sizeof(num_cand));
			for(l = 0; l < NUM_LYTS; l++) {
				if (lyts[l] == olyt) {
					cand[l] = o;
					num_cand[l]++;
					break;
				}
			}

			for(ci = 0; ci < objs->used; ci++) {
				pcb_layer_type_t lyt;
				pcb_any_obj_t *c = objs->array[ci]; /* candidate */

				lyt = pcb_layer_flags_(c->parent.layer);

				if ((lyt & PCB_LYT_ANYWHERE) != (olyt & PCB_LYT_ANYWHERE))
					continue; /* care for the same side only */

				if (!((lyt & (PCB_LYT_MASK | PCB_LYT_PASTE))))
					continue; /* care for mask and paste objects */

				if (!pcb_intersect_obj_obj(o, c))
					continue; /* only if intersects with the original copper pad */

				for(l = 0; l < NUM_LYTS; l++) {
					if (lyts[l] == lyt) {
						cand[l] = c;
						num_cand[l]++;
						break;
					}
				}
			}
			if (vect2pstk_conv_cand(data, objs, quiet, cand, num_cand, cx, cy, -1, NULL) == 0) {
				/* we have deleted from objs, need to start over the main loop */
				n = 0;
			}
		}
	}

	return done;
}

int pcb_pstk_vect2pstk(pcb_data_t *data, vtp0_t *objs, pcb_bool_t quiet)
{
	int t = pcb_pstk_vect2pstk_thr(data, objs, quiet);
	int s = pcb_pstk_vect2pstk_smd(data, objs, quiet);
	if ((t < 0) && (s < 0))
		return -1;
	if (t < 0) t = 0;
	if (s < 0) s = 0;
	return t+s;
}


pcb_pstk_t *pcb_pstk_new_from_shape(pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_bool plated, pcb_coord_t glob_clearance, pcb_pstk_shape_t *shape)
{
	pcb_pstk_proto_t proto;
	pcb_cardinal_t pid;
	pcb_pstk_tshape_t tshp;
	pcb_pstk_shape_t *s;

	memset(&proto, 0, sizeof(proto));
	memset(&tshp, 0, sizeof(tshp));

	tshp.len = 0;
	for(s = shape; s->layer_mask != 0; s++,tshp.len++) ;
	tshp.shape = shape;
	proto.tr.alloced = proto.tr.used = 1; /* has the canonical form only */
	proto.tr.array = &tshp;

	proto.hdia = drill_dia;
	proto.hplated = plated;

	pcb_pstk_proto_update(&proto);
	pid = pcb_pstk_proto_insert_dup(data, &proto, 1);
	if (pid == PCB_PADSTACK_INVALID)
		return NULL;

	return pcb_pstk_new(data, -1, pid, x, y, glob_clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
}

void pcb_shape_rect(pcb_pstk_shape_t *shape, pcb_coord_t width, pcb_coord_t height)
{
	pcb_pstk_poly_t *dst = &shape->data.poly;

	pcb_pstk_shape_alloc_poly(dst, 4);
	shape->shape = PCB_PSSH_POLY;

	width/=2;
	height/=2;

	dst->x[0] = -width; dst->y[0] = -height;
	dst->x[1] = +width; dst->y[1] = -height;
	dst->x[2] = +width; dst->y[2] = +height;
	dst->x[3] = -width; dst->y[3] = +height;
}

void pcb_shape_oval(pcb_pstk_shape_t *shape, pcb_coord_t width, pcb_coord_t height)
{
	shape->shape = PCB_PSSH_LINE;

	if (width == height) {
		shape->shape = PCB_PSSH_CIRC;

		shape->data.circ.x = shape->data.circ.y = 0;
		shape->data.circ.dia = height;
	}
	else if (width > height) {
		shape->data.line.x1 = -width/2 + height/2; shape->data.line.y1 = 0;
		shape->data.line.x2 = +width/2 - height/2; shape->data.line.y2 = 0;
		shape->data.line.thickness = height;
	}
	else {
		shape->data.line.x1 = 0; shape->data.line.y1 = -height/2 + width/2;
		shape->data.line.x2 = 0; shape->data.line.y2 = +height/2 - width/2;
		shape->data.line.thickness = width;
	}
}
