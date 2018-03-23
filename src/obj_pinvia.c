/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */

/* Drawing primitive: pins and vias */

#include "config.h"
#include "board.h"
#include "data.h"
#include "undo.h"
#include "conf_core.h"
#include "polygon.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "stub_vendor.h"
#include "rotate.h"

#include "obj_pinvia.h"
#include "obj_pinvia_op.h"
#include "obj_subc_parent.h"
#include "obj_hash.h"

/* TODO: consider removing this by moving pin/via functions here: */
#include "draw.h"
#include "obj_text_draw.h"
#include "obj_pinvia_draw.h"

#include "obj_elem.h"

/*** utility ***/

int pcb_pin_eq(const pcb_element_t *e1, const pcb_pin_t *p1, const pcb_element_t *e2, const pcb_pin_t *p2)
{
	if (pcb_field_neq(p1, p2, Thickness) || pcb_field_neq(p1, p2, Clearance)) return 0;
	if (pcb_field_neq(p1, p2, Mask) || pcb_field_neq(p1, p2, DrillingHole)) return 0;
	if (pcb_element_neq_offsx(e1, p1, e2, p2, X) || pcb_element_neq_offsy(e1, p1, e2, p2, Y)) return 0;
	if (pcb_neqs(p1->Name, p2->Name)) return 0;
	if (pcb_neqs(p1->Number, p2->Number)) return 0;
	return 1;
}

int pcb_pin_eq_padstack(const pcb_pin_t *p1, const pcb_pin_t *p2)
{
	if (pcb_field_neq(p1, p2, Thickness) || pcb_field_neq(p1, p2, Clearance)) return 0;
	if (pcb_field_neq(p1, p2, Mask) || pcb_field_neq(p1, p2, DrillingHole)) return 0;
	return 1;
}

unsigned int pcb_pin_hash(const pcb_element_t *e, const pcb_pin_t *p)
{
	return
		pcb_hash_coord(p->Thickness) ^ pcb_hash_coord(p->Clearance) ^
		pcb_hash_coord(p->Mask) ^ pcb_hash_coord(p->DrillingHole) ^
		pcb_hash_element_ox(e, p->X) ^ pcb_hash_element_oy(e, p->Y) ^
		pcb_hash_str(p->Name) ^ pcb_hash_str(p->Number);
}

unsigned int pcb_pin_hash_padstack(const pcb_pin_t *p)
{
	return
		pcb_hash_coord(p->Thickness) ^ pcb_hash_coord(p->Clearance) ^
		pcb_hash_coord(p->Mask) ^ pcb_hash_coord(p->DrillingHole);
}

/*** draw ***/
static void GatherPVName(pcb_pin_t *Ptr)
{
	pcb_box_t box;
	pcb_bool vert = PCB_FLAG_TEST(PCB_FLAG_EDGE2, Ptr);

	if (vert) {
		box.X1 = Ptr->X - Ptr->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		box.Y1 = Ptr->Y - Ptr->DrillingHole / 2 - conf_core.appearance.pinout.text_offset_x;
	}
	else {
		box.X1 = Ptr->X + Ptr->DrillingHole / 2 + conf_core.appearance.pinout.text_offset_x;
		box.Y1 = Ptr->Y - Ptr->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
	}

	if (vert) {
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	else {
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	pcb_draw_invalidate(&box);
}

void pcb_via_invalidate_erase(pcb_pin_t *Via)
{
	pcb_draw_invalidate(Via);
	if (PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Via))
		pcb_via_name_invalidate_erase(Via);
	pcb_flag_erase(&Via->Flags);
}

void pcb_via_name_invalidate_erase(pcb_pin_t *Via)
{
	GatherPVName(Via);
}

void pcb_pin_invalidate_erase(pcb_pin_t *Pin)
{
	pcb_draw_invalidate(Pin);
	if (PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Pin))
		pcb_pin_name_invalidate_erase(Pin);
	pcb_flag_erase(&Pin->Flags);
}

void pcb_pin_name_invalidate_erase(pcb_pin_t *Pin)
{
	GatherPVName(Pin);
}

void pcb_via_invalidate_draw(pcb_pin_t *Via)
{
	pcb_draw_invalidate(Via);
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) && PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Via))
		pcb_via_name_invalidate_draw(Via);
}

void pcb_via_name_invalidate_draw(pcb_pin_t *Via)
{
	GatherPVName(Via);
}

void pcb_pin_invalidate_draw(pcb_pin_t *Pin)
{
	pcb_draw_invalidate(Pin);
	if ((!PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) && PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Pin))
			|| pcb_draw_doing_pinout)
		pcb_pin_name_invalidate_draw(Pin);
}

void pcb_pin_name_invalidate_draw(pcb_pin_t *Pin)
{
	GatherPVName(Pin);
}
