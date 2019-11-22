/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
#include "polygon_selfi.h"

/* expect no more than this number of hubs in a complex polygon */
#define MAX_HUB_TRIES 1024

#if 1
#	define TRACE pcb_trace
#else
	void TRACE(const char *fmt, ...) {}
#endif

typedef struct {
	vtp0_t node;
} vhub_t;

static pcb_vnode_t *pcb_pline_split(pcb_vnode_t *v, pcb_vector_t at)
{
	pcb_vnode_t *v2 = node_add_single(v, at);

	v2 = pcb_poly_node_create(at);
	assert(v2 != NULL);
	v2->cvc_prev = v2->cvc_next = NULL;
	v2->Flags.status = UNKNWN;

	/* link in */
	v->next->prev = v2;
	v2->next = v->next;
	v2->prev = v;
	v->next = v2;
	
	return v2;
}

static pcb_vnode_t *pcb_pline_end_at(pcb_vnode_t *v, pcb_vector_t at)
{
	if (pcb_vect_dist2(at, v->point) < ENDP_EPSILON)
		return v;
	if (pcb_vect_dist2(at, v->next->point) < ENDP_EPSILON)
		return v->next;
	return NULL;
}

static vhub_t *hub_find(vtp0_t *hubs, pcb_vnode_t *v, pcb_bool insert)
{
	int n, m;

	for(n = 0; n < vtp0_len(hubs); n++) {
		vhub_t *h = *vtp0_get(hubs, n, 0);
		pcb_vnode_t *stored, **st;
		st = vtp0_get(&h->node, 0, 0);
		if (st == NULL) continue;
		stored = *st;
		/* found the hub at the specific location */
		if (pcb_vect_dist2(stored->point, v->point) < ENDP_EPSILON) {
			for(m = 0; m < vtp0_len(&h->node); m++) {
				st = vtp0_get(&h->node, m, 0);
				if (st != NULL) {
					stored = *st;
					if (stored == v) /* already on the list */
						return h;
				}
			}
			/* not found on the hub's list, maybe insert */
			if (insert) {
				vtp0_append(&h->node, v);
				return h;
			}
			return NULL; /* there is only one hub per point - if the coord matched and the node is not on, it's not going to be on any other hub's list */
		}
	}
	return NULL;
}

/* remove v from h; if h has only one node, remove that too */
static void remove_from_hub(vhub_t *h, pcb_vnode_t *v)
{
	int m;
	pcb_vnode_t *stored, **st;

	for(m = 0; m < vtp0_len(&h->node); m++) {
		st = vtp0_get(&h->node, m, 0);
		if (st == NULL) {
			vtp0_remove(&h->node, m, 1);
			m--;
			continue;
		}
		stored = *st;
		if (stored == v) {
			vtp0_remove(&h->node, m, 1);
			m--;
			v->Flags.in_hub = 0;
		}
	}
	
	/* only one node remains: this is no longer a hub! */
	if (h->node.used == 1) {
		stored = *vtp0_get(&h->node, 0, 0); /* the previous loop guarantees it can not be NULL */
		vtp0_remove(&h->node, 0, 1);
		stored->Flags.in_hub = 0;
	}
}

/* returns 1 if a new intersection is mapped */
static int pcb_pline_add_isectp(vtp0_t *hubs, pcb_vnode_t *v)
{
	vhub_t *h;

	assert(v != NULL);

	v->Flags.in_hub = 1;
	if (hubs->array != NULL) {
		if (hub_find(hubs, v, pcb_true) != NULL) /* already on the list (... by now) */
				return 0;
	}

	h = malloc(sizeof(vhub_t));
	vtp0_append(hubs, h);
	vtp0_init(&h->node);
	vtp0_append(&h->node, v);
	return 1;
}

static int pline_split_off_loop(vtp0_t *hubs, vtp0_t *out, vhub_t *h, pcb_vnode_t *start)
{
	pcb_vnode_t *v, *next, *tmp;
	pcb_pline_t *newpl = NULL;
	pcb_cardinal_t cnt;
	vhub_t *endh;

	for(v = start->next, cnt = 0;; v = v->next) {
		if (v->Flags.in_hub) {
			if (pcb_vect_dist2(start->point, v->point) < ENDP_EPSILON)
				break; /* found a matching hub point */
			goto skip_to_backward; /* found a different hub point, skip */
		}
		cnt++;
	}
	
	/* forward loop */
	TRACE("  Fwd %ld!\n", (long)cnt);
	newpl = pcb_poly_contour_new(start->point);
	next = start->next;
	remove_from_hub(h, start);
	pcb_poly_vertex_exclude(start);
	for(v = next; cnt > 0; v = next, cnt--) {
		TRACE("   Append %mm %mm!\n", v->point[0], v->point[1]);
		next = v->next;
		pcb_poly_vertex_exclude(v);
		tmp = pcb_poly_node_create(v->point);
		pcb_poly_vertex_include(newpl->head.prev, tmp);
	}
	goto new_pl;

	skip_to_backward:;
	for(v = start->prev, cnt = 0;; v = v->prev) {
		if (v->Flags.in_hub) {
			if (pcb_vect_dist2(start->point, v->point) < ENDP_EPSILON)
				break; /* found a matching hub point */
			return 0; /* found a different hub point, skip */
		}
		cnt++;
	}
	if (h == endh) { /* backward loop */
		TRACE("  Bwd %ld!\n", (long)cnt);
		newpl = pcb_poly_contour_new(start->point);
		next = start->prev;
		newpl->head = *start;
		remove_from_hub(h, start);
		pcb_poly_vertex_exclude(start);
		for(v = next; cnt > 0; v = next, cnt--) {
			TRACE("   Append!\n");
			next = v->prev;
			pcb_poly_vertex_exclude(v);
			tmp = pcb_poly_node_create(v->point);
			pcb_poly_vertex_include(start, tmp);
		}
		goto new_pl;
	}
	return 0;

	new_pl:;
TRACE("APPEND: %p %p\n", newpl, newpl->head.next);
	vtp0_append(out, newpl);
	return 1;
}

pcb_bool pcb_pline_is_selfint(pcb_pline_t *pl)
{
	pcb_vnode_t *va, *vb;

	va = (pcb_vnode_t *)&pl->head;
	do {
		for(vb = va->next->next; vb->next != va; vb = vb->next) {
			pcb_vector_t i, tmp;
			if (pcb_vect_inters2(va->point, va->next->point, vb->point, vb->next->point, i, tmp) > 0)
				return pcb_true;
		}
		va = va->next;
	} while (va != &pl->head);
	return pcb_false;
}


void pcb_pline_split_selfint(pcb_pline_t *pl, vtp0_t *out)
{
	int n;
	vtp0_t hubs;
	pcb_vnode_t *va, *vb, *iva, *ivb;

	vtp0_init(&hubs);

	/* reset the in_hub flag */
	va = (pcb_vnode_t *)&pl->head;
	do {
		va->Flags.in_hub = 0;
		va = va->next;
	} while (va != &pl->head);

	/* insert corners at intersections, collect a list of intersection hubs */
	va = (pcb_vnode_t *)&pl->head;
	do {
		for(vb = va->next->next; vb->next != va; vb = vb->next) {
			pcb_vector_t i, tmp;
			if (pcb_vect_inters2(va->point, va->next->point, vb->point, vb->next->point, i, tmp) > 0) {
				iva = pcb_pline_end_at(va, i);
				if (iva == NULL)
					iva = pcb_pline_split(va, i);
				ivb = pcb_pline_end_at(vb, i);
				if (ivb == NULL)
					ivb = pcb_pline_split(vb, i);
				if (pcb_pline_add_isectp(&hubs, iva) || pcb_pline_add_isectp(&hubs, ivb))
					TRACE("Intersect at %mm;%mm\n", i[0], i[1]);
			}
		}
		va = va->next;
	} while (va != &pl->head);

	/*for(t = MAX_HUB_TRIES; t > 0; t--)*/ {
		retry:;
		for(n = 0; n < vtp0_len(&hubs); n++) {
			int m;
			vhub_t *h = *vtp0_get(&hubs, n, 0);
			pcb_vnode_t *v, **v_ = vtp0_get(&h->node, 0, 0);
			if (v_ == NULL) continue;
			v = *v_;
			TRACE("hub %p at %mm;%mm:\n", h, v->point[0], v->point[1]);
			for(m = 0; m < vtp0_len(&h->node); m++) {
				v = *vtp0_get(&h->node, m, 0);
				TRACE(" %d=%p\n", m, v);
				/* try to split off a leaf loop from the hub */
				
				if (pline_split_off_loop(&hubs, out, h, v))
					goto retry;
			}
			TRACE("\n");
		}
	}

TODO("leak: remove the unused hubs");
	vtp0_uninit(&hubs);
}
