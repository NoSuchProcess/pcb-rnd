/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET in 2020)
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

#include <librnd/core/plugins.h>
#include <librnd/core/error.h>

#include "drc.h"
#include "view.h"
#include "conf_core.h"
#include "find.h"
#include "event.h"


static const char *drc_query_cookie = "drc_query";

static void pcb_drc_query(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_message(PCB_MSG_ERROR, "drc_query: not yet implemented\n");
}

int pplg_check_ver_drc_query(int ver_needed) { return 0; }

void pplg_uninit_drc_query(void)
{
	pcb_event_unbind_allcookie(drc_query_cookie);
}

int pplg_init_drc_query(void)
{
	PCB_API_CHK_VER;
	pcb_event_bind(PCB_EVENT_DRC_RUN, pcb_drc_query, NULL, drc_query_cookie);
	return 0;
}

