/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *
 */

#ifndef PCB_FONT_H
#define PCB_FONT_H

#include <genht/htip.h>
#include <librnd/core/global_typedefs.h>
#include "obj_poly.h"
#include "obj_poly_list.h"
#include "obj_arc.h"
#include "obj_arc_list.h"
#include <librnd/core/box.h>

#ifdef PCB_WANT_FONT2
#	include <librnd/font2/font.h>
#else
#	include <librnd/font/font.h>
#endif

struct pcb_font_s {          /* complete set of symbols */
	rnd_font_t rnd_font; /* temporary */
};

struct pcb_fontkit_s {          /* a set of unrelated fonts */
	rnd_font_t dflt;              /* default, fallback font, also the sysfont */
	htip_t fonts;                 /* key: ->id; val: (rnd_font_t *) */
	rnd_bool valid, hash_inited;
	rnd_font_id_t last_id;        /* highest font id ever seen in this kit */
};

/* Look up font. If not found: return NULL (fallback=0), or return the
   default font (fallback=1) */
rnd_font_t *pcb_font(pcb_board_t *pcb, rnd_font_id_t id, int fallback);

void pcb_font_create_default(pcb_board_t *pcb);

/*** font kit handling ***/
void pcb_fontkit_free(pcb_fontkit_t *fk);
rnd_font_t *pcb_new_font(pcb_fontkit_t *fk, rnd_font_id_t id, const char *name);
int pcb_del_font(pcb_fontkit_t *fk, rnd_font_id_t id);
int pcb_move_font(pcb_fontkit_t *fk, rnd_font_id_t src, rnd_font_id_t dst);

/* Reset the fontkit so that only the default font is kept and all extra fonts are purged */
void pcb_fontkit_reset(pcb_fontkit_t *fk);

/* Unlink the font from pcb, but do not free anything but return the font */
rnd_font_t *pcb_font_unlink(pcb_board_t *pcb, rnd_font_id_t id);


#endif

