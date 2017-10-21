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
#include "conf_core.h"
#include "data.h"
#include "data_list.h"
#include "draw.h"
#include "flag.h"
#include "obj_padstack.h"
#include "obj_padstack_draw.h"
#include "obj_padstack_list.h"
#include "obj_padstack_inlines.h"
#include "vtpadstack.h"

#define PS_CROSS_SIZE PCB_MM_TO_COORD(1)

pcb_padstack_t *pcb_padstack_alloc(pcb_data_t *data)
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

pcb_padstack_t *pcb_padstack_new(pcb_data_t *data, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_flag_t Flags)
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
	pcb_padstack_add(data, ps);
	return ps;
}

void pcb_padstack_add(pcb_data_t *data, pcb_padstack_t *ps)
{
	pcb_padstack_bbox(ps);
	if (!data->padstack_tree)
		data->padstack_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps, 0);
	PCB_SET_PARENT(ps, data, data);
}

void pcb_padstack_bbox(pcb_padstack_t *ps)
{
	int n, sn;
	pcb_line_t line;
	pcb_padstack_proto_t *proto = pcb_padstack_get_proto(ps);
	assert(proto != NULL);

	ps->BoundingBox.X1 = ps->BoundingBox.X2 = ps->x;
	ps->BoundingBox.Y1 = ps->BoundingBox.Y2 = ps->y;

	for(sn = 0; sn < proto->len; sn++) {
		pcb_padstack_shape_t *shape = proto->shape + sn;
		switch(shape->shape) {
			case PCB_PSSH_POLY:
				for(n = 0; n < shape->data.poly.len; n++)
					pcb_box_bump_point(&ps->BoundingBox, shape->data.poly.pt[n].X, shape->data.poly.pt[n].Y);
				break;
			case PCB_PSSH_LINE:
				line.Point1.X = shape->data.line.x1;
				line.Point1.Y = shape->data.line.y1;
				line.Point2.X = shape->data.line.x2;
				line.Point2.Y = shape->data.line.y2;
				line.Thickness = shape->data.line.thickness;
				line.Clearance = 0;
				line.Flags = pcb_flag_make(shape->data.line.square ? PCB_FLAG_SQUARE : 0);
				pcb_line_bbox(&line);
				pcb_box_bump_box(&ps->BoundingBox, &line.BoundingBox);
				break;
			case PCB_PSSH_CIRC:
				pcb_box_bump_point(&ps->BoundingBox, ps->x - shape->data.circ.dia/2, ps->y - shape->data.circ.dia/2);
				pcb_box_bump_point(&ps->BoundingBox, ps->x + shape->data.circ.dia/2, ps->y + shape->data.circ.dia/2);
				break;
		}
	}

	pcb_close_box(&ps->BoundingBox);
}

/*** draw ***/

pcb_r_dir_t pcb_padstack_draw_callback(const pcb_box_t *b, void *cl)
{
	pcb_padstack_draw_t *ctx = cl;
	pcb_padstack_t *ps = (pcb_padstack_t *)b;

printf("DRAW %ld!\n", (long int)ctx->gid);

#warning padstack TODO: have an own color instead of subc_*
	pcb_gui->set_color(Output.fgGC, PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps) ? conf_core.appearance.color.subc_selected : conf_core.appearance.color.subc);
	pcb_gui->set_line_width(Output.fgGC, 0);

	pcb_gui->set_draw_xor(Output.fgGC, 1);
	pcb_gui->draw_line(Output.fgGC, ps->x-PS_CROSS_SIZE/2, ps->y, ps->x+PS_CROSS_SIZE/2, ps->y);
	pcb_gui->draw_line(Output.fgGC, ps->x, ps->y-PS_CROSS_SIZE/2, ps->x, ps->y+PS_CROSS_SIZE/2);
	pcb_gui->set_draw_xor(Output.fgGC, 0);

}
