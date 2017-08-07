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
#include "const.h"
#include "hid_actions.h"
#include "obj_all.h"
#include "plugins.h"
#include "stub_draw.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "conf_core.h"

#include "obj_text.h"
#include "obj_text_draw.h"
#include "obj_line_draw.h"

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
	pcb_text_draw(&t, 0);
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
	pcb_line_draw_(&l);
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

#define MAX_FONT 128
typedef struct {
	int y1, y2;
	pcb_font_id_t fid;
} font_coord_t;

font_coord_t font_coord[MAX_FONT];
int font_coords;
int font_new_y1, font_new_y2;
int font_del_y1, font_del_y2;

pcb_text_t *fontsel_txt = NULL;
pcb_layer_t *fontsel_layer = NULL;
int fontsel_txt_type = 0;

static void pcb_draw_font(pcb_hid_gc_t gc, pcb_font_t *f, int x, int *y)
{
	char txt[256];
	pcb_text_t *t;
	const char *nm;
	int y_old = *y;
	pcb_font_id_t target_fid = (fontsel_txt == NULL) ? conf_core.design.text_font_id : fontsel_txt->fid;

	nm = (f->name == NULL) ? "<anonymous>" : f->name;
	pcb_snprintf(txt, sizeof(txt), "#%d [abc ABC 123] %s", f->id, nm);

	dchkbox(gc, x-4, *y, (f->id == target_fid));

	pcb_gui->set_color(gc, "#000000");
	t = dtext(x, *y, 200, f->id, txt);
	pcb_text_bbox(pcb_font(PCB, f->id, 1), t);

	*y += pcb_round(PCB_COORD_TO_MM(t->BoundingBox.Y2 - t->BoundingBox.Y1) + 0.5);

	if (font_coords < MAX_FONT) {
		font_coord[font_coords].y1 = y_old;
		font_coord[font_coords].y2 = *y;
		font_coord[font_coords].fid = f->id;
		font_coords++;
	}
}


static void pcb_draw_fontsel(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	int y = 10;
	pcb_text_t *t;

	pcb_gui->set_color(gc, "#FF0000");
	dtext(0, 0, 300, 0, "Select font");

	font_coords = 0;
	pcb_draw_font(gc, &PCB->fontkit.dflt, 10, &y);

	if (PCB->fontkit.hash_inited) {
		htip_entry_t *e;
		for (e = htip_first(&PCB->fontkit.fonts); e; e = htip_next(&PCB->fontkit.fonts, e))
			pcb_draw_font(gc, e->value, 10, &y);
	}

	/* New font */
	pcb_gui->set_color(gc, "#000000");
	t = dtext(5, y, 250, 0, "[Load font from file]");
	pcb_text_bbox(pcb_font(PCB, 0, 1), t);
	font_new_y1 = y;
	y += pcb_round(PCB_COORD_TO_MM(t->BoundingBox.Y2 - t->BoundingBox.Y1) + 0.5);
	font_new_y2 = y;

	/* Del font */
	pcb_gui->set_color(gc, "#550000");
	t = dtext(5, y, 250, 0, "[Remove current font]");
	pcb_text_bbox(pcb_font(PCB, 0, 1), t);
	font_del_y1 = y;
	y += pcb_round(PCB_COORD_TO_MM(t->BoundingBox.Y2 - t->BoundingBox.Y1) + 0.5);
	font_del_y2 = y;

}

static pcb_font_id_t lookup_fid_for_coord(int ymm)
{
	int n;

	for(n = 0; n < font_coords; n++)
		if ((ymm >= font_coord[n].y1) && (ymm <= font_coord[n].y2))
			return font_coord[n].fid;
	return -1;
}

static pcb_bool pcb_mouse_fontsel(void *widget, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	pcb_font_id_t fid;
	int ymm;

	switch(kind) {
		case PCB_HID_MOUSE_PRESS:
			ymm = PCB_COORD_TO_MM(y);
			fid = lookup_fid_for_coord(ymm);
			if (fid >= 0) {
				if (fontsel_txt == NULL) {
					char sval[128];
					sprintf(sval, "%ld", fid);
					conf_set(CFR_DESIGN, "design/text_font_id", 0, sval, POL_OVERWRITE);
				}
				else {
					switch(fontsel_txt_type) {
						case PCB_TYPE_TEXT:
							pcb_text_set_font(fontsel_layer, fontsel_txt, fid);
							break;
						case PCB_TYPE_ELEMENT_NAME:
							{
								int which;
								pcb_element_t *e = (pcb_element_t *)fontsel_layer;
								for(which = 0; which <= PCB_ELEMNAME_IDX_VALUE; which++) {
									if (&e->Name[which] == fontsel_txt) {
										pcb_element_text_set_font(PCB, PCB->Data, e, which, fid);
										break;
									}
								}
							}
							break;
					}
					pcb_gui->invalidate_all();
				}
				return 1;
			}
			else if ((ymm >= font_new_y1) && (ymm <= font_new_y2)) {
				pcb_hid_actionl("LoadFontFrom", NULL); /* modal, blocking */
				return 1;
			}
			else if ((ymm >= font_del_y1) && (ymm <= font_del_y2)) {
				if (conf_core.design.text_font_id == 0) {
					pcb_message(PCB_MSG_ERROR, "Can not remove the default font.\n");
					return 0;
				}
				pcb_del_font(&PCB->fontkit, conf_core.design.text_font_id);
				conf_set(CFR_DESIGN, "design/text_font_id", 0, "0", POL_OVERWRITE);
				return 1;
			}
	}
	return 0;
}

int pplg_check_ver_draw_fontsel(int ver_needed) { return 0; }

void pplg_uninit_draw_fontsel(void)
{
}

int pplg_init_draw_fontsel(void)
{
	pcb_stub_draw_fontsel = pcb_draw_fontsel;
	pcb_stub_draw_fontsel_mouse_ev = pcb_mouse_fontsel;
	pcb_stub_draw_fontsel_text_obj = &fontsel_txt;
	pcb_stub_draw_fontsel_layer_obj = &fontsel_layer;
	pcb_stub_draw_fontsel_text_type = &fontsel_txt_type;

	return 0;
}
