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
#include "error.h"
#include "config.h"
#include "pcb_bool.h"

pcb_bool pcb_mid_stroke = pcb_false;

static void stub_stroke_record_dummy(int ev_x, int ev_y)
{
}

static void stub_stroke_start_dummy(void)
{
	pcb_message(PCB_MSG_DEFAULT, "Can not use libstroke: not compiled as a buildin and not loaded as a plugin\n");
}

void (*pcb_stub_stroke_record)(int ev_x, int ev_y) = stub_stroke_record_dummy;
void (*pcb_stub_stroke_start)(void) = stub_stroke_start_dummy;
void (*pcb_stub_stroke_finish)(void) = stub_stroke_start_dummy;
