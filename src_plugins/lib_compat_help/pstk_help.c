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
 */

#include "config.h"

#include "pstk_help.h"
#include "obj_pstk_inlines.h"
#include "compat_misc.h"
#include "search.h"

pcb_pstk_t *pcb_pstk_new_hole(pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_bool plated)
{
	pcb_pstk_proto_t proto;
	pcb_cardinal_t pid;

	memset(&proto, 0, sizeof(proto));
	proto.hdia = drill_dia;
	proto.hplated = plated;

	pid = pcb_pstk_proto_insert_dup(data, &proto, 1);

	return pcb_pstk_new(data, pid, x, y, 0, pcb_flag_make(PCB_FLAG_CLEARLINE));
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

int pcb_pstk_vect2pstk_thr(pcb_data_t *data, vtp0_t *objs)
{
	int l, n, plated, done = 0, ci;
	pcb_coord_t cx, cy, d, r, valid;
	pcb_any_obj_t *cand[NUM_LYTS];
	char *term = NULL;
	int num_cand[NUM_LYTS];
	pcb_pstk_proto_t proto;
	vtp0_t tmp;
	int quiet = 1;

	/* output padstacks are going to be marked with the FOUND flag */
	for(n = 0; n < objs->used; n++) PCB_FLAG_CLEAR(PCB_FLAG_FOUND, (pcb_any_obj_t *)(objs->array[n]));

	for(n = 0; n < objs->used; n++) {
		pcb_any_obj_t *h = objs->array[n];
		pcb_pstk_proto_t *p;

		/* find holes */
		switch(h->type) {
			case PCB_OBJ_VIA:
				cx = ((pcb_pin_t *)h)->X;
				cy = ((pcb_pin_t *)h)->Y;
				d = ((pcb_pin_t *)d)->DrillingHole;
				plated = !PCB_FLAG_TEST(PCB_FLAG_HOLE, h);
				break;
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
					if (!pcb_is_point_in_pad(cx, cy, r, (pcb_pad_t *)c)) continue;
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

		vtp0_init(&tmp);
		vtp0_append(&tmp, h);
		for(l = 0; l < objs->used; l++) {
			if (num_cand[l] == 1) {
				vtp0_append(&tmp, cand[l]);
				if (term == NULL)
					term = pcb_attribute_get(&cand[l]->Attributes, "term");
			}
		}

		memset(&proto, 0, sizeof(proto));
		if (pcb_pstk_proto_conv(data, &proto, quiet, &tmp, cx, cy) == 0) {
			proto.hplated = plated;
			pcb_cardinal_t pid = pcb_pstk_proto_insert_or_free(data, &proto, quiet);
			if (pid != PCB_PADSTACK_INVALID) {
				pcb_pstk_t *ps = pcb_pstk_new(data, pid, 0, 0, 0, pcb_flag_make(PCB_FLAG_CLEARLINE | PCB_FLAG_FOUND));
				if (term != NULL)
					pcb_attribute_put(&ps->Attributes, "term", term);
				/* got our new proto - remove the objects we used */
				for(ci = 0; ci < tmp.used; ci++) {
					int i;
					for(i = 0; i < objs->used; i++) {
						if (objs->array[i] == tmp.array[ci]) {
							vtp0_remove(objs, i, 1);
							break;
						}
					}
				}
			}
			
			/* we have deleted from objs, need to start over the main loop */
			n = 0;
		}

		vtp0_uninit(&tmp);
	}

	/* output padstacks are going to be marked with the FOUND flag */
	for(n = 0; n < objs->used; n++) {
		PCB_FLAG_CLEAR(PCB_FLAG_FOUND, (pcb_any_obj_t *)(objs->array[n]));
	}

	return done;
}

int pcb_pstk_vect2pstk_smd(pcb_data_t *data, vtp0_t *objs)
{
	return -1;
}

int pcb_pstk_vect2pstk(pcb_data_t *data, vtp0_t *objs)
{
	int t = pcb_pstk_vect2pstk_thr(data, objs);
	int s = pcb_pstk_vect2pstk_smd(data, objs);
	if ((t < 0) && (s < 0))
		return -1;
	if (t < 0) t = 0;
	if (s < 0) s = 0;
	return t+s;
}

