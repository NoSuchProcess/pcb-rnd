/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

#include <stdarg.h>
#include <ctype.h>
#include "proto_lowcommon.h"
#include "proto_lowsend.h"
#include "proto_lowparse.h"

static const int proto_ver = 1;
static proto_ctx_t pctx;
static const char *cfmt = "%.08mm";

void remote_proto_send_ver()
{
	send_begin(&pctx, "ver");
	send_open(&pctx, 0);
	sendf(&pctx, "%ld", proto_ver);
	send_close(&pctx);
	send_end(&pctx);
}

void remote_proto_send_unit()
{
	send_begin(&pctx, "unit");
	send_open(&pctx, 0);
	sendf(&pctx, "mm");
	send_close(&pctx);
	send_end(&pctx);
}

int remote_proto_send_ready()
{
	send_begin(&pctx, "ready");
	send_open(&pctx, 0);
	send_close(&pctx);
	send_end(&pctx);
	return 0;
}

void proto_send_invalidate(int l, int r, int t, int b)
{
	send_begin(&pctx, "inval");
	send_open(&pctx, 0);
	sendf(&pctx, "%d", l);
	sendf(&pctx, "%d", r);
	sendf(&pctx, "%d", t);
	sendf(&pctx, "%d", b);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_invalidate_all()
{
	send_begin(&pctx, "inval");
	send_open(&pctx, 0);
	send_close(&pctx);
	send_end(&pctx);
}

int proto_send_set_layer(const char *name, int idx, int empty)
{
	send_begin(&pctx, "setly");
	send_open(&pctx, str_is_bin(name));
	sendf(&pctx, "%s", name);
	sendf(&pctx, "%d", idx);
	sendf(&pctx, "%d", empty);
	send_close(&pctx);
	send_end(&pctx);
	return 0;
}

int proto_send_make_gc(void)
{
	send_begin(&pctx, "makeGC");
	send_open(&pctx, 0);
	send_close(&pctx);
	send_end(&pctx);
	return 0;
}

int proto_send_del_gc(void)
{
	send_begin(&pctx, "delGC");
	send_open(&pctx, 0);
	send_close(&pctx);
	send_end(&pctx);
	return 0;
}

void proto_send_set_color(int gc, const char *name)
{
	send_begin(&pctx, "clr");
	send_open(&pctx, str_is_bin(name));
	sendf(&pctx, "%d", gc);
	sendf(&pctx, "%s", name);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_set_line_cap(int gc, char style)
{
	send_begin(&pctx, "cap");
	send_open(&pctx, 0);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, "%c", style);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_set_line_width(int gc, Coord width)
{
	send_begin(&pctx, "linwid");
	send_open(&pctx, 0);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, cfmt, width);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_set_draw_xor(int gc, int xor_set)
{
	send_begin(&pctx, "setxor");
	send_open(&pctx, 0);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, "%d", xor_set);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_draw_line(int gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
	send_begin(&pctx, "line");
	send_open(&pctx, 0);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, cfmt, x1);
	sendf(&pctx, cfmt, y1);
	sendf(&pctx, cfmt, x2);
	sendf(&pctx, cfmt, y2);
	send_close(&pctx);
	send_end(&pctx);
}


int remote_proto_parse()
{
	for(;;) {
		int c = getchar();
		if (c == EOF)
			return -1;
		parse_char(&pctx, c);
	}
}
