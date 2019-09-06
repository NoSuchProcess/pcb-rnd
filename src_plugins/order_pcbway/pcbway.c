/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "safe_fs.h"
#include "../src_plugins/order/order.h"
#include "order_pcbway_conf.h"
#include "../src_plugins/order_pcbway/conf_internal.c"

conf_order_pcbway_t conf_order_pcbway;
#define ORDER_PCBWAY_CONF_FN "order_pcbway.conf"
#define CFG conf_order_pcbway.plugins.order_pcbway


static int pcbway_cache_update(pcb_hidlib_t *hidlib)
{
	double mt, age, now = pcb_dtime();
	char *path;
	path = pcb_strdup_printf("%s%cGetCountry", conf_order.plugins.order.cache, PCB_DIR_SEPARATOR_C);
	mt = pcb_file_mtime(hidlib, path);
	if ((mt < 0) || ((now - mt) > CFG.cache_update_sec)) {
		if (CFG.verbose)
			pcb_message(PCB_MSG_INFO, "pcbway: stale '%s', updating it in the cache\n", path);
	}
	else if (CFG.verbose)
		pcb_message(PCB_MSG_INFO, "pcbway: '%s' from cache\n", path);

	free(path);
}

static void pcbway_populate_dad(pcb_order_imp_t *imp, order_ctx_t *octx)
{
	if (pcbway_cache_update(&PCB->hidlib) != 0) {
		PCB_DAD_LABEL(octx->dlg, "Error: failed to update the cache.");
		return -1;
	}
	PCB_DAD_LABEL(octx->dlg, "pcbway!");
}

static pcb_order_imp_t pcbway = {
	"PCBWay",
	NULL,
	pcbway_populate_dad
};


int pplg_check_ver_order_pcbway(int ver_needed) { return 0; }

void pplg_uninit_order_pcbway(void)
{
	pcb_conf_unreg_file(ORDER_PCBWAY_CONF_FN, order_pcbway_conf_internal);
	pcb_conf_unreg_fields("plugins/order_pcbway/");
}

int pplg_init_order_pcbway(void)
{
	PCB_API_CHK_VER;

	pcb_conf_reg_file(ORDER_PCBWAY_CONF_FN, order_pcbway_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_order_pcbway, field,isarray,type_name,cpath,cname,desc,flags);
#include "order_pcbway_conf_fields.h"

	pcb_order_reg(&pcbway);
	return 0;
}
