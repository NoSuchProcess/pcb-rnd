/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  kicad IO plugin - load/save in KiCad format
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

/* A pattern matching/scoring table for pairing up rather fixed KiCad layers
   with dynamic pcb-rnd layer types */

#include "layer.h"

typedef struct {
	/* match */
	int id;         /* if not 0, layer last_copper+lnum must match this value */
	char *prefix;   /* when non-NULL: layer name prefix must match on prefix_len length */
	int prefix_len; /* 0 means full string match */
	int score;      /* match score - the higher the more chance this rule is used */

	/* action */
	enum {
		LYACT_EXISTING,    /* create a new layer in an existing group */
		LYACT_NEW_MISC,    /* create a new misc group */
		LYACT_NEW_OUTLINE  /* create a new outline group */
	} action;

	pcb_layer_combining_t lyc;  /* layer combination (compositing) flags */
	const char *purpose;        /* may be NULL */
	pcb_layer_type_t type;      /* for selecting/creating the group */

	int auto_create;            /* if automatic layer creation is necessary, use this line for a layer */
} kicad_layertab_t;

extern const kicad_layertab_t kicad_layertab[];

