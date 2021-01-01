/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  delayed creation of objects
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2021)
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

#include <genht/hash.h>

#include "delay_create.h"

void pcb_dlcr_init(pcb_dlcr_t *dlcr)
{
	memset(dlcr, 0, sizeof(pcb_dlcr_t));
	htsp_init(&dlcr->name2layer, strhash, strkeyeq);
}

void pcb_dlcr_uninit(pcb_dlcr_t *dlcr)
{
	TODO("free everything");
}

void pcb_dlcr_layer_reg(pcb_dlcr_t *dlcr, pcb_dlcr_layer_t *layer)
{
	pcb_dlcr_layer_t **p;

	htsp_set(&dlcr->name2layer, layer->name, layer);
	p = (pcb_dlcr_layer_t **)vtp0_get(&dlcr->id2layer, layer->id, 1);
	*p = layer;
}


void pcb_dlcr_layer_free(pcb_dlcr_layer_t *layer)
{
	free(layer->name);
	free(layer);
}

