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

pcb_plug_io_t plug_io_lihata_v1, plug_io_lihata_v2, plug_io_lihata_v3;
conf_io_lihata_t conf_io_lihata;

int io_lihata_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	int lih = (strcmp(fmt, "lihata") == 0);

	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((lih) && (typ & PCB_IOT_BUFFER) && (ctx->write_buffer != NULL))
		return 40;

/*PCB_IOT_FOOTPRINT |*/

	if (!lih || ((typ & (~(PCB_IOT_PCB | PCB_IOT_FONT))) != 0))
		return 0;

	if (wr)
		return ctx->save_preference_prio;
	return 100;
}

int pplg_check_ver_io_lihata(int ver_needed) { return 0; }

void pplg_uninit_io_lihata(void)
{
	conf_unreg_fields("plugins/io_lihata/");
}

int pplg_init_io_lihata(void)
{

	/* register the IO hook */
	plug_io_lihata_v3.plugin_data = NULL;
	plug_io_lihata_v3.fmt_support_prio = io_lihata_fmt;
	plug_io_lihata_v3.test_parse_pcb = io_lihata_test_parse_pcb;
	plug_io_lihata_v3.parse_pcb = io_lihata_parse_pcb;
	plug_io_lihata_v3.parse_element = io_lihata_parse_element;
	plug_io_lihata_v3.parse_font = io_lihata_parse_font;
	plug_io_lihata_v3.write_font = io_lihata_write_font;
	plug_io_lihata_v3.write_buffer = io_lihata_write_buffer;
	plug_io_lihata_v3.write_element = NULL;
	plug_io_lihata_v3.write_pcb = io_lihata_write_pcb_v3;
	plug_io_lihata_v3.default_fmt = "lihata";
	plug_io_lihata_v3.description = "lihata board v3";
	plug_io_lihata_v3.save_preference_prio = 40;
	plug_io_lihata_v3.default_extension = ".lht";
	plug_io_lihata_v3.fp_extension = ".lht";
	plug_io_lihata_v3.mime_type = "application/x-pcbrnd-board";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v3);

	plug_io_lihata_v2.plugin_data = NULL;
	plug_io_lihata_v2.fmt_support_prio = io_lihata_fmt;
	plug_io_lihata_v2.test_parse_pcb = io_lihata_test_parse_pcb;
	plug_io_lihata_v2.parse_pcb = io_lihata_parse_pcb;
	plug_io_lihata_v2.parse_element = NULL;
	plug_io_lihata_v2.parse_font = io_lihata_parse_font;
	plug_io_lihata_v2.write_font = io_lihata_write_font;
	plug_io_lihata_v2.write_buffer = NULL;
	plug_io_lihata_v2.write_element = NULL;
	plug_io_lihata_v2.write_pcb = io_lihata_write_pcb_v2;
	plug_io_lihata_v2.default_fmt = "lihata";
	plug_io_lihata_v2.description = "lihata board v2";
	plug_io_lihata_v2.save_preference_prio = 199;
	plug_io_lihata_v2.default_extension = ".lht";
	plug_io_lihata_v2.fp_extension = ".lht";
	plug_io_lihata_v2.mime_type = "application/x-pcbrnd-board";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v2);

	plug_io_lihata_v1.plugin_data = NULL;
	plug_io_lihata_v1.fmt_support_prio = io_lihata_fmt;
	plug_io_lihata_v1.test_parse_pcb = io_lihata_test_parse_pcb;
	plug_io_lihata_v1.parse_pcb = io_lihata_parse_pcb;
	plug_io_lihata_v1.parse_element = NULL;
	plug_io_lihata_v1.parse_font = io_lihata_parse_font;
	plug_io_lihata_v1.write_font = io_lihata_write_font;
	plug_io_lihata_v1.write_buffer = NULL;
	plug_io_lihata_v1.write_element = NULL;
	plug_io_lihata_v1.write_pcb = io_lihata_write_pcb_v1;
	plug_io_lihata_v1.default_fmt = "lihata";
	plug_io_lihata_v1.description = "lihata board v1";
	plug_io_lihata_v1.save_preference_prio = 100;
	plug_io_lihata_v1.default_extension = ".lht";
	plug_io_lihata_v1.fp_extension = ".lht";
	plug_io_lihata_v1.mime_type = "application/x-pcbrnd-board";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v1);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_io_lihata, field,isarray,type_name,cpath,cname,desc,flags);
#include "lht_conf_fields.h"

	return 0;
}

