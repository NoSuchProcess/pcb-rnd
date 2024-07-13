/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - plugin coordination
 *  pcb-rnd Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 Entrust Fund in 2024)
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
#include <librnd/core/hidlib.h>
#include <librnd/core/conf_multi.h>
#include <librnd/core/compat_misc.h>
#include "plug_io.h"
#include "io_easyeda_conf.h"
#include "../src_plugins/io_easyeda/conf_internal.c"

conf_io_easyeda_t conf_io_easyeda;

#include "read_std.h"
#include "read_pro.h"

static pcb_plug_io_t io_easyeda_std, io_easyeda_pro;
static const char *easyeda_cookie = "EasyEDA IO";


int io_easyeda_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if ((strcmp(ctx->description, fmt) != 0) && (strcmp(ctx->default_fmt, fmt) != 0) && (rnd_strcasecmp(fmt, "easyeda") != 0)) /* format name mismatch */
		return 0;

	if (wr)
		return 0;

	if ((typ & (PCB_IOT_FOOTPRINT | PCB_IOT_PCB)) != 0) /* support only footprints and boards */
		return 100;

	return 0;
}

int pplg_check_ver_io_easyeda(int ver_needed) { return 0; }

void pplg_uninit_io_easyeda(void)
{
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_easyeda_std);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_easyeda_pro);
	rnd_conf_plug_unreg("plugins/io_easyeda/", io_easyeda_conf_internal, easyeda_cookie);
}

int pplg_init_io_easyeda(void)
{
	RND_API_CHK_VER;

	io_easyeda_std.plugin_data = NULL;
	io_easyeda_std.fmt_support_prio = io_easyeda_fmt;
	io_easyeda_std.test_parse = io_easyeda_std_test_parse;
	io_easyeda_std.parse_pcb = io_easyeda_std_parse_pcb;
	io_easyeda_std.parse_footprint = io_easyeda_std_parse_footprint;
	io_easyeda_std.map_footprint = io_easyeda_std_map_footprint;
	io_easyeda_std.parse_font = NULL;
	io_easyeda_std.write_buffer = NULL;
	io_easyeda_std.write_pcb = NULL;
	io_easyeda_std.default_fmt = "easyeda std";
	io_easyeda_std.description = "EasyEDA std board";
	io_easyeda_std.save_preference_prio = 61;
	io_easyeda_std.default_extension = ".json";
	io_easyeda_std.fp_extension = ".json";
	io_easyeda_std.mime_type = "application/x-easyeda";
	io_easyeda_std.multi_footprint = 1;

	io_easyeda_pro.plugin_data = NULL;
	io_easyeda_pro.fmt_support_prio = io_easyeda_fmt;
	io_easyeda_pro.test_parse = io_easyeda_pro_test_parse;
	io_easyeda_pro.parse_pcb = io_easyeda_pro_parse_pcb;
	io_easyeda_pro.parse_footprint = io_easyeda_pro_parse_footprint;
	io_easyeda_pro.map_footprint = io_easyeda_pro_map_footprint;
	io_easyeda_pro.parse_font = NULL;
	io_easyeda_pro.write_buffer = NULL;
	io_easyeda_pro.write_pcb = NULL;
	io_easyeda_pro.default_fmt = "easyeda pro";
	io_easyeda_pro.description = "EasyEDA pro board";
	io_easyeda_pro.save_preference_prio = 61;
	io_easyeda_pro.default_extension = ".json"; TODO("revise this");
	io_easyeda_pro.fp_extension = ".json"; TODO("revise this");
	io_easyeda_pro.mime_type = "application/x-easyeda";
	io_easyeda_pro.multi_footprint = 1;

	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_easyeda_std);
	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_easyeda_pro);

rnd_conf_plug_reg(conf_io_easyeda, io_easyeda_conf_internal, easyeda_cookie);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_io_easyeda, field,isarray,type_name,cpath,cname,desc,flags);
#include "io_easyeda_conf_fields.h"

	return 0;
}

