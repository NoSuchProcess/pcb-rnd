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

static pcb_plug_io_t io_pads;
static const char *pads_cookie = "PADS IO";


int io_pads_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if ((strcmp(ctx->description, fmt) != 0) && (rnd_strcasecmp(fmt, "pads") != 0)) /* format name mismatch */
		return 0;

	if (((typ & (~(PCB_IOT_FOOTPRINT))) != 0) && ((typ & (~(PCB_IOT_PCB))) != 0)) /* support only footprints */
		return 0;

	if (wr) /* no footprint write yet */
		return 0;

	return 100;
}

int pplg_check_ver_io_pads(int ver_needed) { return 0; }

void pplg_uninit_io_pads(void)
{
	rnd_remove_actions_by_cookie(pads_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pads);
}

int pplg_init_io_pads(void)
{
	RND_API_CHK_VER;

	io_pads.plugin_data = NULL;
	io_pads.fmt_support_prio = io_pads_fmt;
	io_pads.test_parse = io_pads_test_parse;
	io_pads.parse_pcb = io_pads_parse_pcb;
/*	io_pads.parse_footprint = io_pads_parse_footprint;
	io_pads.map_footprint = io_pads_map_footprint;*/
	io_pads.parse_font = NULL;
	io_pads.write_buffer = NULL;
	io_pads.write_pcb = NULL;
	io_pads.default_fmt = "pads";
	io_pads.description = "pads board";
	io_pads.save_preference_prio = 91;
	io_pads.default_extension = ".asc";
	io_pads.fp_extension = ".asc";
	io_pads.mime_type = "application/x-pads";
	io_pads.multi_footprint = 1;

	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pads);

	return 0;
}

