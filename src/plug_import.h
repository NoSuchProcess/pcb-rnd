/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
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
 *
 */

#ifndef PCB_PLUG_IMPORT_H
#define PCB_PLUG_IMPORT_H

#include "config.h"
#include "conf.h"

/**************************** API definition *********************************/

typedef enum pcb_plug_import_aspect_e { /* bitfield of aspects that can be imported */
	IMPORT_ASPECT_NETLIST = 1
} pcb_plug_import_aspect_t;

typedef struct pcb_plug_import_s pcb_plug_import_t;
struct pcb_plug_import_s {
	pcb_plug_import_t *next;
	void *plugin_data;

	/* Check if the plugin supports format fmt. Return 0 if not supported or
	   an integer priority if supported. The higher the prio is the more likely
	   the plugin gets the next operation on the file. Base prio should be 100
	   for native formats. Return non-0 only if all aspects are supported. */
	int (*fmt_support_prio)(pcb_plug_import_t *ctx, unsigned int aspects, FILE *f, const char *filename);

	/* Perform the import; return 0 on success */
	int (*import)(pcb_plug_import_t *ctx, unsigned int aspects, const char *fn);
};

extern pcb_plug_import_t *pcb_plug_import_chain;

void pcb_import_uninit(void);

/********** hook wrappers **********/
int pcb_import(pcb_hidlib_t *hidlib, char *filename, unsigned int aspect);
int pcb_import_netlist(pcb_hidlib_t *, char *);


#endif
