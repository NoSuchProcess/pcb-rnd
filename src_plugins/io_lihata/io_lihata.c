/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016..2021 Tibor 'Igor2' Palinkas
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
#include "plug_io.h"
#include "read.h"
#include "write.h"
#include "io_lihata.h"

pcb_plug_io_t plug_io_lihata_v1, plug_io_lihata_v2, plug_io_lihata_v3,
              plug_io_lihata_v4, plug_io_lihata_v5, plug_io_lihata_v6,
              plug_io_lihata_v7, plug_io_lihata_v8, plug_io_lihata_v9;
conf_io_lihata_t conf_io_lihata;

pcb_plug_io_t *plug_io_lihata_default = &plug_io_lihata_v9;

int io_lihata_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	int lih = (strncmp(fmt, "lihata", 6) == 0);

	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((lih) && (typ & PCB_IOT_BUFFER)) {
		if (wr && (ctx->write_buffer != NULL))
			return 40;
		if (!wr && (ctx->parse_buffer != NULL))
			return 40;
	}

	if ((lih) && (typ & PCB_IOT_PADSTACK)) {
		if (wr && (ctx->write_padstack != NULL))
			return 100;
		if (!wr && (ctx->parse_padstack != NULL))
			return 100;
	}

	if (!lih || ((typ & (~(PCB_IOT_PCB | PCB_IOT_FONT | PCB_IOT_FOOTPRINT))) != 0))
		return 0;

	if (wr)
		return ctx->save_preference_prio;
	return 100;
}

int pplg_check_ver_io_lihata(int ver_needed) { return 0; }

void pplg_uninit_io_lihata(void)
{
	rnd_conf_unreg_fields("plugins/io_lihata/");
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v9);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v8);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v7);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v6);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v5);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v4);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v3);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v2);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v1);
}

int pplg_init_io_lihata(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */


	plug_io_lihata_v7.plugin_data = NULL;
	plug_io_lihata_v7.fmt_support_prio = io_lihata_fmt;
	plug_io_lihata_v7.test_parse = io_lihata_test_parse;
	plug_io_lihata_v7.parse_pcb = io_lihata_parse_pcb;
	plug_io_lihata_v7.parse_footprint = io_lihata_parse_subc;
	plug_io_lihata_v7.parse_font = io_lihata_parse_font;
	plug_io_lihata_v7.parse_buffer = io_lihata_parse_buffer;
	plug_io_lihata_v7.parse_padstack = io_lihata_parse_padstack;
	plug_io_lihata_v7.write_font = io_lihata_write_font;
	plug_io_lihata_v7.write_buffer = io_lihata_write_buffer;
	plug_io_lihata_v7.write_padstack = io_lihata_write_padstack;
	plug_io_lihata_v7.write_subcs_head = io_lihata_write_subcs_head;
	plug_io_lihata_v7.write_subcs_subc = io_lihata_write_subcs_subc;
	plug_io_lihata_v7.write_subcs_tail = io_lihata_write_subcs_tail;
	plug_io_lihata_v7.write_pcb = io_lihata_write_pcb_v7;
	plug_io_lihata_v7.default_fmt = "lihata";
	plug_io_lihata_v7.description = "lihata board v7";
	plug_io_lihata_v7.save_preference_prio = 197;
	plug_io_lihata_v7.default_extension = ".rp";
	plug_io_lihata_v7.alternate_extension = ".lht";
	plug_io_lihata_v7.fp_extension = ".rf";
	plug_io_lihata_v7.mime_type = "application/x-pcbrnd-board";
	plug_io_lihata_v7.save_as_subd_init = io_lihata_save_as_subd_init;
	plug_io_lihata_v7.save_as_subd_uninit = io_lihata_save_as_subd_uninit;
	plug_io_lihata_v7.save_as_fmt_changed = io_lihata_save_as_fmt_changed;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v7);


	plug_io_lihata_v8 = plug_io_lihata_v7;
	plug_io_lihata_v8.write_pcb = io_lihata_write_pcb_v8;
	plug_io_lihata_v8.description = "lihata board v8";
	plug_io_lihata_v8.save_preference_prio = 198;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v8);

	plug_io_lihata_v9 = plug_io_lihata_v7;
	plug_io_lihata_v9.write_pcb = io_lihata_write_pcb_v9;
	plug_io_lihata_v9.description = "lihata board v9";
	plug_io_lihata_v9.save_preference_prio = 199;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v9);


	plug_io_lihata_v6 = plug_io_lihata_v7;
	plug_io_lihata_v6.write_pcb = io_lihata_write_pcb_v6;
	plug_io_lihata_v6.description = "lihata board v6";
	plug_io_lihata_v6.save_preference_prio = 100;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v6);


	plug_io_lihata_v5 = plug_io_lihata_v7;
	plug_io_lihata_v5.write_pcb = io_lihata_write_pcb_v5;
	plug_io_lihata_v5.description = "lihata board v5";
	plug_io_lihata_v5.save_preference_prio = 100;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v5);

	plug_io_lihata_v4 = plug_io_lihata_v7;
	plug_io_lihata_v4.write_pcb = io_lihata_write_pcb_v4;
	plug_io_lihata_v4.description = "lihata board v4";
	plug_io_lihata_v4.save_preference_prio = 100;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v4);

	plug_io_lihata_v3 = plug_io_lihata_v7;
	plug_io_lihata_v3.write_pcb = io_lihata_write_pcb_v3;
	plug_io_lihata_v3.description = "lihata board v3";
	plug_io_lihata_v3.save_preference_prio = 100;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v3);

	plug_io_lihata_v2 = plug_io_lihata_v7;
	plug_io_lihata_v2.write_pcb = io_lihata_write_pcb_v2;
	plug_io_lihata_v2.description = "lihata board v2";
	plug_io_lihata_v2.save_preference_prio = 100;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v2);

	plug_io_lihata_v1 = plug_io_lihata_v7;
	plug_io_lihata_v1.write_pcb = io_lihata_write_pcb_v1;
	plug_io_lihata_v1.map_footprint = io_lihata_map_footprint; /* it is enough to have this for one version so the check is done only once */
	plug_io_lihata_v1.write_pcb = io_lihata_write_pcb_v1;
	plug_io_lihata_v1.description = "lihata board v1";
	plug_io_lihata_v1.save_preference_prio = 100;
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &plug_io_lihata_v1);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_io_lihata, field,isarray,type_name,cpath,cname,desc,flags);
#include "lht_conf_fields.h"

	return 0;
}

