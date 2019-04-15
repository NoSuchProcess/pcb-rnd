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

#include "config.h"

#include "layertab.h"

/* shorthand lyc for silk, mask and paste (pcb-rnd side feature) */
#define LYS (PCB_LYC_AUTO)
#define LYM (PCB_LYC_AUTO | PCB_LYC_SUB)
#define LYP (PCB_LYC_AUTO)

const kicad_layertab_t kicad_layertab[] = {
	/*id prefix     plen score | action            lyc   purp      layer-type */
	{ 1, "B.Adhes",   0,  3,     LYACT_NEW_MISC,    0,   "adhes",  PCB_LYT_MECH | PCB_LYT_BOTTOM, 1},
	{ 0, "B.Adhes",   0,  2,     LYACT_NEW_MISC,    0,   "adhes",  PCB_LYT_MECH | PCB_LYT_BOTTOM, 0},
	{ 1, NULL,        0,  1,     LYACT_NEW_MISC,    0,   "adhes",  PCB_LYT_MECH | PCB_LYT_BOTTOM, 0},

	{ 2, "F.Adhes",   0,  3,     LYACT_NEW_MISC,    0,   "adhes",  PCB_LYT_MECH | PCB_LYT_TOP, 1},
	{ 0, "F.Adhes",   0,  2,     LYACT_NEW_MISC,    0,   "adhes",  PCB_LYT_MECH | PCB_LYT_TOP, 0},
	{ 2, NULL,        0,  1,     LYACT_NEW_MISC,    0,   "adhes",  PCB_LYT_MECH | PCB_LYT_TOP, 0},

	{ 3, "B.Paste",   0,  3,     LYACT_EXISTING,    LYP, NULL,     PCB_LYT_PASTE | PCB_LYT_BOTTOM, 1},
	{ 0, "B.Paste",   0,  2,     LYACT_EXISTING,    LYP, NULL,     PCB_LYT_PASTE | PCB_LYT_BOTTOM, 0},
	{ 3, NULL,        0,  1,     LYACT_EXISTING,    LYP, NULL,     PCB_LYT_PASTE | PCB_LYT_BOTTOM, 0},

	{ 4, "F.Paste",   0,  3,     LYACT_EXISTING,    LYP, NULL,     PCB_LYT_PASTE | PCB_LYT_TOP, 1},
	{ 0, "F.Paste",   0,  2,     LYACT_EXISTING,    LYP, NULL,     PCB_LYT_PASTE | PCB_LYT_TOP, 0},
	{ 4, NULL,        0,  1,     LYACT_EXISTING,    LYP, NULL,     PCB_LYT_PASTE | PCB_LYT_TOP, 0},

	{ 5, "B.SilkS",   0,  3,     LYACT_EXISTING,    LYS, NULL,     PCB_LYT_SILK | PCB_LYT_BOTTOM, 1},
	{ 0, "B.SilkS",   0,  2,     LYACT_EXISTING,    LYS, NULL,     PCB_LYT_SILK | PCB_LYT_BOTTOM, 0},
	{ 5, NULL,        0,  1,     LYACT_EXISTING,    LYS, NULL,     PCB_LYT_SILK | PCB_LYT_BOTTOM, 0},

	{ 6, "F.SilkS",   0,  3,     LYACT_EXISTING,    LYS, NULL,     PCB_LYT_SILK | PCB_LYT_TOP, 1},
	{ 0, "F.SilkS",   0,  2,     LYACT_EXISTING,    LYS, NULL,     PCB_LYT_SILK | PCB_LYT_TOP, 0},
	{ 6, NULL,        0,  1,     LYACT_EXISTING,    LYS, NULL,     PCB_LYT_SILK | PCB_LYT_TOP, 0},

	{ 7, "B.Mask",    0,  3,     LYACT_EXISTING,    LYM, NULL,     PCB_LYT_MASK | PCB_LYT_BOTTOM, 1},
	{ 0, "B.Mask",    0,  2,     LYACT_EXISTING,    LYM, NULL,     PCB_LYT_MASK | PCB_LYT_BOTTOM, 0},
	{ 7, NULL,        0,  1,     LYACT_EXISTING,    LYM, NULL,     PCB_LYT_MASK | PCB_LYT_BOTTOM, 0},

	{ 8, "F.Mask",    0,  3,     LYACT_EXISTING,    LYM, NULL,     PCB_LYT_MASK | PCB_LYT_TOP, 1},
	{ 0, "F.Mask",    0,  2,     LYACT_EXISTING,    LYM, NULL,     PCB_LYT_MASK | PCB_LYT_TOP, 0},
	{ 8, NULL,        0,  1,     LYACT_EXISTING,    LYM, NULL,     PCB_LYT_MASK | PCB_LYT_TOP, 0},

	{ 9, "Dwgs.",     5,  3,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 1},
	{ 0, "Dwgs.",     5,  2,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},
	{ 9, NULL,        0,  1,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},

	{10, "Cmts.",     5,  3,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 1},
	{00, "Cmts.",     5,  2,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},
	{10, NULL,        0,  1,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},

	{11, "Eco",       3,  3,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 1},
	{12, "Eco",       3,  3,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},
	{00, "Eco",       3,  2,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},
	{11, NULL,        0,  1,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},
	{12, NULL,        0,  1,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},

	{13, "Edge.Cuts", 0,  5,     LYACT_NEW_OUTLINE, 0,   "uroute", 0, 1},
	{00, "Edge.Cuts", 0,  4,     LYACT_NEW_OUTLINE, 0,   "uroute", 0, 0},
	{13, "Edge.",     5,  3,     LYACT_NEW_OUTLINE, 0,   "uroute", 0, 0},
	{00, "Edge.",     5,  2,     LYACT_NEW_OUTLINE, 0,   "uroute", 0, 0},
	{13, NULL,        0,  1,     LYACT_NEW_OUTLINE, 0,   "uroute", 0, 0},

	{14, "Margin",    0,  3,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 1},
	{00, "Margin",    0,  2,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},
	{14, NULL,        0,  1,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC, 0},

	{15, "B.CrtYd",   0,  3,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_BOTTOM, 1},
	{00, "B.CrtYd",   0,  2,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_BOTTOM, 0},
	{15, NULL,        0,  1,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_BOTTOM, 0},

	{16, "F.CrtYd",   0,  3,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_TOP, 1},
	{00, "F.CrtYd",   0,  2,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_TOP, 0},
	{16, NULL,        0,  1,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_TOP, 0},

	{17, "B.Fab",     0,  3,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_BOTTOM, 1},
	{00, "B.Fab",     0,  2,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_BOTTOM, 0},
	{17, NULL,        0,  1,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_BOTTOM, 0},

	{18, "F.Fab",     0,  3,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_TOP, 1},
	{00, "F.Fab",     0,  2,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_TOP, 0},
	{18, NULL,        0,  1,     LYACT_NEW_MISC,    0,   NULL,     PCB_LYT_DOC | PCB_LYT_TOP, 0},

	{0, NULL, 0, 0, 0, 0, NULL, 0, 0} /* terminator: score = 0 */
};

#undef LYM
#undef LYS
#undef LYP
