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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
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

pcb_bool_t pcb_subc_eq(const pcb_subc_t *sc1, const pcb_subc_t *sc2)
{
	return pcb_false;
}

unsigned int pcb_subc_hash(const pcb_subc_t *sc)
{
	unsigned int hash;
	int lid;
	pcb_host_trans_t tr;
	gdl_iterator_t it;

	pcb_subc_get_host_trans(sc, &tr);

	/* hash layers and layer objects */
	hash = sc->data->LayerN;
	for(lid = 0; lid < sc->data->LayerN; lid++) {
		pcb_layer_t *ly = &sc->data->Layer[lid];

		pcb_line_t *l;
		pcb_arc_t *a;
		pcb_text_t *t;
		pcb_poly_t *p;

		hash ^= pcb_layer_hash_bound(ly, tr.on_bottom);

		linelist_foreach(&ly->Arc, &it, a)
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
#warning subc TODO: subc in subc: trans in trans
#if 0
		pcb_subc_t *s;
		polylist_foreach(&sc->data->subc, &it, s)
			hash ^= pcb_subc_hash_(&tr, p);
#endif
		padstacklist_foreach(&sc->data->padstack, &it, ps)
			hash ^= pcb_pstk_hash(&tr, ps);
	}
	return hash;
}

