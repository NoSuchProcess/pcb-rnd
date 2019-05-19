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

#ifndef PCB_EXT_OBJ_H
#define PCB_EXT_OBJ_H

#include <genvector/vtp0.h>

#include "obj_subc.h"
#include "draw.h"


typedef struct pcb_extobj_s pcb_extobj_t;

struct pcb_extobj_s {
	/* static data - filled in by the extobj code */
	const char *name;
	void (*draw)(pcb_draw_info_t *info, pcb_subc_t *obj);

	/* dynamic data - filled in by core */
	int idx;
	unsigned registered:1;
};

void pcb_extobj_init(void);
void pcb_extobj_uninit(void);

void pcb_extobj_unreg(pcb_extobj_t *o);
void pcb_extobj_reg(pcb_extobj_t *o);

int pcb_extobj_lookup_idx(const char *name);

extern int pcb_extobj_invalid; /* this changes upon each new extobj reg, forcing the caches to be invalidated eventually */
extern vtp0_t pcb_extobj_i2o;  /* extobj_idx -> (pcb_ext_obj_t *) */

PCB_INLINE pcb_extobj_t *pcb_extobj_get(pcb_subc_t *obj)
{
	pcb_extobj_t **eo;

	if ((obj->extobj == NULL) || (obj->extobj_idx == pcb_extobj_invalid))
		return NULL; /* known negative */

	if (obj->extobj_idx <= 0) { /* invalid idx cache - look up by name */
		obj->extobj_idx = pcb_extobj_lookup_idx(obj->extobj);
		if (obj->extobj_idx == 0) { /* no such name */
			obj->extobj_idx = pcb_extobj_invalid; /* make the next lookup cheaper */
			return NULL;
		}
	}

	eo = (pcb_extobj_t **)vtp0_get(&pcb_extobj_i2o, obj->extobj_idx, 0);
	if ((eo == NULL) || (*eo == NULL)) /* extobj backend got deregistered meanwhile */
		obj->extobj_idx = pcb_extobj_invalid; /* make the next lookup cheaper */
	return *eo;
}


#endif
