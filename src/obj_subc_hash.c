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

#include "data.h"
#include "layer.h"
#include "obj_subc.h"
#include "obj_arc.h"
#include "obj_line.h"
#include "obj_text.h"
#include "obj_poly.h"
#include "obj_pstk.h"

int pcb_subc_hash_ignore_uid = 0;

int pcb_subc_eq(pcb_subc_t *sc1, pcb_subc_t *sc2)
{
	pcb_host_trans_t tr1, tr2;
	int lid;
	gdl_iterator_t it;

	/* optimization: cheap tests first */
	if (!pcb_subc_hash_ignore_uid && memcmp(&sc1->uid, &sc2->uid, sizeof(sc1->uid)) != 0) return 0;
	if (sc1->data->LayerN != sc2->data->LayerN) return 0;
	if (padstacklist_length(&sc1->data->padstack) != padstacklist_length(&sc2->data->padstack)) return 0;
	if (pcb_subclist_length(&sc1->data->subc) != pcb_subclist_length(&sc2->data->subc)) return 0;

	for(lid = 0; lid < sc1->data->LayerN; lid++) {
		pcb_layer_t *ly1 = &sc1->data->Layer[lid];
		pcb_layer_t *ly2 = &sc2->data->Layer[lid];
TODO("todo")
/*		if (!pcb_layer_eq_bound(ly1, tr1.on_bottom, ly2, tr2.on_bottom)) return 0;*/
		if (arclist_length(&ly1->Arc) != arclist_length(&ly2->Arc)) return 0;
	}

	pcb_subc_get_host_trans(sc1, &tr1, 1);
	pcb_subc_get_host_trans(sc2, &tr2, 1);

	for(lid = 0; lid < sc1->data->LayerN; lid++) {
		pcb_line_t *l1, *l2;
		pcb_arc_t *a1, *a2;
		pcb_text_t *t1, *t2;
		pcb_poly_t *p1, *p2;
		pcb_layer_t *ly1 = &sc1->data->Layer[lid];
		pcb_layer_t *ly2 = &sc2->data->Layer[lid];


		/* assume order of children objects are the same */
		a2 = arclist_first(&ly2->Arc);
		arclist_foreach(&ly1->Arc, &it, a1) {
			if (!pcb_arc_eq(&tr1, a1, &tr2, a2)) return 0;
			a2 = arclist_next(a2);
		}

		l2 = linelist_first(&ly2->Line);
		linelist_foreach(&ly1->Line, &it, l1) {
			if (!pcb_line_eq(&tr1, l1, &tr2, l2)) return 0;
			l2 = linelist_next(l2);
		}

		t2 = textlist_first(&ly2->Text);
		textlist_foreach(&ly1->Text, &it, t1) {
			if (!pcb_text_eq(&tr1, t1, &tr2, t2)) return 0;
			t2 = textlist_next(t2);
		}

		p2 = polylist_first(&ly2->Polygon);
		polylist_foreach(&ly1->Polygon, &it, p1) {
			if (!pcb_poly_eq(&tr1, p1, &tr2, p2)) return 0;
			p2 = polylist_next(p2);
		}
	}

	/* compare global objects */
	{
		pcb_pstk_t *p1, *p2;

		p2 = padstacklist_first(&sc2->data->padstack);
		padstacklist_foreach(&sc1->data->padstack, &it, p1) {
			if (!pcb_pstk_eq(&tr1, p1, &tr2, p2)) return 0;
			p2 = padstacklist_next(p2);
		}
TODO("subc: subc-in-subc eq check")
	}

	return 1;
}

unsigned int pcb_subc_hash(pcb_subc_t *sc)
{
	unsigned int hash;
	int lid;
	pcb_host_trans_t tr;
	gdl_iterator_t it;

	pcb_subc_get_host_trans(sc, &tr, 1);

	hash = sc->data->LayerN;

	if (!pcb_subc_hash_ignore_uid)
		hash ^= murmurhash(&sc->uid, sizeof(sc->uid));

	/* hash layers and layer objects */

	for(lid = 0; lid < sc->data->LayerN; lid++) {
		pcb_layer_t *ly = &sc->data->Layer[lid];

		pcb_line_t *l;
		pcb_arc_t *a;
		pcb_text_t *t;
		pcb_poly_t *p;

		hash ^= pcb_layer_hash_bound(ly, tr.on_bottom);

		arclist_foreach(&ly->Arc, &it, a)
			hash ^= pcb_arc_hash(&tr, a);

		linelist_foreach(&ly->Line, &it, l)
			hash ^= pcb_line_hash(&tr, l);

		textlist_foreach(&ly->Text, &it, t)
			hash ^= pcb_text_hash(&tr, t);

		polylist_foreach(&ly->Polygon, &it, p)
			hash ^= pcb_poly_hash(&tr, p);
	}

	/* hash global objects */
	{
		pcb_pstk_t *ps;
TODO("subc: subc in subc: trans in trans")
#if 0
		pcb_subc_t *s;
		subclist_foreach(&sc->data->subc, &it, s)
			hash ^= pcb_subc_hash_(&tr, p);
#endif
		padstacklist_foreach(&sc->data->padstack, &it, ps)
			hash ^= pcb_pstk_hash(&tr, ps);
	}
	return hash;
}

