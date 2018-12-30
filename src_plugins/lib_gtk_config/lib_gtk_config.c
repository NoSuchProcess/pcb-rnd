/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include <stdlib.h>
#include "lib_gtk_config.h"
#include "hid_gtk_conf.h"
#include "plugins.h"

static const char *lib_gtk_config_cookie = "lib_gtk_config";

conf_hid_id_t ghid_conf_id = -1;
conf_hid_gtk_t conf_hid_gtk;

void pcb_gtk_conf_init(void)
{
	ghid_conf_id = conf_hid_reg(lib_gtk_config_cookie, NULL);
}

int pplg_check_ver_lib_gtk_config(int ver_needed) { return 0; }

void pplg_uninit_lib_gtk_config(void)
{
	conf_hid_unreg(lib_gtk_config_cookie);
	conf_unreg_fields("plugins/hid_gtk/");
}

int pplg_init_lib_gtk_config(void)
{
	PCB_API_CHK_VER;
	pcb_gtk_conf_init();

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_hid_gtk, field,isarray,type_name,cpath,cname,desc,flags);
#include "../src_plugins/lib_gtk_config/hid_gtk_conf_fields.h"

	return 0;
}

