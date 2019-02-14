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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include "config.h"
#include "stub_draw.h"
#include "obj_text.h"
#include "obj_text_draw.h"

/****** common code ******/
void dummy_draw_text(pcb_draw_info_t *info, pcb_hid_gc_t gc, const char *str)
{
	pcb_text_t t;

	pcb_gui->set_color(gc, pcb_color_red);

	memset(&t, 0, sizeof(t));
	t.TextString = (char *)str;
	t.fid = 0; /* use the default font */
	t.Scale = 150;
	t.Flags = pcb_no_flags();
	pcb_text_draw_(info, &t, 0, 0, PCB_TXT_TINY_ACCURATE);
}

static pcb_bool dummy_mouse(pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}


/****** fab ******/

int dummy_DrawFab_overhang(void)
{
	return 0;
}

void dummy_DrawFab(pcb_draw_info_t *info, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	dummy_draw_text(info, gc, "Can't render the fab layer: the draw_fab plugin is not compiled and/or not loaded");
}

int (*pcb_stub_draw_fab_overhang)(void) = dummy_DrawFab_overhang;
void (*pcb_stub_draw_fab)(pcb_draw_info_t *info, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e) = dummy_DrawFab;


/****** csect - cross section of the board ******/


static void dummy_draw_csect(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	dummy_draw_text(NULL, gc, "Can't render the fab layer: the draw_csect plugin is not compiled and/or not loaded");
}


void (*pcb_stub_draw_csect)(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e) = dummy_draw_csect;
pcb_bool (*pcb_stub_draw_csect_mouse_ev)(pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y) = dummy_mouse;


/****** font selector GUI ******/
static void dummy_draw_fontsel(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	dummy_draw_text(NULL, gc, "Can't render the font selector: the draw_fontsel plugin is not compiled and/or not loaded");
}

static pcb_text_t *dummy_fontsel_text = NULL;

void (*pcb_stub_draw_fontsel)(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e, pcb_text_t *txt) = dummy_draw_fontsel;
pcb_bool (*pcb_stub_draw_fontsel_mouse_ev)(pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y, pcb_text_t *txt) = dummy_mouse;

