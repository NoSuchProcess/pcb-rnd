/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/* draw cross section */
#include "config.h"


#include "board.h"
#include "data.h"
#include "draw.h"
#include "plugins.h"
#include "stub_draw_csect.h"

static void draw_csect(pcb_hid_gc_t gc)
{
	
}

static void hid_draw_csect_uninit(void)
{
}

pcb_uninit_t hid_draw_csect_init(void)
{
	pcb_stub_draw_csect = draw_csect;
	return hid_draw_csect_uninit;
}
