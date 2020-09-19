/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  ttf glyph import
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "data.h"

#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>

#include "ttf_load.h"
#include "str_approx.h"

static const char *ttf_cookie = "ttf importer";

static void str_init(pcb_ttf_stroke_t *s)
{
	rnd_trace("stroke init\n");
}

static void str_start(pcb_ttf_stroke_t *s, int chr)
{
	rnd_trace("stroke start\n");
}

static void str_finish(pcb_ttf_stroke_t *s)
{
	rnd_trace("stroke finish\n");
}

static void str_uninit(pcb_ttf_stroke_t *s)
{
	rnd_trace("stroke uninit\n");
}

#define TRX(x) RND_MM_TO_COORD((x) * str->scale_x + str->dx)
#define TRY(y) RND_MM_TO_COORD((str->ttf->face->height - (y) - str->ttf->face->ascender - str->ttf->face->descender) * str->scale_y + str->dy)

static int str_move_to(const FT_Vector *to, void *s_)
{
	pcb_ttf_stroke_t *str = s_;
	rnd_trace(" move %f;%f %ld;%ld\n", str->x, str->y, to->x, to->y);
	str->x = to->x;
	str->y = to->y;
	return 0;
}

static int str_line_to(const FT_Vector *to, void *s_)
{
	pcb_ttf_stroke_t *str = s_;
	rnd_trace(" line %f;%f %ld;%ld\n", str->x, str->y, to->x, to->y);
	pcb_font_new_line_in_sym(str->sym,
		TRX(str->x), TRY(str->y),
		TRX(to->x),  TRY(to->y),
		1);
	str->x = to->x;
	str->y = to->y;
	return 0;
}


static const char pcb_acts_LoadTtfGlyphs[] = "LoadTtfGlyphs(filename, srcglyps, [dstchars])";
static const char pcb_acth_LoadTtfGlyphs[] = "Loads glyphs from an outline ttf in the specified source range, optionally remapping them to dstchars range in the pcb-rnd font";
fgw_error_t pcb_act_LoadTtfGlyphs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *fn;
	pcb_ttf_t ctx = {0};
	pcb_ttf_stroke_t stroke = {0};
	int r;
	pcb_font_t *f = pcb_font(pcb, 0, 1);

	RND_ACT_CONVARG(1, FGW_STR, LoadTtfGlyphs, fn = argv[1].val.str);

	r = pcb_ttf_load(&ctx, fn);
	rnd_trace("ttf load; %d\n", r);

	stroke.funcs.move_to = str_move_to;
	stroke.funcs.line_to = str_line_to;
	stroke.funcs.conic_to = stroke_approx_conic_to;
	stroke.funcs.cubic_to = stroke_approx_cubic_to;
	
	stroke.init   = str_init;
	stroke.start  = str_start;
	stroke.finish = str_finish;
	stroke.uninit = str_uninit;

	stroke.scale_x = stroke.scale_y = 0.001;
	stroke.sym = &f->Symbol['A'];
	stroke.ttf = &ctx;

	pcb_font_free_symbol(stroke.sym);


	r = pcb_ttf_trace(&ctx, 'A', 'A', &stroke, 1);
	rnd_trace("ttf trace; %d\n", r);
	stroke.sym->Valid = 1;

	pcb_ttf_unload(&ctx);
	RND_ACT_IRES(-1);
	return 0;
}

rnd_action_t ttf_action_list[] = {
	{"LoadTtfGlyphs", pcb_act_LoadTtfGlyphs, pcb_acth_LoadTtfGlyphs, pcb_acts_LoadTtfGlyphs}
};

int pplg_check_ver_import_ttf(int ver_needed) { return 0; }

void pplg_uninit_import_ttf(void)
{
	rnd_remove_actions_by_cookie(ttf_cookie);
}

int pplg_init_import_ttf(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(ttf_action_list, ttf_cookie)
	return 0;
}
