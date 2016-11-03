/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
#include "plugins.h"
#include "plug_io.h"
#include "read.h"
#include "write.h"
#include "io_lihata.h"

static plug_io_t io_lihata;
conf_io_lihata_t conf_io_lihata;

int io_lihata_fmt(plug_io_t *ctx, plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((strcmp(fmt, "lihata") != 0) ||
		((typ & (~(/*PCB_IOT_FOOTPRINT | PCB_IOT_BUFFER |*/ PCB_IOT_PCB))) != 0))
		return 0;
	if (wr)
		return 100;
	return 100;
}

static void hid_io_lihata_uninit(void)
{
}

pcb_uninit_t hid_io_lihata_init(void)
{

	/* register the IO hook */
	io_lihata.plugin_data = NULL;
	io_lihata.fmt_support_prio = io_lihata_fmt;
	io_lihata.parse_pcb = io_lihata_parse_pcb;
	io_lihata.parse_element = NULL;
	io_lihata.parse_font = NULL;
	io_lihata.write_buffer = NULL;
	io_lihata.write_element = NULL;
	io_lihata.write_pcb = io_lihata_write_pcb;
	io_lihata.default_fmt = "lihata";
	io_lihata.description = "lihata board";
	io_lihata.save_preference_prio = 20;

	HOOK_REGISTER(plug_io_t, plug_io_chain, &io_lihata);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_io_lihata, field,isarray,type_name,cpath,cname,desc,flags);
#include "lht_conf_fields.h"

	return hid_io_lihata_uninit;
}

