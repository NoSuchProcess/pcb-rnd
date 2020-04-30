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

#include <librnd/core/unit.h>

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

#define MARGINX RND_MIL_TO_COORD(500)
#define MARGINY RND_MIL_TO_COORD(500)

pcb_media_t pcb_media_data[] = {
	{"A0", RND_MM_TO_COORD(841), RND_MM_TO_COORD(1189), MARGINX, MARGINY},
	{"A1", RND_MM_TO_COORD(594), RND_MM_TO_COORD(841), MARGINX, MARGINY},
	{"A2", RND_MM_TO_COORD(420), RND_MM_TO_COORD(594), MARGINX, MARGINY},
	{"A3", RND_MM_TO_COORD(297), RND_MM_TO_COORD(420), MARGINX, MARGINY},
	{"A4", RND_MM_TO_COORD(210), RND_MM_TO_COORD(297), MARGINX, MARGINY},
	{"A5", RND_MM_TO_COORD(148), RND_MM_TO_COORD(210), MARGINX, MARGINY},
	{"A6", RND_MM_TO_COORD(105), RND_MM_TO_COORD(148), MARGINX, MARGINY},
	{"A7", RND_MM_TO_COORD(74), RND_MM_TO_COORD(105), MARGINX, MARGINY},
	{"A8", RND_MM_TO_COORD(52), RND_MM_TO_COORD(74), MARGINX, MARGINY},
	{"A9", RND_MM_TO_COORD(37), RND_MM_TO_COORD(52), MARGINX, MARGINY},
	{"A10", RND_MM_TO_COORD(26), RND_MM_TO_COORD(37), MARGINX, MARGINY},
	{"B0", RND_MM_TO_COORD(1000), RND_MM_TO_COORD(1414), MARGINX, MARGINY},
	{"B1", RND_MM_TO_COORD(707), RND_MM_TO_COORD(1000), MARGINX, MARGINY},
	{"B2", RND_MM_TO_COORD(500), RND_MM_TO_COORD(707), MARGINX, MARGINY},
	{"B3", RND_MM_TO_COORD(353), RND_MM_TO_COORD(500), MARGINX, MARGINY},
	{"B4", RND_MM_TO_COORD(250), RND_MM_TO_COORD(353), MARGINX, MARGINY},
	{"B5", RND_MM_TO_COORD(176), RND_MM_TO_COORD(250), MARGINX, MARGINY},
	{"B6", RND_MM_TO_COORD(125), RND_MM_TO_COORD(176), MARGINX, MARGINY},
	{"B7", RND_MM_TO_COORD(88), RND_MM_TO_COORD(125), MARGINX, MARGINY},
	{"B8", RND_MM_TO_COORD(62), RND_MM_TO_COORD(88), MARGINX, MARGINY},
	{"B9", RND_MM_TO_COORD(44), RND_MM_TO_COORD(62), MARGINX, MARGINY},
	{"B10", RND_MM_TO_COORD(31), RND_MM_TO_COORD(44), MARGINX, MARGINY},
	{"Letter", RND_INCH_TO_COORD(8.5), RND_INCH_TO_COORD(11), MARGINX, MARGINY},
	{"USLetter", RND_INCH_TO_COORD(8.5), RND_INCH_TO_COORD(11), MARGINX, MARGINY}, /* kicad */
	{"11x17", RND_INCH_TO_COORD(11), RND_INCH_TO_COORD(17), MARGINX, MARGINY},
	{"Ledger", RND_INCH_TO_COORD(17), RND_INCH_TO_COORD(11), MARGINX, MARGINY},
	{"Legal", RND_INCH_TO_COORD(8.5), RND_INCH_TO_COORD(14), MARGINX, MARGINY},
	{"Executive", RND_INCH_TO_COORD(7.5), RND_INCH_TO_COORD(10), MARGINX, MARGINY},
	{"A-size", RND_INCH_TO_COORD(8.5), RND_INCH_TO_COORD(11), MARGINX, MARGINY},
	{"B-size", RND_INCH_TO_COORD(11), RND_INCH_TO_COORD(17), MARGINX, MARGINY},
	{"C-size", RND_INCH_TO_COORD(17), RND_INCH_TO_COORD(22), MARGINX, MARGINY},
	{"D-size", RND_INCH_TO_COORD(22), RND_INCH_TO_COORD(34), MARGINX, MARGINY},
	{"E-size", RND_INCH_TO_COORD(34), RND_INCH_TO_COORD(44), MARGINX, MARGINY},
	{"US-Business_Card", RND_INCH_TO_COORD(3.5), RND_INCH_TO_COORD(2.0), 0, 0},
	{"Intl-Business_Card", RND_INCH_TO_COORD(3.375), RND_INCH_TO_COORD(2.125), 0, 0},
	{NULL, 0, 0, 0, 0},
};

/* MUST BE IN SYNC with the above table */
const char *pcb_medias[] = {
	"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "A10",
	"B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "B10",
	"Letter", "USLetter", "11x17", "Ledger", "Legal", "Executive",
	"A-Size", "B-size", "C-Size", "D-size", "E-size",
	"US-Business_Card", "Intl-Business_Card",
	NULL
};

#undef MARGINX
#undef MARGINY
