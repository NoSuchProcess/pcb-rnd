/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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


typedef struct {
	int id; /* in eagle */
	pcb_layer_type_t lyt;
	const char *purp;
	const char *name;
	int color;
} eagle_layertab_t;

static const eagle_layertab_t eagle_layertab[] = {

	{29,  PCB_LYT_MASK | PCB_LYT_TOP,     NULL,    "tStop", -1},
	{30,  PCB_LYT_MASK | PCB_LYT_BOTTOM,  NULL,    "bStop", -1},
	{31,  PCB_LYT_PASTE | PCB_LYT_TOP,    NULL,    "tCream", -1},
	{32,  PCB_LYT_PASTE | PCB_LYT_BOTTOM, NULL,    "bCream", -1},

	/* tDocu & bDocu are used for info used when designing, but not necessarily for
	   exporting to Gerber i.e. package outlines that cross pads, or instructions.
	   These layers within the silk groups will be needed when subc replaces elements
	   since most Eagle packages use tDocu, bDocu for some of their artwork */
	{51,  PCB_LYT_SILK | PCB_LYT_TOP,     NULL,    "tDocu",   14},
	{52,  PCB_LYT_SILK | PCB_LYT_BOTTOM,  NULL,    "bDocu",   7},

	{0}
};
