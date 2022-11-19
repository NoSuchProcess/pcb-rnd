/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Altium IO plugin - plugin coordination
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
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
#include <librnd/hid/hid.h>
#include <librnd/core/compat_misc.h>
#include "plug_io.h"

#include "pcbdoc_ascii.h"
#include "pcbdoc_bin.h"
#include "pcbdoc.h"

static pcb_plug_io_t io_pcbdoc_ascii;
static pcb_plug_io_t io_pcbdoc_bin;
static const char *altium_cookie = "Altium IO";


int io_altium_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if ((strcmp(ctx->description, fmt) != 0) && (rnd_strcasecmp(fmt, "altium") != 0)) /* format name mismatch */
		return 0;

	if ((typ & (~(PCB_IOT_PCB))) != 0) /* support only boards */
		return 0;

	if (wr)
		return 0;

	return 100;
}

int pplg_check_ver_io_altium(int ver_needed) { return 0; }

void pplg_uninit_io_altium(void)
{
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pcbdoc_ascii);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pcbdoc_bin);
}

int pplg_init_io_altium(void)
{
	RND_API_CHK_VER;

	io_pcbdoc_ascii.plugin_data = NULL;
	io_pcbdoc_ascii.fmt_support_prio = io_altium_fmt;
	io_pcbdoc_ascii.test_parse = pcbdoc_ascii_test_parse;
	io_pcbdoc_ascii.parse_pcb = io_altium_parse_pcbdoc_ascii;
/*	io_pcbdoc_ascii.parse_footprint = io_altium_parse_footprint;
	io_pcbdoc_ascii.map_footprint = io_altium_map_footprint;*/
	io_pcbdoc_ascii.parse_font = NULL;
	io_pcbdoc_ascii.write_buffer = NULL;
	io_pcbdoc_ascii.write_pcb = NULL;
	io_pcbdoc_ascii.default_fmt = "altium";
	io_pcbdoc_ascii.description = "Protel/Altium PcbDoc ASCII board";
	io_pcbdoc_ascii.save_preference_prio = 1;
	io_pcbdoc_ascii.default_extension = ".PcbDoc";
	io_pcbdoc_ascii.fp_extension = NULL;
	io_pcbdoc_ascii.mime_type = "application/x-altium";
	io_pcbdoc_ascii.multi_footprint = 0;

	io_pcbdoc_bin = io_pcbdoc_ascii;
	io_pcbdoc_bin.description = "Altium PcbDoc binary board (v6)";
	io_pcbdoc_bin.test_parse = pcbdoc_bin_test_parse;
	io_pcbdoc_bin.parse_pcb = io_altium_parse_pcbdoc_bin;

	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pcbdoc_ascii);
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_pcbdoc_bin);

	return 0;
}

