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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */
#include "error.h"
#include "config.h"
#include "pcb_bool.h"

pcb_bool pcb_mid_stroke = pcb_false;

/* Warn if gesture is wider or higher than this amount of coords (2mm) */
#define MIN_CDIFF 2*1000000

/* Warn only once, for large gestures */
static pcb_bool far = pcb_false, warned = pcb_false;
static pcb_coord_t x1 = -1, y1;

static void stub_stroke_record_dummy(pcb_coord_t ev_x, pcb_coord_t ev_y)
{
	if (x1 < 0) {
		x1 = ev_x;
		y1 = ev_y;
	}
	else {
		pcb_coord_t d;

		d = x1 - ev_x;
		if (d < 0) d = -d;
		if (d > MIN_CDIFF) far = pcb_true;

		d = y1 - ev_y;
		if (d < 0) d = -d;
		if (d > MIN_CDIFF) far = pcb_true;
	}
}

static void stub_stroke_start_dummy(void)
{
	pcb_mid_stroke = pcb_true;
}

static int stub_stroke_finish_dummy(pcb_hidlib_t *hl)
{
	if (far) {
		if (!warned) {
			pcb_message(PCB_MSG_WARNING, "No gesture recognition, can not use libstroke: not compiled as a buildin and not loaded as a plugin\n");
			warned = pcb_true;
		}
	}
	pcb_mid_stroke = pcb_false;
	x1 = -1;

	return -1;
}

void (*pcb_stub_stroke_record)(pcb_coord_t ev_x, pcb_coord_t ev_y) = stub_stroke_record_dummy;
void (*pcb_stub_stroke_start)(void) = stub_stroke_start_dummy;

/* Returns 0 on success (gesture recognized) */
int (*pcb_stub_stroke_finish)(pcb_hidlib_t *hl) = stub_stroke_finish_dummy;
