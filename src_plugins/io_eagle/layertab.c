/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020,2021 Tibor 'Igor2' Palinkas
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
	pcb_layer_combining_t comb;
	const char *purp;
	const char *name;
	int color;
} eagle_layertab_t;

#define NEG_AUTO PCB_LYC_SUB | PCB_LYC_AUTO
#define POS_AUTO PCB_LYC_AUTO

static const eagle_layertab_t eagle_layertab[] = {
	/* non-standard doc layers */
	{17,  PCB_LYT_DOC,                   0, "eagle:pads",     "Pads", -1},
	{18,  PCB_LYT_DOC,                   0, "eagle:vias",     "Vias", -1},
	{19,  PCB_LYT_DOC,                   0, "eagle:unrouted", "Unrouted", -1},
	{23,  PCB_LYT_DOC | PCB_LYT_TOP,     0, "eagle:torigins", "tOrigins", -1},
	{24,  PCB_LYT_DOC | PCB_LYT_BOTTOM,  0, "eagle:borigins", "bOrigins", -1},

	/* we have refdes and values on silk normally */
	{25,  PCB_LYT_DOC | PCB_LYT_TOP,      0,        NULL,    "tNames", -1},
	{26,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   0,        NULL,    "bNames", -1},
	{27,  PCB_LYT_DOC | PCB_LYT_TOP,      0,        NULL,    "tValues", -1},
	{28,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   0,        NULL,    "bValues", -1},

	/* standard mask & paste */
	{29,  PCB_LYT_MASK | PCB_LYT_TOP,     NEG_AUTO, NULL,    "tStop", -1},
	{30,  PCB_LYT_MASK | PCB_LYT_BOTTOM,  NEG_AUTO, NULL,    "bStop", -1},
	{31,  PCB_LYT_PASTE | PCB_LYT_TOP,    POS_AUTO, NULL,    "tCream", -1},
	{32,  PCB_LYT_PASTE | PCB_LYT_BOTTOM, POS_AUTO, NULL,    "bCream", -1},

	/* uncommon fab layers */
	{33,  PCB_LYT_MECH | PCB_LYT_TOP,     0,        "finish.eagle", "tFinish", -1},
	{34,  PCB_LYT_MECH | PCB_LYT_BOTTOM,  0,        "finish.eagle", "bFinish", -1},
	{35,  PCB_LYT_MECH | PCB_LYT_TOP,     0,        "adhesive",     "tGlue", -1},
	{36,  PCB_LYT_MECH | PCB_LYT_BOTTOM,  0,        "adhesive",     "bGlue", -1},
	{37,  PCB_LYT_DOC | PCB_LYT_TOP,      0,        "eagle:ttest",  "tTest", -1},
	{38,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   0,        "eagle:btest",  "bTest", -1},

	/* various keepouts */
	{39,  PCB_LYT_DOC | PCB_LYT_TOP,      0,        "ko.courtyard",     "tKeepout", -1},
	{40,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   0,        "ko.courtyard",     "bKeepout", -1},
	{41,  PCB_LYT_DOC | PCB_LYT_TOP,      0,        "ko@top-copper",    "tRestrict", -1},
	{42,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   0,        "ko@bottom-copper", "bRestrict", -1},
	{43,  PCB_LYT_DOC,                    0,        "ko.via",           "vRestrict", -1},

	/* non-standard doc layers */
	{44,  PCB_LYT_DOC,                    0,        "eagle:drills",     "Drills", -1},
	{45,  PCB_LYT_DOC,                    0,        "eagle:holes",      "Holes", -1},

	/* unplated internal cutouts */
	{46,  PCB_LYT_BOUNDARY,               0,        "uroute",           "Milling", -1},

	/* non-standard doc layers */
	{47,  PCB_LYT_DOC,                    0,        "eagle:measures",   "Measures", -1},
	{48,  PCB_LYT_DOC,                    0,        "eagle:document",   "Document", -1},
	{49,  PCB_LYT_DOC,                    0,        "eagle:referencelc","ReferenceLC", -1},
	{50,  PCB_LYT_DOC,                    0,        "eagle:referencels","ReferenceLS", -1},

	/* user doc not ending up at the fab */
	{51,  PCB_LYT_DOC | PCB_LYT_TOP,      0,        "assy",             "tDocu",   14},
	{52,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   0,        "assy",             "bDocu",   7},

	{0}
};
