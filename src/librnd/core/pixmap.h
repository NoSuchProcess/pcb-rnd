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

/* generic pixmap (low level, part of the hidlib): draw, calculate hash, compare */

#ifndef PCB_PIXMAP_H
#define PCB_PIXMAP_H

#include <librnd/core/global_typedefs.h>

typedef struct pcb_pixmap_import_s pcb_pixmap_import_t;

struct pcb_pixmap_import_s {
	const char *name;
	int (*load)(pcb_hidlib_t *hidlib, pcb_pixmap_t *pxm, const char *fn);

	/* filled in by code */
	pcb_pixmap_import_t *next;
	const char *cookie;
};

struct pcb_pixmap_s {
	long size;                 /* total size of the array in memory (sx*sy*3) */
	long sx, sy;               /* x and y dimensions */
	unsigned char tr, tg, tb;  /* color of the transparent pixel if has_transp is 1 */
	unsigned int hash;         /* precalculated hash value */
	unsigned char *p;          /* pixel array in r,g,b rows of sx long each */
	unsigned long neutral_oid; /* UID of the pixmap in neutral position */
	unsigned long refco;       /* optional reference counting */

	void *hid_data;            /* HID's version of the pixmap */

	/* transformation info */
	pcb_angle_t tr_rot;        /* rotation angle (0 if not transformed) */
	double tr_xscale;
	double tr_yscale;
	unsigned tr_xmirror:1;     /* whether the pixmap is mirrored along the x axis (vertical mirror) */
	unsigned tr_ymirror:1;     /* whether the pixmap is mirrored along the y axis (horizontal mirror) */

	unsigned has_transp:1;     /* 1 if the pixmap has any transparent pixels */
	unsigned transp_valid:1;   /* 1 if transparent pixel is available */
	unsigned hash_valid:1;     /* 1 if the has value has been calculated */
	unsigned hid_data_valid:1; /* 1 if hid_data is already generated and no data changed since - maintained by core, HIDs don't need to check */
};

void pcb_pixmap_reg_import(const pcb_pixmap_import_t *imp, const char *cookie);
void pcb_pixmap_unreg_import_all(const char *cookie);
void rnd_pixmap_uninit(void);

int pcb_pixmap_load(pcb_hidlib_t *hidlib, pcb_pixmap_t *pxm, const char *fn);

unsigned int pcb_pixmap_hash_meta(const void *key);
unsigned int pcb_pixmap_hash_pixels(const void *key);
int pcb_pixmap_eq_meta(const void *keya, const void *keyb);
int pcb_pixmap_eq_pixels(const void *keya, const void *keyb);

#endif
