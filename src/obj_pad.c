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

/* Drawing primitive: smd pads */

#include "config.h"

#include "board.h"
#include "data.h"
#include "undo.h"
#include "polygon.h"
#include "compat_misc.h"
#include "conf_core.h"

#include "obj_pad.h"
#include "obj_pad_list.h"
#include "obj_pad_op.h"
#include "obj_hash.h"

/* TODO: remove this if draw.[ch] pads are moved */
#include "draw.h"
#include "obj_text_draw.h"
#include "obj_pad_draw.h"

#include "obj_elem.h"

/* changes the nopaste flag of a pad */
pcb_bool pcb_pad_change_paste(pcb_pad_t *Pad)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pad))
		return pcb_false;
	pcb_undo_add_obj_to_flag(Pad);
	PCB_FLAG_TOGGLE(PCB_FLAG_NOPASTE, Pad);
	return pcb_true;
}
