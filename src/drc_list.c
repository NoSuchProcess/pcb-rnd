/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#define TDL_DONT_UNDEF
#include "drc.h"
#include <genlist/gentdlist_impl.c>

void pcb_drc_free(pcb_drc_violation_t *item)
{
	pcb_drc_list_remove(item);
	free(item->title);
	free(item->explanation);
	free(item);
}

void pcb_drc_list_free_fields(pcb_drc_list_t *lst)
{
	for(;;) {
		pcb_drc_violation_t *item = pcb_drc_list_first(lst);
		if (item == NULL)
			break;
		pcb_drc_free(item);
	}
}

void pcb_drc_list_free(pcb_drc_list_t *lst)
{
	pcb_drc_list_free_fields(lst);
	free(lst);
}

pcb_drc_violation_t *pcb_drc_by_uid(const pcb_drc_list_t *lst, unsigned long int uid)
{
	pcb_drc_violation_t *v;

	for(v = pcb_drc_list_first((pcb_drc_list_t *)lst); v != NULL; v = pcb_drc_list_next(v))
		if (v->uid == uid)
			return v;

	return NULL;
}
