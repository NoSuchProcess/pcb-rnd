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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* printing routines */
#include "config.h"

#include <time.h>

#include "board.h"
#include "build_run.h"
#include "data.h"
#include "draw.h"
#include "font.h"
#include "obj_all.h"
#include "plugins.h"
#include "stub_draw.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "conf_core.h"

#include "obj_text.h"
#include "obj_text_draw.h"

static pcb_text_t *dtext(int x, int y, int scale, pcb_font_id_t fid, const char *txt)
{
	static pcb_text_t t;

	t.X = PCB_MM_TO_COORD(x);
	t.Y = PCB_MM_TO_COORD(y);
	t.TextString = (char *)txt;
	t.Direction = 0;
	t.Scale = scale;
	t.fid = fid;
	t.Flags = pcb_no_flags();
	DrawTextLowLevel(&t, 0);
	return &t;
}

/* draw a line of a specific thickness */
static void dline(int x1, int y1, int x2, int y2, float thick)
{
	pcb_line_t l;
	l.Point1.X = PCB_MM_TO_COORD(x1);
	l.Point1.Y = PCB_MM_TO_COORD(y1);
	l.Point2.X = PCB_MM_TO_COORD(x2);
	l.Point2.Y = PCB_MM_TO_COORD(y2);
	l.Thickness = PCB_MM_TO_COORD(thick);
	_draw_line(&l);
}

static void dchkbox(pcb_hid_gc_t gc, int x0, int y0, int checked)
{
	int w = 2, h = 2;
	float th = 0.1, th2 = 0.4;

	pcb_gui->set_color(gc, "#000000");
	dline(x0, y0, x0+w, y0, th);
	dline(x0+w, y0, x0+w, y0+h, th);
	dline(x0+w, y0+h, x0, y0+h, th);
	dline(x0, y0+h, x0, y0, th);
	if (checked) {
		pcb_gui->set_color(gc, "#AA0033");
		dline(x0, y0, x0+w, y0+h, th2);
		dline(x0, y0+h, x0+w, y0, th2);
	}
}


static void pcb_draw_font(pcb_hid_gc_t gc, pcb_font_t *f, int x, int *y)
{
	char txt[256];
	pcb_text_t *t;
	const char *nm;

	nm = (f->name == NULL) ? "<anonymous>" : f->name;
	pcb_snprintf(txt, sizeof(txt), "#%d [abc ABC 123] %s", f->id, nm);

	dchkbox(gc, x-4, *y, (f->id == conf_core.design.text_font_id));

	pcb_gui->set_color(gc, "#000000");
	t = dtext(x, *y, 200, f->id, txt);
	pcb_text_bbox(pcb_font(PCB, f->id, 1), t);

	*y += pcb_round(PCB_COORD_TO_MM(t->BoundingBox.Y2 - t->BoundingBox.Y1) + 0.5);
}


static void pcb_draw_fontsel(pcb_hid_gc_t gc)
{
	int y = 10;

	pcb_gui->set_color(gc, "#FF0000");
	dtext(0, 0, 300, 0, "Select font");


	pcb_draw_font(gc, &PCB->fontkit.dflt, 10, &y);

	if (PCB->fontkit.hash_inited) {
		htip_entry_t *e;
		for (e = htip_first(&PCB->fontkit.fonts); e; e = htip_next(&PCB->fontkit.fonts, e))
			pcb_draw_font(gc, e->value, 10, &y);
	}

}

static void hid_draw_fontsel_uninit(void)
{
}

pcb_uninit_t hid_draw_fontsel_init(void)
{
	pcb_stub_draw_fontsel = pcb_draw_fontsel;
	return hid_draw_fontsel_uninit;
}
