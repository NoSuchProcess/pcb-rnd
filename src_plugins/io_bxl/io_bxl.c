/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  BXL IO plugin - plugin coordination
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

static pcb_plug_io_t io_bxl;
static const char *bxl_cookie = "bxl IO";


int io_bxl_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if ((strcmp(ctx->description, fmt) != 0) && (pcb_strcasecmp(fmt, "bxl") != 0)) /* format name mismatch */
		return 0;

	if ((typ & (~(PCB_IOT_FOOTPRINT))) != 0) /* support only footprints */
		return 0;

	if (wr) /* no footprint write yet */
		return 0;

	return 100;
}

int pplg_check_ver_io_bxl(int ver_needed) { return 0; }

void pplg_uninit_io_bxl(void)
{
	pcb_remove_actions_by_cookie(bxl_cookie);
	PCB_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_bxl);
}

int pplg_init_io_bxl(void)
{
	PCB_API_CHK_VER;

	io_bxl.plugin_data = NULL;
	io_bxl.fmt_support_prio = io_bxl_fmt;
	io_bxl.test_parse = io_bxl_test_parse;
	io_bxl.parse_pcb = NULL;
	io_bxl.parse_footprint = io_bxl_parse_footprint;
	io_bxl.parse_font = NULL;
	io_bxl.write_buffer = NULL;
	io_bxl.write_pcb = NULL;
	io_bxl.default_fmt = "bxl";
	io_bxl.description = "bxl footprint";
	io_bxl.save_preference_prio = 90;
	io_bxl.default_extension = ".bxl";
	io_bxl.fp_extension = ".bxl";
	io_bxl.mime_type = "application/x-bxl";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_bxl);

	return 0;
}

