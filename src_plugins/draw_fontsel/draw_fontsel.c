/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2018 Tibor 'Igor2' Palinkas
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
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include "stub_draw.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include "conf_core.h"

#include "obj_text.h"
#include "obj_text_draw.h"
#include "obj_line_draw.h"

static pcb_draw_info_t dinfo;
static rnd_xform_t dxform;


static pcb_text_t *dtext(int x, int y, int scale, pcb_font_id_t fid, const char *txt)
{
	static pcb_text_t t = {0};

	if (dinfo.xform == NULL) dinfo.xform = &dxform;

	t.X = RND_MM_TO_COORD(x);
	t.Y = RND_MM_TO_COORD(y);
	t.TextString = (char *)txt;
	t.Scale = scale;
	t.fid = fid;
	t.Flags = pcb_no_flags();
	pcb_text_draw_(&dinfo, &t, 0, 0, PCB_TXT_TINY_ACCURATE);
	return &t;
}

/* draw a line of a specific thickness */
static void dline(int x1, int y1, int x2, int y2, float thick)
{
	pcb_line_t l;

	if (dinfo.xform == NULL) dinfo.xform = &dxform;

	l.Point1.X = RND_MM_TO_COORD(x1);
	l.Point1.Y = RND_MM_TO_COORD(y1);
	l.Point2.X = RND_MM_TO_COORD(x2);
	l.Point2.Y = RND_MM_TO_COORD(y2);
	l.Thickness = RND_MM_TO_COORD(thick);
	pcb_line_draw_(&dinfo, &l, 0);
}

static void dchkbox(rnd_hid_gc_t gc, int x0, int y0, int checked)
{
	int w = 2, h = 2;
	float th = 0.1, th2 = 0.4;

	rnd_render->set_color(gc, rnd_color_black);
	dline(x0, y0, x0+w, y0, th);
	dline(x0+w, y0, x0+w, y0+h, th);
	dline(x0+w, y0+h, x0, y0+h, th);
	dline(x0, y0+h, x0, y0, th);
	if (checked) {
		rnd_render->set_color(gc, rnd_color_red);
		dline(x0, y0, x0+w, y0+h, th2);
		dline(x0, y0+h, x0+w, y0, th2);
	}
}

#define MAX_FONT 128
typedef struct {
	int y1, y2;
	pcb_font_id_t fid;
} font_coord_t;

font_coord_t font_coord[MAX_FONT];
int font_coords;

static void pcb_draw_font(rnd_hid_gc_t gc, pcb_font_t *f, int x, int *y, pcb_text_t *txt)
{
	char buf[256];
	pcb_text_t *t;
	const char *nm;
	int y_old = *y;
	pcb_font_id_t target_fid = (txt == NULL) ? conf_core.design.text_font_id : txt->fid;

	nm = (f->name == NULL) ? "<anonymous>" : f->name;
	rnd_snprintf(buf, sizeof(buf), "#%d [abc ABC 123] %s", f->id, nm);

	dchkbox(gc, x-4, *y, (f->id == target_fid));

	rnd_render->set_color(gc, rnd_color_black);
	t = dtext(x, *y, 200, f->id, buf);
	pcb_text_bbox(pcb_font(PCB, f->id, 1), t);

	*y += rnd_round(RND_COORD_TO_MM(t->BoundingBox.Y2 - t->BoundingBox.Y1) + 0.5);

	if (font_coords < MAX_FONT) {
		font_coord[font_coords].y1 = y_old;
		font_coord[font_coords].y2 = *y;
		font_coord[font_coords].fid = f->id;
		font_coords++;
	}
}


static void pcb_draw_fontsel(rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e, pcb_text_t *txt)
{
	int y = 0;

	font_coords = 0;
	pcb_draw_font(gc, &PCB->fontkit.dflt, 0, &y, txt);

	if (PCB->fontkit.hash_inited) {
		htip_entry_t *e;
		for (e = htip_first(&PCB->fontkit.fonts); e; e = htip_next(&PCB->fontkit.fonts, e))
			pcb_draw_font(gc, e->value, 0, &y, txt);
	}
}

static pcb_font_id_t lookup_fid_for_coord(int ymm)
{
	int n;

	for(n = 0; n < font_coords; n++)
		if ((ymm >= font_coord[n].y1) && (ymm <= font_coord[n].y2))
			return font_coord[n].fid;
	return -1;
}

static rnd_bool pcb_mouse_fontsel(rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y, pcb_text_t *txt)
{
	pcb_font_id_t fid;
	int ymm;

	switch(kind) {
		case RND_HID_MOUSE_PRESS:
			ymm = RND_COORD_TO_MM(y);
			fid = lookup_fid_for_coord(ymm);
			if (fid >= 0) {
				if (txt == NULL) {
					char sval[128];
					sprintf(sval, "%ld", fid);
					rnd_conf_set(RND_CFR_DESIGN, "design/text_font_id", 0, sval, RND_POL_OVERWRITE);
				}
				else {
					switch(txt->type) {
						case PCB_OBJ_TEXT:
							pcb_text_set_font(txt, fid);
							break;
						default:
							break;
					}
					rnd_gui->invalidate_all(rnd_gui);
				}
				return 1;
			}
		default:
			break;
	}
	return 0;
}

int pplg_check_ver_draw_fontsel(int ver_needed) { return 0; }

void pplg_uninit_draw_fontsel(void)
{
}

int pplg_init_draw_fontsel(void)
{
	RND_API_CHK_VER;
	pcb_stub_draw_fontsel = pcb_draw_fontsel;
	pcb_stub_draw_fontsel_mouse_ev = pcb_mouse_fontsel;
	return 0;
}
