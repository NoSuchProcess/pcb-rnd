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

#include <assert.h>

#include "board.h"
#include "data.h"
#include "data_list.h"
#include "obj_padstack.h"
#include "obj_padstack_list.h"

pcb_pad_t *pcb_padstack_alloc(pcb_data_t *data)
{
	pcb_padstack_t *ps;

	ps = calloc(sizeof(pcb_padstack_t), 1);
	ps->type = PCB_OBJ_PADSTACK;
	ps->Attributes.post_change = pcb_obj_attrib_post_change;
	PCB_SET_PARENT(ps, data, data);

	padstacklist_append(&data->padstack, ps);

	return ps;
}

void pcb_padstack_free(pcb_padstack_t *ps)
{
	padstacklist_remove(ps);
	free(ps);
}

pcb_pad_t *pcb_padstack_new(pcb_data_t *data, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_flag_t Flags)
{
	pcb_padstack_t *ps;

	if (proto >= pcb_vtpadstack_proto_len(&data->ps_protos))
		return NULL;

	ps = pcb_padstack_alloc(data);

	/* copy values */
	ps->proto = proto;
	ps->x = x;
	ps->y = y;
	ps->Flags = Flags;
	ps->ID = pcb_create_ID_get();
	PCB_SET_PARENT(ps, data, data);
	return ps;
}

