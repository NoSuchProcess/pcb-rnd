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

pcb_text_t *pcb_subc_add_dyntex(pcb_subc_t *sc, pcb_coord_t x, pcb_coord_t y, unsigned direction, int scale, pcb_bool bottom, const char *pattern)
{
	pcb_layer_type_t side = bottom ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
	pcb_layer_t *ly = pcb_subc_get_layer(sc, side | PCB_LYT_SILK, 0, pcb_true, "top-silk", pcb_false);
	if (ly != NULL)
		return pcb_text_new(ly, pcb_font(PCB, 0, 0), x, y, 90.0 * direction, scale, 0, pattern, pcb_flag_make(PCB_FLAG_DYNTEXT | PCB_FLAG_FLOATER | (bottom ? PCB_FLAG_ONSOLDER : 0)));
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

