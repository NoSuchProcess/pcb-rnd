/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

#include <genlist/gendlist.h>
#include <librnd/core/error.h>
#include <genregex/regex_se.h>

#include "anyload.h"

typedef struct {
	const pcb_anyload_t *al;
	re_se_t *rx;
	const char *rx_str;
	gdl_elem_t link;
} pcb_aload_t;

static gdl_list_t anyloads;

int pcb_anyload_reg(const char *root_regex, const pcb_anyload_t *al_in)
{
	re_se_t *rx;
	pcb_aload_t *al;

	rx = re_se_comp(root_regex);
	if (rx == NULL) {
		rnd_message(RND_MSG_ERROR, "pcb_anyload_reg: failed to compile regex '%s' for '%s'\n", root_regex, al_in->cookie);
		return -1;
	}

	al = calloc(sizeof(pcb_aload_t), 1);
	al->al = al_in;
	al->rx = rx;
	al->rx_str = root_regex;

	gdl_append(&anyloads, al, link);

	return 0;
}

static void pcb_anyload_free(pcb_aload_t *al)
{
	gdl_remove(&anyloads, al, link);
	re_se_free(al->rx);
	free(al);
}

void pcb_anyload_unreg_by_cookie(const char *cookie)
{
	pcb_aload_t *al, *next;
	for(al = gdl_first(&anyloads); al != NULL; al = next) {
		next = al->link.next;
		if (al->al->cookie == cookie)
			pcb_anyload_free(al);
	}
}

void pcb_anyload_uninit(void)
{
	pcb_aload_t *al;
	while((al = gdl_first(&anyloads)) != NULL) {
		rnd_message(RND_MSG_ERROR, "pcb_anyload: '%s' left anyloader regs in\n", al->al->cookie);
		pcb_anyload_free(al);
	}
}

void pcb_anyload_init2(void)
{

}

