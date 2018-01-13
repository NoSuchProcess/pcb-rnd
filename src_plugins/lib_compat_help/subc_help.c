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

#include "subc_help.h"

pcb_layer_t *pcb_subc_get_layer(pcb_subc_t *sc, pcb_layer_type_t lyt, pcb_layer_combining_t comb, pcb_bool_t alloc, const char *name)
{
	int n;

	/* look for an existing layer with matching lyt and comb first */
	for(n = 0; n < sc->data->LayerN; n++) {
		if (sc->data->Layer[n].meta.bound.type != lyt)
			continue;
		if ((comb != -1) && (sc->data->Layer[n].comb != comb))
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

	memset(&sc->data->Layer[n], 0, sizeof(sc->data->Layer[n]));
	sc->data->Layer[n].name = pcb_strdup(name);
	sc->data->Layer[n].comb = comb;
	sc->data->Layer[n].is_bound = 1;
	sc->data->Layer[n].meta.bound.type = lyt;
	sc->data->Layer[n].parent = sc->data;
	return &sc->data->Layer[n];
}

pcb_text_t *pcb_subc_add_refdes_text(pcb_subc_t *sc, pcb_coord_t x, pcb_coord_t y, unsigned direction, int scale)
{
	pcb_layer_t *ly = pcb_subc_get_layer(sc, PCB_LYT_TOP | PCB_LYT_SILK, 0, pcb_true, "top-silk");
	if (ly != NULL)
		return pcb_text_new(ly, pcb_font(PCB, 0, 0), x, y, direction, scale, "%a.parent.refdes%", pcb_flag_make(PCB_FLAG_DYNTEXT | PCB_FLAG_FLOATER));
	return 0;
}
