/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, io_pcb, was written and is Copyright (C) 2016 by Tibor Palinkas
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

#include "config.h"
#include "global.h"
#include "plugins.h"
#include "parse_l.h"
#include "file.h"

static plug_io_t io_pcb[3];
static io_pcb_ctx_t ctx[3];

int io_pcb_fmt(plug_io_t *ctx, plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	/* All types are supported. */
	if (strcmp(fmt, "pcb") != 0)
		return 0;
	return 100; /* read-write */
}

extern void yylex_destroy(void);
static void hid_io_pcb_uninit(void)
{
	int n;
	yylex_destroy();

	for(n = 0; n < 3; n++)
		HOOK_UNREGISTER(plug_io_t, plug_io_chain, &(io_pcb[n]));
}

pcb_uninit_t hid_io_pcb_init(void)
{

	memset(&io_pcb, 0, sizeof(io_pcb));


	/* register the IO hook */
	ctx[0].write_coord_fmt = pcb_printf_slot[8];
	io_pcb[0].plugin_data = &ctx[0];
	io_pcb[0].fmt_support_prio = io_pcb_fmt;
	io_pcb[0].parse_pcb = io_pcb_ParsePCB;
	io_pcb[0].parse_element = io_pcb_ParseElement;
	io_pcb[0].parse_font = io_pcb_ParseFont;
	io_pcb[0].write_buffer = io_pcb_WriteBuffer;
	io_pcb[0].write_element = io_pcb_WriteElementData;
	io_pcb[0].write_pcb = io_pcb_WritePCB;
	io_pcb[0].default_fmt = "pcb";
	io_pcb[0].description = "geda/pcb - mainline (centimils)";
	io_pcb[0].save_preference_prio = 100;
	HOOK_REGISTER(plug_io_t, plug_io_chain, &(io_pcb[0]));

	ctx[1].write_coord_fmt = pcb_printf_slot[9];
	io_pcb[1].plugin_data = &ctx[1];
	io_pcb[1].fmt_support_prio = io_pcb_fmt;
	io_pcb[1].write_buffer = io_pcb_WriteBuffer;
	io_pcb[1].write_element = io_pcb_WriteElementData;
	io_pcb[1].write_pcb = io_pcb_WritePCB;
	io_pcb[1].default_fmt = "pcb";
	io_pcb[1].description = "geda/pcb - readable units";
	io_pcb[1].save_preference_prio = 99;
	HOOK_REGISTER(plug_io_t, plug_io_chain, &(io_pcb[1]));

	ctx[2].write_coord_fmt = "%$$mn";
	io_pcb[2].plugin_data = &ctx[2];
	io_pcb[2].fmt_support_prio = io_pcb_fmt;
	io_pcb[2].write_buffer = io_pcb_WriteBuffer;
	io_pcb[2].write_element = io_pcb_WriteElementData;
	io_pcb[2].write_pcb = io_pcb_WritePCB;
	io_pcb[2].default_fmt = "pcb";
	io_pcb[2].description = "geda/pcb - nanometer";
	io_pcb[2].save_preference_prio = 98;
	HOOK_REGISTER(plug_io_t, plug_io_chain, &(io_pcb[2]));

	return hid_io_pcb_uninit;
}
