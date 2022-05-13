/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2022 Tibor 'Igor2' Palinkas
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
#include <librnd/core/hid.h>


#include "../export_ps/ps.h"
#include "export_lpr_conf.h"
#include "lpr_hid.h"

conf_export_lpr_t conf_export_lpr;

int pplg_check_ver_export_lpr(int ver_needed) { return 0; }


void pplg_uninit_export_lpr(void)
{
	rnd_lpr_uninit();
	rnd_conf_unreg_fields("plugins/export_lpr/");
}

int pplg_init_export_lpr(void)
{
	int res = rnd_lpr_init(&ps_hid, ps_ps_init, ps_hid_export_to_file, &conf_export_lpr.plugins.export_lpr.default_xcalib, &conf_export_lpr.plugins.export_lpr.default_ycalib);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_export_lpr, field,isarray,type_name,cpath,cname,desc,flags);
#include "export_lpr_conf_fields.h"

	return res;
}
