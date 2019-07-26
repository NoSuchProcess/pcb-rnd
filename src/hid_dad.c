/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

/* widget-type-independent DAD functions */

#include "config.h"
#include "hid_dad.h"

void pcb_hid_dad_close(void *hid_ctx, pcb_dad_retovr_t *retovr, int retval)
{
	retovr->valid = 1;
	retovr->value = retval;
	pcb_gui->attr_dlg_free(hid_ctx);
}

void pcb_hid_dad_close_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_dad_retovr_t **retovr = attr->wdata;
	pcb_hid_dad_close(hid_ctx, *retovr, attr->val.lng);
}

int pcb_hid_dad_run(void *hid_ctx, pcb_dad_retovr_t *retovr)
{
	int ret;

	retovr->valid = 0;
	retovr->dont_free++;
	ret = pcb_gui->attr_dlg_run(hid_ctx);
	if (retovr->valid)
		ret = retovr->value;
	retovr->dont_free--;
	return ret;
}

