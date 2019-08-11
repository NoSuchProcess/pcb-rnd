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

#include <stdlib.h>
#include <genht/htsp.h>

#include "pixmap.h"
#include "pixmap_pcb.h"

void pcb_pixmap_hash_init(pcb_pixmap_hash_t *pmhash)
{
	htpp_init(&pmhash->meta, pcb_pixmap_hash_meta, pcb_pixmap_eq_meta);
	htpp_init(&pmhash->pixels, pcb_pixmap_hash_pixels, pcb_pixmap_eq_pixels);
}

void pcb_pixmap_hash_uninit(pcb_pixmap_hash_t *pmhash)
{
	htpp_uninit(&pmhash->meta);
	htpp_uninit(&pmhash->pixels);
}

pcb_pixmap_t *pcb_pixmap_insert_neutral_or_free(pcb_pixmap_hash_t *pmhash, pcb_pixmap_t *pm)
{
	if ((pm->tr_rot != 0) || pm->tr_xmirror || pm->tr_ymirror)
		return NULL;
	return NULL;
}

