/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  delayed creation of objects
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2021)
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

#include "delay_clearance.h"
#include "data.h"
#include "data_it.h"
#include "draw.h"
#include "change.h"
#include "obj_pstk_inlines.h"

void pcb_dlcl_apply_(pcb_board_t *pcb, rnd_coord_t clr[PCB_DLCL_max])
{
	pcb_objtype_t mask = 0;
	pcb_data_it_t it;
	pcb_any_obj_t *o;

	if (clr[PCB_DLCL_TRACE] > 0)    mask |= PCB_OBJ_ARC | PCB_OBJ_LINE;
	if (clr[PCB_DLCL_SMD_TERM] > 0) mask |= PCB_OBJ_PSTK;
	if (clr[PCB_DLCL_TRH_TERM] > 0) mask |= PCB_OBJ_PSTK;
	if (clr[PCB_DLCL_VIA] > 0)      mask |= PCB_OBJ_PSTK;
	if (clr[PCB_DLCL_POLY] > 0)     mask |= PCB_OBJ_POLY;


	if (mask == 0)
		return;

	for(o = pcb_data_first(&it, pcb->Data, mask); o != NULL; o = pcb_data_next(&it)) {
		switch(o->type) {
			case PCB_OBJ_ARC:
			case PCB_OBJ_LINE:
				pcb_chg_obj_clear_size(o->type, o->parent.layer, o, NULL, clr[PCB_DLCL_TRACE] * 2, 1);
				break;
			case PCB_OBJ_PSTK:
			{
				pcb_pstk_t *ps = (pcb_pstk_t *)o;
				pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
				rnd_coord_t nclr = 0;

				if (proto == NULL) break;
				
				if (proto->hdia > 0) {
					if (ps->term == NULL)
						nclr = clr[PCB_DLCL_VIA];
					else
						nclr = clr[PCB_DLCL_TRH_TERM];
				}
				else if (ps->term != NULL)
					nclr = clr[PCB_DLCL_SMD_TERM];

				if (nclr > 0)
					pcb_chg_obj_clear_size(o->type, o->parent.layer, o, NULL, nclr * 2, 1);
			}
			default: break;
		}
	}
}


void pcb_dlcl_apply(pcb_board_t *pcb, rnd_coord_t clr[PCB_DLCL_max])
{
	pcb_data_clip_inhibit_inc(pcb->Data);
	pcb_draw_inhibit_inc();
	pcb_dlcl_apply_(pcb, clr);
	pcb_draw_inhibit_dec();
	pcb_data_clip_inhibit_dec(pcb->Data, 0);
}
