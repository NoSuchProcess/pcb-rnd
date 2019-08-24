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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "plugins.h"
#include "parse_common.h"
#include "file.h"

static pcb_plug_io_t io_pcb[3];
static io_pcb_ctx_t ctx[3];

pcb_plug_io_t *pcb_preferred_io_pcb, *pcb_nanometer_io_pcb, *pcb_centimil_io_pcb;

int io_pcb_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	/* All types are supported. */
	if (strcmp(fmt, "pcb") != 0)
		return 0;
	return 100; /* read-write */
}

extern void pcb_lex_destroy(void);
int pplg_check_ver_io_pcb(int ver_needed) { return 0; }

void pplg_uninit_io_pcb(void)
{
	int n;
	pcb_lex_destroy();

	for(n = 0; n < 3; n++)
		PCB_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &(io_pcb[n]));
}

int pplg_init_io_pcb(void)
{
	PCB_API_CHK_VER;

	memset(&io_pcb, 0, sizeof(io_pcb));

	/* register the IO hook */
	ctx[0].write_coord_fmt = pcb_printf_slot[8];
	io_pcb[0].plugin_data = &ctx[0];
	io_pcb[0].fmt_support_prio = io_pcb_fmt;
	io_pcb[0].test_parse = io_pcb_test_parse;
	io_pcb[0].parse_pcb = io_pcb_ParsePCB;
	io_pcb[0].parse_footprint = io_pcb_ParseElement;
	io_pcb[0].parse_font = io_pcb_ParseFont;
	io_pcb[0].write_buffer = NULL;
	io_pcb[0].write_footprint = io_pcb_WriteSubcData;
	io_pcb[0].write_pcb = io_pcb_WritePCB;
	io_pcb[0].default_fmt = "pcb";
	io_pcb[0].description = "geda/pcb - mainline (centimils)";
	io_pcb[0].save_preference_prio = 89;
	io_pcb[0].default_extension = ".pcb";
	io_pcb[0].fp_extension = ".fp";
	io_pcb[0].mime_type = "application/x-pcb-layout";
	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &(io_pcb[0]));
	pcb_centimil_io_pcb = &io_pcb[0];

	ctx[1].write_coord_fmt = pcb_printf_slot[9];
	io_pcb[1].plugin_data = &ctx[1];
	io_pcb[1].fmt_support_prio = io_pcb_fmt;
	io_pcb[1].write_buffer = NULL;
	io_pcb[1].write_footprint = io_pcb_WriteSubcData;
	io_pcb[1].write_pcb = io_pcb_WritePCB;
	io_pcb[1].default_fmt = "pcb";
	io_pcb[1].description = "geda/pcb - readable units";
	io_pcb[1].save_preference_prio = 90;
	io_pcb[1].default_extension = ".pcb";
	io_pcb[1].fp_extension = ".fp";
	io_pcb[1].mime_type = "application/x-pcb-layout";
	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &(io_pcb[1]));
	pcb_preferred_io_pcb = &io_pcb[1];

	ctx[2].write_coord_fmt = "%$$mn";
	io_pcb[2].plugin_data = &ctx[2];
	io_pcb[2].fmt_support_prio = io_pcb_fmt;
	io_pcb[2].write_buffer = NULL;
	io_pcb[2].write_footprint = io_pcb_WriteSubcData;
	io_pcb[2].write_pcb = io_pcb_WritePCB;
	io_pcb[2].default_fmt = "pcb";
	io_pcb[2].description = "geda/pcb - nanometer";
	io_pcb[2].save_preference_prio = 88;
	io_pcb[2].default_extension = ".pcb";
	io_pcb[2].fp_extension = ".fp";
	io_pcb[2].mime_type = "application/x-pcb-layout";
	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &(io_pcb[2]));
	pcb_nanometer_io_pcb = &io_pcb[2];


	return 0;
}
