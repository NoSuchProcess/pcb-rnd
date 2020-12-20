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
	/* non-standard doc layers */
	{17,  PCB_LYT_DOC,                    "eagle:pads",     "Pads", -1},
	{18,  PCB_LYT_DOC,                    "eagle:vias",     "Vias", -1},
	{19,  PCB_LYT_DOC,                    "eagle:unrouted", "Unrouted", -1},
	{23,  PCB_LYT_DOC | PCB_LYT_TOP,      "eagle:torigins", "tOrigins", -1},
	{24,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   "eagle:borigins", "bOrigins", -1},

	/* we have refdes and values on silk normally */
	{25,  PCB_LYT_DOC | PCB_LYT_TOP,      NULL,    "tNames", -1},
	{26,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   NULL,    "bNames", -1},
	{27,  PCB_LYT_DOC | PCB_LYT_TOP,      NULL,    "tValues", -1},
	{28,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   NULL,    "bValues", -1},

	/* standard mask & paste */
	{29,  PCB_LYT_MASK | PCB_LYT_TOP,     NULL,    "tStop", -1},
	{30,  PCB_LYT_MASK | PCB_LYT_BOTTOM,  NULL,    "bStop", -1},
	{31,  PCB_LYT_PASTE | PCB_LYT_TOP,    NULL,    "tCream", -1},
	{32,  PCB_LYT_PASTE | PCB_LYT_BOTTOM, NULL,    "bCream", -1},

	/* uncommon fab layers */
	{33,  PCB_LYT_MECH | PCB_LYT_TOP,     "finish.eagle", "tFinish", -1},
	{34,  PCB_LYT_MECH | PCB_LYT_BOTTOM,  "finish.eagle", "bFinish", -1},
	{35,  PCB_LYT_MECH | PCB_LYT_TOP,     "adhesive",     "tGlue", -1},
	{36,  PCB_LYT_MECH | PCB_LYT_BOTTOM,  "adhesive",     "bGlue", -1},
	{37,  PCB_LYT_DOC | PCB_LYT_TOP,      "eagle:ttest",  "tTest", -1},
	{38,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   "eagle:btest",  "bTest", -1},

	/* various keepouts */
	{39,  PCB_LYT_DOC | PCB_LYT_TOP,      "ko.courtyard",     "tKeepout", -1},
	{40,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   "ko.courtyard",     "bKeepout", -1},
	{41,  PCB_LYT_DOC | PCB_LYT_TOP,      "ko@top-copper",    "tRestrict", -1},
	{42,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   "ko@bottom-copper", "bRestrict", -1},
	{43,  PCB_LYT_DOC,                    "ko.via",           "vRestrict", -1},

	/* non-standard doc layers */
	{44,  PCB_LYT_DOC,                    "eagle:drills",     "Drills", -1},
	{45,  PCB_LYT_DOC,                    "eagle:holes",      "Holes", -1},

	/* unplated internal cutouts */
	{46,  PCB_LYT_BOUNDARY,               "uroute",           "Milling", -1},

	/* non-standard doc layers */
	{47,  PCB_LYT_DOC,                    "eagle:measures",   "Measures", -1},
	{48,  PCB_LYT_DOC,                    "eagle:document",   "Document", -1},
	{49,  PCB_LYT_DOC,                    "eagle:referencelc","ReferenceLC", -1},
	{50,  PCB_LYT_DOC,                    "eagle:referencels","ReferenceLS", -1},

	/* user doc not ending up at the fab */
	{51,  PCB_LYT_DOC | PCB_LYT_TOP,      "assy",             "tDocu",   14},
	{52,  PCB_LYT_DOC | PCB_LYT_BOTTOM,   "assy",             "bDocu",   7},

	{0}
};
