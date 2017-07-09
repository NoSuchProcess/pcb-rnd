/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "toolpath.h"

#include "board.h"
#include "data.h"
#include "flag.h"
#include "layer.h"
#include "layer_ui.h"
#include "polygon.h"


extern const char *pcb_millpath_cookie;

int pcb_tlp_mill_copper_layer(pcb_tlp_session_t *result, pcb_layer_t *layer)
{
	pcb_board_t *pcb = pcb_data_get_top(layer->parent);
	if (result->lres == NULL)
		result->lres = pcb_uilayer_alloc(pcb_millpath_cookie, "postmill copper", "#EE9922");

	if (result->fill != NULL)
		pcb_poly_remove(result->lres, result->fill);

	pcb_poly_new_from_rectangle(result->lres, 0, 0, pcb->MaxWidth, pcb->MaxHeight, pcb_flag_make(PCB_FLAG_FULLPOLY));

	return 0;
}

