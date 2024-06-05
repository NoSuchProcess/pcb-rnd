/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
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
#include "ht_endp.h"
#include "ht_endp.c"

#include "endp.h"

RND_INLINE void hpgl_add_endp(htendp_t *ht, rnd_cheap_point_t ep, void *a)
{
	htendp_entry_t *e;
	e = htendp_getentry(ht, ep);
	if (e == NULL) {
		vtp0_t empty = {0};
		htendp_insert(ht, ep, empty);
		e = htendp_getentry(ht, ep);
	}
	else {
		long n;
		for(n = 0; n < e->value.used; n++)
			if (e->value.array[n] == a)
				return;
	}

	vtp0_append(&e->value, a);
}

void hpgl_add_arc(htendp_t *ht, pcb_arc_t *a, pcb_dynf_t dflg)
{
	rnd_cheap_point_t ep;

	PCB_DFLAG_SET(&a->Flags, dflg);

	pcb_arc_get_end(a, 0, &ep.X, &ep.Y);
	hpgl_add_endp(ht, ep, a);

	pcb_arc_get_end(a, 1, &ep.X, &ep.Y);
	hpgl_add_endp(ht, ep, a);
}

void hpgl_add_line(htendp_t *ht, pcb_line_t *l, pcb_dynf_t dflg)
{
	rnd_cheap_point_t ep;

	PCB_DFLAG_SET(&l->Flags, dflg);

	ep.X = l->Point1.X; ep.Y = l->Point1.Y;
	hpgl_add_endp(ht, ep, l);

	ep.X = l->Point2.X; ep.Y = l->Point2.Y;
	hpgl_add_endp(ht, ep, l);
}

void hpgl_endp_init(htendp_t *ht)
{
	htendp_init(ht, endphash, endpkeyeq);
}

void hpgl_endp_uninit(htendp_t *ht)
{
	htendp_entry_t *e;

	for(e = htendp_first(ht); e != NULL; e = htendp_next(ht, e))
		vtp0_uninit(&e->value);

	htendp_uninit(ht);
}

/*** render ***/

/* Assume pt is one of the endpoints of obj; figure which side of obj pt is
   on and set pt_idx and return the other end */
RND_INLINE rnd_cheap_point_t other_end(pcb_any_obj_t *obj, rnd_cheap_point_t pt, int *pt_idx)
{
	rnd_cheap_point_t ep0, ep1;
	pcb_line_t *l = (pcb_line_t *)obj;
	pcb_arc_t *a = (pcb_arc_t *)obj;

	switch(obj->type) {
		case PCB_OBJ_ARC:
			pcb_arc_get_end(a, 0, &ep0.X, &ep0.Y);
			pcb_arc_get_end(a, 1, &ep1.X, &ep1.Y);
			break;
		case PCB_OBJ_LINE:
			ep0.X = l->Point1.X; ep0.Y = l->Point1.Y;
			ep1.X = l->Point1.X; ep1.Y = l->Point1.Y;
			break;
		default:
			abort();
	}

	if ((ep0.X == pt.X) && (ep0.Y == pt.Y)) {
		*pt_idx = 0;
		return ep1;
	}

	if ((ep1.X == pt.X) && (ep1.Y == pt.Y)) {
		*pt_idx = 1;
		return ep0;
	}

	abort();
}

RND_INLINE double obj_len2(pcb_any_obj_t *o)
{
	double dx, dy;
	pcb_line_t *l = (pcb_line_t *)o;
	pcb_arc_t *a = (pcb_arc_t *)o;

	switch(o->type) {
		case PCB_OBJ_ARC:
			dx = pcb_arc_length(a);
			return dx*dx;
		case PCB_OBJ_LINE:
			dx = l->Point2.X - l->Point1.X;
			dy = l->Point2.Y - l->Point1.Y;
			return dx*dx + dy*dy;
		default:
			return 0;
	}

	return 0;
}

/* Return the longest unrendered obj from pt or NULL */
RND_INLINE pcb_any_obj_t *longest_obj_at(htendp_t *ht, rnd_cheap_point_t pt, pcb_dynf_t dflg)
{
	htendp_entry_t *e = htendp_getentry(ht, pt);
	long n;
	double best_len = 0;
	pcb_any_obj_t *best = NULL;

	for(n = 0; n < e->value.used; n++) {
		pcb_any_obj_t *o = e->value.array[n];
		if (PCB_DFLAG_TEST(&o->Flags, dflg)) {
			double len = obj_len2(o);
			if ((best == NULL) || (len > best_len)) {
				best = o;
				best_len = len;
			}
		}
	}

	return best;
}

static void render_from(htendp_t *ht, rnd_cheap_point_t pt, pcb_any_obj_t *obj, void (*cb)(void *uctx, pcb_any_obj_t *o, endp_state_t st), void *uctx, pcb_dynf_t dflg, int first)
{
	rnd_cheap_point_t next_pt;
	pcb_any_obj_t *next_obj;

	while(obj != NULL) {
		int pt_idx;
		endp_state_t st = first ? ENDP_ST_START : 0;

		PCB_DFLAG_CLR(&obj->Flags, dflg);
		first = 0;

		next_pt = other_end(obj, pt, &pt_idx);
		next_obj = longest_obj_at(ht, pt, dflg);

		if (next_obj == NULL)
			st |= ENDP_ST_END;

		if (pt_idx != 0)
			st |= ENDP_ST_REVERSE;

		cb(uctx, obj, st);

		obj = next_obj;
		pt = next_pt;
	}
}

static void render_loop(htendp_t *ht, void (*cb)(void *uctx, pcb_any_obj_t *o, endp_state_t st), void *uctx, pcb_dynf_t dflg, int when_1)
{
	htendp_entry_t *e;

	for(e = htendp_first(ht); e != NULL; e = htendp_next(ht, e)) {
	long n;
		vtp0_t *v = &e->value;

		if (when_1 && (v->used != 1))
			continue;

		for(n = 0; n < v->used; n++) {
			pcb_any_obj_t *o = v->array[n];
			if (PCB_DFLAG_TEST(&o->Flags, dflg))
				render_from(ht, e->key, o, cb, uctx, dflg, 1);
		}
	}
}


void hpgl_endp_render(htendp_t *ht, void (*cb)(void *uctx, pcb_any_obj_t *o, endp_state_t st), void *uctx, pcb_dynf_t dflg)
{
	/* first start rendering from single-object endpoints to avoid rendering from
	   the middle of a sequence of lines */
	render_loop(ht, cb, uctx, dflg, 1);

	/* render from anywhere else not covered so far */
	render_loop(ht, cb, uctx, dflg, 0);
}

