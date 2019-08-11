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

#ifndef PCB_HID_PIXMAP_H
#define PCB_HID_PIXMAP_H

#include "global_typedefs.h"
#include <genht/htpp.h>

struct pcb_pixmap_s {
	long size;                 /* total size of the array in memory (sx*sy*3) */
	long sx, sy;               /* x and y dimensions */
	unsigned char tr, tg, tb;  /* color of the transparent pixel */
	unsigned int hash;         /* precalculated hash value */
	unsigned char *p;          /* pixel array in r,g,b rows of sx long each */
	unsigned long neutral_oid; /* UID of the pixmap in neutral position */

	void *hid_data;            /* HID's version of the pixmap */

	/* transformation info */
	pcb_angle_t tr_rot;        /* rotation angle (0 if not transformed) */
	unsigned tr_xmirror:1;     /* whether the pixmap is mirrored along the x axis (vertical mirror) */
	unsigned tr_ymirror:1;     /* whether the pixmap is mirrored along the y axis (horizontal mirror) */

	unsigned transp_valid:1;   /* 1 if transparent pixel is available */
	unsigned hash_valid:1;     /* 1 if the has value has been calculated */
	unsigned hid_data_valid:1; /* 1 if hid_data is already generated and no data changed since - maintained by core, HIDs don't need to check */
};

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
