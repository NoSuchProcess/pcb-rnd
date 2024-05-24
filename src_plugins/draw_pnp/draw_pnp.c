/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
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

static pcb_draw_info_t dinfo;
static rnd_xform_t dxform;

/* Safe color that is always visible on the background color, even if the user changed background to dark */
#define BLACK  (&conf_core.appearance.color.element)

static pcb_text_t *dtext(int x, int y, int scale, rnd_font_id_t fid, const char *txt)
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
	pcb_line_t l = {0};

	if (dinfo.xform == NULL) dinfo.xform = &dxform;

	l.Point1.X = RND_MM_TO_COORD(x1);
	l.Point1.Y = RND_MM_TO_COORD(y1);
	l.Point2.X = RND_MM_TO_COORD(x2);
	l.Point2.Y = RND_MM_TO_COORD(y2);
	l.Thickness = RND_MM_TO_COORD(thick);
	pcb_line_draw_(&dinfo, &l, 0);
}


int pplg_check_ver_draw_pnp(int ver_needed) { return 0; }

void pplg_uninit_draw_pnp(void)
{
}

int pplg_init_draw_pnp(void)
{
	RND_API_CHK_VER;

	return 0;
}
