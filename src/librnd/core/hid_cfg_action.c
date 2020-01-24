/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
#include <librnd/core/hid_cfg_action.h>
#include <librnd/core/actions.h>

int pcb_hid_cfg_action(pcb_hidlib_t *hl, const lht_node_t *node)
{
	if (node == NULL)
		return -1;
	switch(node->type) {
		case LHT_TEXT:
			return pcb_parse_actions(hl, node->data.text.value);
		case LHT_LIST:
			for(node = node->data.list.first; node != NULL; node = node->next) {
				if (node->type == LHT_TEXT) {
					if (pcb_parse_actions(hl, node->data.text.value) != 0)
						return -1;
				}
				else
					return -1;
			}
			return 0;
		default: ; /* suppress compiler warning: can't handle any other type */
	}
	return -1;
}
