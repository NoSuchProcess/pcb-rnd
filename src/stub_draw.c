/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */
#include "config.h"
#include "stub_draw.h"
#include "obj_text.h"
#include "obj_text_draw.h"

/****** common code ******/
void dummy_draw_text(pcb_hid_gc_t gc, const char *str)
{
	pcb_text_t t;

	pcb_gui->set_color(gc, "#FF0000");

	t.X = 0;
	t.Y = 0;
	t.TextString = str;
	t.Direction = 0;
	t.fid = 0; /* use the default font */
	t.Scale = 150;
	t.Flags = pcb_no_flags();
	DrawTextLowLevel(&t, 0);
}

static pcb_bool dummy_mouse(void *widget, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

static void dummy_overlay(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *ctx)
{
}


/****** fab ******/

int dummy_DrawFab_overhang(void)
{
	return 0;
}

void dummy_DrawFab(pcb_hid_gc_t gc)
{
	dummy_draw_text(gc, "Can't render the fab layer: the draw_fab plugin is not compiled and/or not loaded");
}

int (*pcb_stub_draw_fab_overhang)(void) = dummy_DrawFab_overhang;
void (*pcb_stub_draw_fab)(pcb_hid_gc_t gc) = dummy_DrawFab;


/****** csect - cross section of the board ******/


static void dummy_draw_csect(pcb_hid_gc_t gc)
{
	dummy_draw_text(gc, "Can't render the fab layer: the draw_csect plugin is not compiled and/or not loaded");
}


void (*pcb_stub_draw_csect)(pcb_hid_gc_t gc) = dummy_draw_csect;
pcb_bool (*pcb_stub_draw_csect_mouse_ev)(void *widget, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y) = dummy_mouse;
void (*pcb_stub_draw_csect_overlay)(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *ctx) = dummy_overlay;


/****** font selector GUI ******/
static void dummy_draw_fontsel(pcb_hid_gc_t gc)
{
	dummy_draw_text(gc, "Can't render the font selector: the draw_fontsel plugin is not compiled and/or not loaded");
}

void (*pcb_stub_draw_fontsel)(pcb_hid_gc_t gc) = dummy_draw_fontsel;
pcb_bool (*pcb_stub_draw_fontsel_mouse_ev)(void *widget, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y) = dummy_mouse;

