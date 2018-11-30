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
#include "config.h"

#define TDL_DONT_UNDEF
#include "view.h"
#include <genlist/gentdlist_impl.c>
#undef TDL_DONT_UNDEF
#include <genlist/gentdlist_undef.h>


#include "actions.h"


void pcb_view_free(pcb_view_t *item)
{
	pcb_view_list_remove(item);
	pcb_idpath_list_clear(&item->objs[0]);
	pcb_idpath_list_clear(&item->objs[1]);
	free(item->title);
	free(item->explanation);
	free(item);
}

void pcb_view_list_free_fields(pcb_view_list_t *lst)
{
	for(;;) {
		pcb_view_t *item = pcb_view_list_first(lst);
		if (item == NULL)
			break;
		pcb_view_free(item);
	}
}

void pcb_view_list_free(pcb_view_list_t *lst)
{
	pcb_view_list_free_fields(lst);
	free(lst);
}

pcb_view_t *pcb_view_by_uid(const pcb_view_list_t *lst, unsigned long int uid)
{
	pcb_view_t *v;

	for(v = pcb_view_list_first((pcb_view_list_t *)lst); v != NULL; v = pcb_view_list_next(v))
		if (v->uid == uid)
			return v;

	return NULL;
}

void pcb_view_goto(pcb_view_t *item)
{
	if (item->have_coord) {
		fgw_arg_t res, argv[5];

		argv[1].type = FGW_COORD; fgw_coord(&argv[1]) = item->bbox.X1;
		argv[2].type = FGW_COORD; fgw_coord(&argv[2]) = item->bbox.Y1;
		argv[3].type = FGW_COORD; fgw_coord(&argv[3]) = item->bbox.X2;
		argv[4].type = FGW_COORD; fgw_coord(&argv[4]) = item->bbox.Y2;
		pcb_actionv_bin("zoom", &res, 5, argv);
	}
}

