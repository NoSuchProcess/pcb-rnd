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

#include "subc_help.h"

pcb_layer_t *pcb_subc_get_layer(pcb_subc_t *sc, pcb_layer_type_t lyt, pcb_layer_combining_t comb, pcb_bool_t alloc, const char *name, pcb_bool req_name_match)
{
	int n;

	/* look for an existing layer with matching lyt and comb first */
	for(n = 0; n < sc->data->LayerN; n++) {
		if (sc->data->Layer[n].meta.bound.type != lyt)
			continue;
		if ((comb != -1) && (sc->data->Layer[n].comb != comb))
			continue;
		if (req_name_match && (strcmp(sc->data->Layer[n].name, name) != 0))
			continue;
		return &sc->data->Layer[n];
	}

	if (!alloc)
		return NULL;

	if (sc->data->LayerN == PCB_MAX_LAYER)
		return NULL;

	n = sc->data->LayerN++;
	if (name == NULL)
		name = "";

	if (comb == -1) {
		/* "any" means default, for the given layer type */
		if (lyt & PCB_LYT_MASK)
			comb = PCB_LYC_SUB;
		else
			comb = 0; /* positive, manual */
	}

	memset(&sc->data->Layer[n], 0, sizeof(sc->data->Layer[n]));
	sc->data->Layer[n].name = pcb_strdup(name);
	sc->data->Layer[n].comb = comb;
	sc->data->Layer[n].is_bound = 1;
	sc->data->Layer[n].meta.bound.type = lyt;
	sc->data->Layer[n].parent.data = sc->data;
	sc->data->Layer[n].parent_type = PCB_PARENT_DATA;
	sc->data->Layer[n].type = PCB_OBJ_LAYER;
	return &sc->data->Layer[n];
}

pcb_text_t *pcb_subc_add_dyntex(pcb_subc_t *sc, pcb_coord_t x, pcb_coord_t y, unsigned direction, int scale, pcb_bool bottom, const char *pattern)
{
	pcb_layer_type_t side = bottom ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
	pcb_layer_t *ly = pcb_subc_get_layer(sc, side | PCB_LYT_SILK, 0, pcb_true, "top-silk", pcb_false);
	if (ly != NULL)
		return pcb_text_new(ly, pcb_font(PCB, 0, 0), x, y, direction, scale, pattern, pcb_flag_make(PCB_FLAG_DYNTEXT | PCB_FLAG_FLOATER | (bottom ? PCB_FLAG_ONSOLDER : 0)));
	return 0;
}

pcb_text_t *pcb_subc_add_refdes_text(pcb_subc_t *sc, pcb_coord_t x, pcb_coord_t y, unsigned direction, int scale, pcb_bool bottom)
{
	return pcb_subc_add_dyntex(sc, x, y, direction, scale, bottom, "%a.parent.refdes%");
}

pcb_text_t *pcb_subc_get_refdes_text(pcb_subc_t *sc)
{
	int l, score, best_score = 0;
	pcb_text_t *best = NULL;
	for(l = 0; l < sc->data->LayerN; l++) {
		pcb_layer_t *ly = &sc->data->Layer[l];
		pcb_text_t *text;
		gdl_iterator_t it;
		textlist_foreach(&ly->Text, &it, text) {
			if (!PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, text))
				continue;
			if (!strstr(text->TextString, "%a.parent.refdes%"))
				continue;
			score = 1;
			if (ly->meta.bound.type & PCB_LYT_SILK) score += 5;
			if (ly->meta.bound.type & PCB_LYT_TOP)  score += 2;
			if (score > best_score) {
				best = text;
				best_score = score;
			}
		}
	}

	return best;
}

