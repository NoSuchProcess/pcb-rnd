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

/* pcb-rnd specific parts (how to store and look up pixmaps) */

#ifndef PCB_PIXMAP_PCB_H
#define PCB_PIXMAP_PCB_H

#include "global_typedefs.h"
#include <genht/htpp.h>

#include "pixmap.h"

typedef struct pcb_pixmap_hash_s {
	htpp_t meta;      /* all pixmaps, hashed and compared only by metadata (including natural_oid) - used to look up a specific transformed version */
	htpp_t pixels;    /* all pixmaps, hashed and compared by the pixels (but not natural_oid) - used to look up a specific pixel array */
} pcb_pixmap_hash_t;


void pcb_pixmap_hash_init(pcb_pixmap_hash_t *pmhash);
void pcb_pixmap_hash_uninit(pcb_pixmap_hash_t *pmhash);

/* Either inserts pm in the board's pixmap hash, if pm is new there, or frees
   pm and returns the matching pixmap from the board's pixmap hash. pm must
   be in neutral position (no rotation, no mirror). Returns NULL on error. */
pcb_pixmap_t *pcb_pixmap_insert_neutral_or_free(pcb_pixmap_hash_t *pmhash, pcb_pixmap_t *pm);

#endif
