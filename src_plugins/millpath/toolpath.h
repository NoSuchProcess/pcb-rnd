/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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

#include "layer.h"
#include "layer_grp.h"
#include "polygon.h"

typedef struct pcb_tlp_tools_s    pcb_tlp_tools_t;
typedef struct pcb_tlp_line_s     pcb_tlp_line_t;
typedef struct pcb_tlp_seg_s      pcb_tlp_seg_t;
typedef struct pcb_tlp_session_s  pcb_tlp_session_t;

typedef enum pcb_tlp_segtype_s {
	PCB_TLP_SEG_LINE
} pcb_tlp_segtype_t;


struct pcb_tlp_tools_s {
	int used;          /* number of tools */
	pcb_coord_t *dia;  /* tool diameters */
};

struct pcb_tlp_line_s {
	pcb_coord_t x1, y1, x2, y2;
};

struct pcb_tlp_seg_s {
	pcb_tlp_segtype_t type;
	union {
		pcb_tlp_line_t line;
	} seg;
	/* TODO: list link */
};

struct pcb_tlp_session_s {
	const pcb_tlp_tools_t *tools;
	pcb_coord_t edge_clearance; /* when milling copper, keep this clearance from the edges */

	/* temp data */
	pcb_layer_t *res_ply;    /* resulting "remove" polygon */
	pcb_layer_t *res_path;   /* resulting toolpath */
	pcb_layer_t *res_remply; /* resulting "remain" polygon */
	pcb_poly_t *fill;        /* base fill: copper to remove */
	pcb_poly_t *remain;      /* remaining copper */

	pcb_layergrp_t *grp;
	
	/* TODO: list on segments */
};

int pcb_tlp_mill_copper_layer(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layergrp_t *grp);

int pcb_tlp_mill_script(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layergrp_t *grp, const char *script);

