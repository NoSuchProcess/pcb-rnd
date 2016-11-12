/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2003 Thomas Nau
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
#include "../report/drill.h"
#include "obj_all.h"
#include "plugins.h"
#include "stub_draw_fab.h"

#include "obj_text_draw.h"


/* ---------------------------------------------------------------------------
 * prints a FAB drawing.
 */

#define TEXT_SIZE	PCB_MIL_TO_COORD(150)
#define TEXT_LINE	PCB_MIL_TO_COORD(150)
#define DRILL_MARK_SIZE	PCB_MIL_TO_COORD(16)
#define FAB_LINE_W      PCB_MIL_TO_COORD(8)

static void fab_line(hid_gc_t gc, int x1, int y1, int x2, int y2)
{
	gui->draw_line(gc, x1, y1, x2, y2);
}

static void fab_circle(hid_gc_t gc, int x, int y, int r)
{
	gui->draw_arc(gc, x, y, r, r, 0, 180);
	gui->draw_arc(gc, x, y, r, r, 180, 180);
}

/* align is 0=left, 1=center, 2=right, add 8 for underline */
static void text_at(hid_gc_t gc, int x, int y, int align, const char *fmt, ...)
{
	char tmp[512];
	int w = 0, i;
	pcb_text_t t;
	va_list a;
	pcb_font_t *font = &PCB->Font;
	va_start(a, fmt);
	vsprintf(tmp, fmt, a);
	va_end(a);
	t.Direction = 0;
	t.TextString = tmp;
	t.Scale = PCB_COORD_TO_MIL(TEXT_SIZE);	/* pcnt of 100mil base height */
	t.Flags = NoFlags();
	t.X = x;
	t.Y = y;
	for (i = 0; tmp[i]; i++)
		w += (font->Symbol[(int) tmp[i]].Width + font->Symbol[(int) tmp[i]].Delta);
	w = PCB_SCALE_TEXT(w, t.Scale);
	t.X -= w * (align & 3) / 2;
	if (t.X < 0)
		t.X = 0;
	DrawTextLowLevel(&t, 0);
	if (align & 8)
		fab_line(gc, t.X,
						 t.Y + PCB_SCALE_TEXT(font->MaxHeight, t.Scale) + PCB_MIL_TO_COORD(10),
						 t.X + w, t.Y + PCB_SCALE_TEXT(font->MaxHeight, t.Scale) + PCB_MIL_TO_COORD(10));
}

/* Y, +, X, circle, square */
static void drill_sym(hid_gc_t gc, int idx, int x, int y)
{
	int type = idx % 5;
	int size = idx / 5;
	int s2 = (size + 1) * DRILL_MARK_SIZE;
	int i;
	switch (type) {
	case 0:											/* Y */ ;
		fab_line(gc, x, y, x, y + s2);
		fab_line(gc, x, y, x + s2 * 13 / 15, y - s2 / 2);
		fab_line(gc, x, y, x - s2 * 13 / 15, y - s2 / 2);
		for (i = 1; i <= size; i++)
			fab_circle(gc, x, y, i * DRILL_MARK_SIZE);
		break;
	case 1:											/* + */
		;
		fab_line(gc, x, y - s2, x, y + s2);
		fab_line(gc, x - s2, y, x + s2, y);
		for (i = 1; i <= size; i++) {
			fab_line(gc, x - i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE, x + i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE);
			fab_line(gc, x - i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE, x - i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE);
			fab_line(gc, x - i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE, x + i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE);
			fab_line(gc, x + i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE, x + i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE);
		}
		break;
	case 2:											/* X */ ;
		fab_line(gc, x - s2 * 3 / 4, y - s2 * 3 / 4, x + s2 * 3 / 4, y + s2 * 3 / 4);
		fab_line(gc, x - s2 * 3 / 4, y + s2 * 3 / 4, x + s2 * 3 / 4, y - s2 * 3 / 4);
		for (i = 1; i <= size; i++) {
			fab_line(gc, x - i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE, x + i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE);
			fab_line(gc, x - i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE, x - i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE);
			fab_line(gc, x - i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE, x + i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE);
			fab_line(gc, x + i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE, x + i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE);
		}
		break;
	case 3:											/* circle */ ;
		for (i = 0; i <= size; i++)
			fab_circle(gc, x, y, (i + 1) * DRILL_MARK_SIZE - DRILL_MARK_SIZE / 2);
		break;
	case 4:											/* square */
		for (i = 1; i <= size + 1; i++) {
			fab_line(gc, x - i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE, x + i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE);
			fab_line(gc, x - i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE, x - i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE);
			fab_line(gc, x - i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE, x + i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE);
			fab_line(gc, x + i * DRILL_MARK_SIZE, y - i * DRILL_MARK_SIZE, x + i * DRILL_MARK_SIZE, y + i * DRILL_MARK_SIZE);
		}
		break;
	}
}

static int count_drill_lines(DrillInfoTypePtr AllDrills)
{
	int n, ds = 0;
	for (n = AllDrills->DrillN - 1; n >= 0; n--) {
		DrillTypePtr drill = &(AllDrills->Drill[n]);
		if (drill->PinCount + drill->ViaCount > drill->UnplatedCount)
			ds++;
		if (drill->UnplatedCount)
			ds++;
	}
	return ds;
}


static int DrawFab_overhang(void)
{
	DrillInfoTypePtr AllDrills = GetDrillInfo(PCB->Data);
	int ds = count_drill_lines(AllDrills);
	if (ds < 4)
		ds = 4;
	return (ds + 2) * TEXT_LINE;
}

static void DrawFab(hid_gc_t gc)
{
	DrillInfoTypePtr AllDrills;
	int i, n, yoff, total_drills = 0, ds = 0;
	time_t currenttime;
	char utcTime[64];
	AllDrills = GetDrillInfo(PCB->Data);
	RoundDrillInfo(AllDrills, PCB_MIL_TO_COORD(1));
	yoff = -TEXT_LINE;

	/* count how many drill description lines will be needed */
	ds = count_drill_lines(AllDrills);

	/*
	 * When we only have a few drill sizes we need to make sure the
	 * drill table header doesn't fall on top of the board info
	 * section.
	 */
	if (ds < 4) {
		yoff -= (4 - ds) * TEXT_LINE;
	}

	gui->set_line_width(gc, FAB_LINE_W);

	for (n = AllDrills->DrillN - 1; n >= 0; n--) {
		int plated_sym = -1, unplated_sym = -1;
		DrillTypePtr drill = &(AllDrills->Drill[n]);
		if (drill->PinCount + drill->ViaCount > drill->UnplatedCount)
			plated_sym = --ds;
		if (drill->UnplatedCount)
			unplated_sym = --ds;
		gui->set_color(gc, PCB->PinColor);
		for (i = 0; i < drill->PinN; i++)
			drill_sym(gc, TEST_FLAG(PCB_FLAG_HOLE, drill->Pin[i]) ? unplated_sym : plated_sym, drill->Pin[i]->X, drill->Pin[i]->Y);
		if (plated_sym != -1) {
			drill_sym(gc, plated_sym, TEXT_SIZE, yoff + TEXT_SIZE / 4);
			text_at(gc, PCB_MIL_TO_COORD(1350), yoff, PCB_MIL_TO_COORD(2), "YES");
			text_at(gc, PCB_MIL_TO_COORD(980), yoff, PCB_MIL_TO_COORD(2), "%d", drill->PinCount + drill->ViaCount - drill->UnplatedCount);

			if (unplated_sym != -1)
				yoff -= TEXT_LINE;
		}
		if (unplated_sym != -1) {
			drill_sym(gc, unplated_sym, TEXT_SIZE, yoff + TEXT_SIZE / 4);
			text_at(gc, PCB_MIL_TO_COORD(1400), yoff, PCB_MIL_TO_COORD(2), "NO");
			text_at(gc, PCB_MIL_TO_COORD(980), yoff, PCB_MIL_TO_COORD(2), "%d", drill->UnplatedCount);
		}
		gui->set_color(gc, PCB->ElementColor);
		text_at(gc, PCB_MIL_TO_COORD(450), yoff, PCB_MIL_TO_COORD(2), "%0.3f", PCB_COORD_TO_INCH(drill->DrillSize) + 0.0004);
		if (plated_sym != -1 && unplated_sym != -1)
			text_at(gc, PCB_MIL_TO_COORD(450), yoff + TEXT_LINE, PCB_MIL_TO_COORD(2), "%0.3f", PCB_COORD_TO_INCH(drill->DrillSize) + 0.0004);
		yoff -= TEXT_LINE;
		total_drills += drill->PinCount;
		total_drills += drill->ViaCount;
	}

	gui->set_color(gc, PCB->ElementColor);
	text_at(gc, 0, yoff, PCB_MIL_TO_COORD(9), "Symbol");
	text_at(gc, PCB_MIL_TO_COORD(410), yoff, PCB_MIL_TO_COORD(9), "Diam. (Inch)");
	text_at(gc, PCB_MIL_TO_COORD(950), yoff, PCB_MIL_TO_COORD(9), "Count");
	text_at(gc, PCB_MIL_TO_COORD(1300), yoff, PCB_MIL_TO_COORD(9), "Plated?");
	yoff -= TEXT_LINE;
	text_at(gc, 0, yoff, 0,
					"There are %d different drill sizes used in this layout, %d holes total", AllDrills->DrillN, total_drills);
	/* Create a portable timestamp. */
	currenttime = time(NULL);
	{
		/* avoid gcc complaints */
		const char *fmt = "%c UTC";
		strftime(utcTime, sizeof utcTime, fmt, gmtime(&currenttime));
	}
	yoff = -TEXT_LINE;
	for (i = 0; i < max_copper_layer; i++) {
		pcb_layer_t *l = LAYER_PTR(i);
		if (l->Name && (linelist_length(&l->Line) || arclist_length(&l->Arc))) {
			if (strcmp("route", l->Name) == 0)
				break;
			if (strcmp("outline", l->Name) == 0)
				break;
		}
	}
	if (i == max_copper_layer) {
		gui->set_line_width(gc, PCB_MIL_TO_COORD(10));
		gui->draw_line(gc, 0, 0, PCB->MaxWidth, 0);
		gui->draw_line(gc, 0, 0, 0, PCB->MaxHeight);
		gui->draw_line(gc, PCB->MaxWidth, 0, PCB->MaxWidth, PCB->MaxHeight);
		gui->draw_line(gc, 0, PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight);
		/*FPrintOutline (); */
		gui->set_line_width(gc, FAB_LINE_W);
		text_at(gc, PCB_MIL_TO_COORD(2000), yoff, 0,
						"Maximum Dimensions: %f mils wide, %f mils high", PCB_COORD_TO_MIL(PCB->MaxWidth), PCB_COORD_TO_MIL(PCB->MaxHeight));
		text_at(gc, PCB->MaxWidth / 2, PCB->MaxHeight + PCB_MIL_TO_COORD(20), 1,
						"Board outline is the centerline of this %f mil"
						" rectangle - 0,0 to %f,%f mils",
						PCB_COORD_TO_MIL(FAB_LINE_W), PCB_COORD_TO_MIL(PCB->MaxWidth), PCB_COORD_TO_MIL(PCB->MaxHeight));
	}
	else {
		pcb_layer_t *layer = LAYER_PTR(i);
		gui->set_line_width(gc, PCB_MIL_TO_COORD(10));
		LINE_LOOP(layer);
		{
			gui->draw_line(gc, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
		}
		END_LOOP;
		ARC_LOOP(layer);
		{
			gui->draw_arc(gc, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);
		}
		END_LOOP;
		TEXT_LOOP(layer);
		{
			DrawTextLowLevel(text, 0);
		}
		END_LOOP;
		gui->set_line_width(gc, FAB_LINE_W);
		text_at(gc, PCB->MaxWidth / 2, PCB->MaxHeight + PCB_MIL_TO_COORD(20), 1, "Board outline is the centerline of this path");
	}
	yoff -= TEXT_LINE;
	text_at(gc, PCB_MIL_TO_COORD(2000), yoff, 0, "Date: %s", utcTime);
	yoff -= TEXT_LINE;
	text_at(gc, PCB_MIL_TO_COORD(2000), yoff, 0, "Author: %s", pcb_author());
	yoff -= TEXT_LINE;
	text_at(gc, PCB_MIL_TO_COORD(2000), yoff, 0, "Title: %s - Fabrication Drawing", UNKNOWN(PCB->Name));
}

static void hid_draw_fab_uninit(void)
{
}

pcb_uninit_t hid_draw_fab_init(void)
{
	stub_DrawFab = DrawFab;
	stub_DrawFab_overhang = DrawFab_overhang;
	return hid_draw_fab_uninit;
}
