/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  PADS IO plugin - plugin coordination
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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

#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid.h>
#include <librnd/core/compat_misc.h>
#include "plug_io.h"

#include "read.h"
#include "write.h"

static pcb_plug_io_t io_pads_9_4, io_pads_2005;
static const char *pads_cookie = "PADS IO";


int io_pads_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if ((strcmp(ctx->description, fmt) != 0) && (rnd_strcasecmp(fmt, "pads") != 0)) /* format name mismatch */
		return 0;

	if (((typ & (~(PCB_IOT_FOOTPRINT))) != 0) && ((typ & (~(PCB_IOT_PCB))) != 0)) /* support only footprints */
		return 0;

	if (wr)
		return 93;
	if (wr)
		return 0;


	return 100;
}

int pplg_check_ver_io_pads(int ver_needed) { return 0; }

void pplg_uninit_io_pads(void)
{
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pads_2005);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pads_9_4);
}

int pplg_init_io_pads(void)
{
	RND_API_CHK_VER;

	io_pads_2005.plugin_data = NULL;
	io_pads_2005.fmt_support_prio = io_pads_fmt;
	io_pads_2005.test_parse = io_pads_test_parse;
	io_pads_2005.parse_pcb = io_pads_parse_pcb;
/*	io_pads_2005.parse_footprint = io_pads_parse_footprint;
	io_pads_2005.map_footprint = io_pads_map_footprint;*/
	io_pads_2005.parse_font = NULL;
	io_pads_2005.write_buffer = NULL;
	io_pads_2005.write_pcb = io_pads_write_pcb_2005;
	io_pads_2005.default_fmt = "pads";
	io_pads_2005.description = "PADS ASCII board (V2005)";
	io_pads_2005.save_preference_prio = 61;
	io_pads_2005.default_extension = ".asc";
	io_pads_2005.fp_extension = ".asc";
	io_pads_2005.mime_type = "application/x-pads";
	io_pads_2005.multi_footprint = 1;

	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pads_2005);

	io_pads_9_4 = io_pads_2005;
	io_pads_9_4.description = "PADS ASCII board (V9.4)";
	io_pads_9_4.save_preference_prio = 63;
	io_pads_9_4.write_pcb = io_pads_write_pcb_9_4;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pads_9_4);

	return 0;
}

