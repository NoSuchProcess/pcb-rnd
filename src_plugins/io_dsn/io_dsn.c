/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  dsn IO plugin - plugin coordination
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
#include <string.h>

#include "read.h"
#include "write.h"

#include "hid.h"
#include "compat_misc.h"
#include "plug_io.h"
#include "plugins.h"


static const char *dsn_cookie = "dsn IO";
static pcb_plug_io_t io_dsn;

int io_dsn_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((pcb_strcasecmp(fmt, "dsn") != 0) ||
		((typ & (~(PCB_IOT_PCB))) != 0))
		return 0;

	return 100;
}

int pplg_check_ver_io_dsn(int ver_needed) { return 0; }

void pplg_uninit_io_dsn(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_dsn);
}

#include "dolists.h"
int pplg_init_io_dsn(void)
{
	PCB_API_CHK_VER;

	/* register the IO hook */
	io_dsn.plugin_data = NULL;
	io_dsn.fmt_support_prio = io_dsn_fmt;
	io_dsn.test_parse = io_dsn_test_parse;
	io_dsn.parse_pcb = io_dsn_parse_pcb;
	io_dsn.parse_footprint = NULL;
	io_dsn.parse_font = NULL;
	io_dsn.write_buffer = NULL;
	io_dsn.write_pcb = io_dsn_write_pcb;
	io_dsn.default_fmt = "dsn";
	io_dsn.description = "specctra dsn";
	io_dsn.save_preference_prio = 20;
	io_dsn.default_extension = ".dsn";
	io_dsn.fp_extension = NULL;
	io_dsn.mime_type = "application/dsn";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_dsn);

	return 0;
}
