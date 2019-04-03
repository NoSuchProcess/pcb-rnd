/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

/* Media (paper) sizes; moved from export_ps */

#include "config.h"
#include <stdlib.h>

#include "unit.h"

#include "media.h"

/*
 * Metric ISO sizes in mm.  See http://en.wikipedia.org/wiki/ISO_paper_sizes
 *
 * A0  841 x 1189
 * A1  594 x 841
 * A2  420 x 594
 * A3  297 x 420
 * A4  210 x 297
 * A5  148 x 210
 * A6  105 x 148
 * A7   74 x 105
 * A8   52 x  74
 * A9   37 x  52
 * A10  26 x  37
 *
 * B0  1000 x 1414
 * B1   707 x 1000
 * B2   500 x  707
 * B3   353 x  500
 * B4   250 x  353
 * B5   176 x  250
 * B6   125 x  176
 * B7    88 x  125
 * B8    62 x   88
 * B9    44 x   62
 * B10   31 x   44
 *
 * awk '{printf("  {\"%s\", %d, %d, MARGINX, MARGINY},\n", $2, $3*100000/25.4, $5*100000/25.4)}'
 *
 * See http://en.wikipedia.org/wiki/Paper_size#Loose_sizes for some of the other sizes.  The
 * {A,B,C,D,E}-Size here are the ANSI sizes and not the architectural sizes.
 */

#define MARGINX PCB_MIL_TO_COORD(500)
#define MARGINY PCB_MIL_TO_COORD(500)

MediaType media_data[] = {
	{"A0", PCB_MM_TO_COORD(841), PCB_MM_TO_COORD(1189), MARGINX, MARGINY},
	{"A1", PCB_MM_TO_COORD(594), PCB_MM_TO_COORD(841), MARGINX, MARGINY},
	{"A2", PCB_MM_TO_COORD(420), PCB_MM_TO_COORD(594), MARGINX, MARGINY},
	{"A3", PCB_MM_TO_COORD(297), PCB_MM_TO_COORD(420), MARGINX, MARGINY},
	{"A4", PCB_MM_TO_COORD(210), PCB_MM_TO_COORD(297), MARGINX, MARGINY},
	{"A5", PCB_MM_TO_COORD(148), PCB_MM_TO_COORD(210), MARGINX, MARGINY},
	{"A6", PCB_MM_TO_COORD(105), PCB_MM_TO_COORD(148), MARGINX, MARGINY},
	{"A7", PCB_MM_TO_COORD(74), PCB_MM_TO_COORD(105), MARGINX, MARGINY},
	{"A8", PCB_MM_TO_COORD(52), PCB_MM_TO_COORD(74), MARGINX, MARGINY},
	{"A9", PCB_MM_TO_COORD(37), PCB_MM_TO_COORD(52), MARGINX, MARGINY},
	{"A10", PCB_MM_TO_COORD(26), PCB_MM_TO_COORD(37), MARGINX, MARGINY},
	{"B0", PCB_MM_TO_COORD(1000), PCB_MM_TO_COORD(1414), MARGINX, MARGINY},
	{"B1", PCB_MM_TO_COORD(707), PCB_MM_TO_COORD(1000), MARGINX, MARGINY},
	{"B2", PCB_MM_TO_COORD(500), PCB_MM_TO_COORD(707), MARGINX, MARGINY},
	{"B3", PCB_MM_TO_COORD(353), PCB_MM_TO_COORD(500), MARGINX, MARGINY},
	{"B4", PCB_MM_TO_COORD(250), PCB_MM_TO_COORD(353), MARGINX, MARGINY},
	{"B5", PCB_MM_TO_COORD(176), PCB_MM_TO_COORD(250), MARGINX, MARGINY},
	{"B6", PCB_MM_TO_COORD(125), PCB_MM_TO_COORD(176), MARGINX, MARGINY},
	{"B7", PCB_MM_TO_COORD(88), PCB_MM_TO_COORD(125), MARGINX, MARGINY},
	{"B8", PCB_MM_TO_COORD(62), PCB_MM_TO_COORD(88), MARGINX, MARGINY},
	{"B9", PCB_MM_TO_COORD(44), PCB_MM_TO_COORD(62), MARGINX, MARGINY},
	{"B10", PCB_MM_TO_COORD(31), PCB_MM_TO_COORD(44), MARGINX, MARGINY},
	{"Letter", PCB_INCH_TO_COORD(8.5), PCB_INCH_TO_COORD(11), MARGINX, MARGINY},
	{"11x17", PCB_INCH_TO_COORD(11), PCB_INCH_TO_COORD(17), MARGINX, MARGINY},
	{"Ledger", PCB_INCH_TO_COORD(17), PCB_INCH_TO_COORD(11), MARGINX, MARGINY},
	{"Legal", PCB_INCH_TO_COORD(8.5), PCB_INCH_TO_COORD(14), MARGINX, MARGINY},
	{"Executive", PCB_INCH_TO_COORD(7.5), PCB_INCH_TO_COORD(10), MARGINX, MARGINY},
	{"A-size", PCB_INCH_TO_COORD(8.5), PCB_INCH_TO_COORD(11), MARGINX, MARGINY},
	{"B-size", PCB_INCH_TO_COORD(11), PCB_INCH_TO_COORD(17), MARGINX, MARGINY},
	{"C-size", PCB_INCH_TO_COORD(17), PCB_INCH_TO_COORD(22), MARGINX, MARGINY},
	{"D-size", PCB_INCH_TO_COORD(22), PCB_INCH_TO_COORD(34), MARGINX, MARGINY},
	{"E-size", PCB_INCH_TO_COORD(34), PCB_INCH_TO_COORD(44), MARGINX, MARGINY},
	{"US-Business_Card", PCB_INCH_TO_COORD(3.5), PCB_INCH_TO_COORD(2.0), 0, 0},
	{"Intl-Business_Card", PCB_INCH_TO_COORD(3.375), PCB_INCH_TO_COORD(2.125), 0, 0},
	{NULL, 0, 0, 0, 0},
};

#undef MARGINX
#undef MARGINY
