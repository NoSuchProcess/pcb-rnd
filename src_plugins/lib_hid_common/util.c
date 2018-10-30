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

#include "util.h"
#include "data_it.h"
#include "flag.h"

static pcb_cardinal_t pcb_get_bbox_by_flag(pcb_box_t *dst, const pcb_data_t *data, pcb_flag_values_t flg)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	pcb_cardinal_t cnt = 0;

	dst->X1 = dst->Y1 = PCB_MAX_COORD;
	dst->X2 = dst->Y2 = -PCB_MAX_COORD;

	for(o = pcb_data_first(&it, (pcb_data_t *)data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		if (!PCB_FLAG_TEST(flg, o))
			continue;
		cnt++;
		pcb_box_bump_box(dst, &o->BoundingBox);
	}
	return cnt;
}

pcb_cardinal_t pcb_get_selection_bbox(pcb_box_t *dst, const pcb_data_t *data)
{
	return pcb_get_bbox_by_flag(dst, data, PCB_FLAG_SELECTED);
}

pcb_cardinal_t pcb_get_found_bbox(pcb_box_t *dst, const pcb_data_t *data)
{
	return pcb_get_bbox_by_flag(dst, data, PCB_FLAG_FOUND);
}
