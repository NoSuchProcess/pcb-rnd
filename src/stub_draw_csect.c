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

/* stub for drawing cross section of the board */

#include "config.h"
#include "stub_draw_csect.h"
#include "obj_text.h"
#include "obj_text_draw.h"

static void dummy_draw_csect(pcb_hid_gc_t gc)
{
	pcb_text_t t;
	t.X = 0;
	t.Y = 0;
	t.TextString = "Can't render the fab layer: the draw_csect plugin is not compiled and/or not loaded";
	t.Direction = 0;
	t.Scale = 150;
	t.Flags = pcb_no_flags();
	DrawTextLowLevel(&t, 0);
}

void (*pcb_stub_draw_csect)(pcb_hid_gc_t gc) = dummy_draw_csect;

