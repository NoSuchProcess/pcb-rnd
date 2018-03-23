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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Drawing primitive: elements */

#ifndef PCB_OBJ_ELEM_H
#define PCB_OBJ_ELEM_H

#include "obj_common.h"
#include "obj_arc_list.h"
#include "obj_line_list.h"
#include "obj_pad_list.h"
#include "obj_pinvia_list.h"
#include "obj_text.h"
#include "font.h"


struct pcb_element_s {
	PCB_ANYOBJECTFIELDS;
	pcb_text_t Name[PCB_MAX_ELEMENTNAMES]; /* the elements names: description text, name on PCB second, value third - see PCB_ELEMNAME_IDX_VISIBLE() below */
	pcb_coord_t MarkX, MarkY;               /* position mark */
	pinlist_t Pin;
	padlist_t Pad;
	linelist_t Line;
	arclist_t Arc;
	pcb_box_t VBox;
	gdl_elem_t link;
};

void *pcb_element_remove(pcb_element_t *Element);
/* returns whether the silk group of the element's is visible */
#define pcb_element_silk_vis(elem) \
	(PCB->LayerGroups.grp[((PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (elem))) ? pcb_layergrp_get_bottom_silk() : pcb_layergrp_get_top_silk())].vis)



/* ---------------------------------------------------------------------------
 * access macros for elements name structure
 */
#define PCB_ELEMNAME_IDX_DESCRIPTION 0
#define PCB_ELEMNAME_IDX_REFDES      1
#define PCB_ELEMNAME_IDX_VALUE       2
#define PCB_ELEMNAME_IDX_VISIBLE()   (conf_core.editor.name_on_pcb ? PCB_ELEMNAME_IDX_REFDES :\
	(conf_core.editor.description ? PCB_ELEMNAME_IDX_DESCRIPTION : PCB_ELEMNAME_IDX_VALUE))

#define PCB_ELEM_NAME_VISIBLE(p,e)   ((e)->Name[PCB_ELEMNAME_IDX_VISIBLE()].TextString)
#define PCB_ELEM_NAME_DESCRIPTION(e) ((e)->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString)
#define PCB_ELEM_NAME_REFDES(e)      ((e)->Name[PCB_ELEMNAME_IDX_REFDES].TextString)
#define PCB_ELEM_NAME_VALUE(e)       ((e)->Name[PCB_ELEMNAME_IDX_VALUE].TextString)

#define PCB_ELEM_TEXT_VISIBLE(p,e)   ((e)->Name[PCB_ELEMNAME_IDX_VISIBLE()])
#define PCB_ELEM_TEXT_DESCRIPTION(e) ((e)->Name[PCB_ELEMNAME_IDX_DESCRIPTION])
#define PCB_ELEM_TEXT_REFDES(e)      ((e)->Name[PCB_ELEMNAME_IDX_REFDES])
#define PCB_ELEM_TEXT_VALUE(e)       ((e)->Name[PCB_ELEMNAME_IDX_VALUE])

/*** loops ***/

#define PCB_ELEMENT_LOOP(top) do {                                  \
	pcb_element_t *element;                                           \
	gdl_iterator_t __it__;                                            \
	pinlist_foreach(&(top)->Element, &__it__, element) {

#define	PCB_ELEMENT_PCB_TEXT_LOOP(element) do { \
	pcb_cardinal_t n; \
	pcb_text_t *text; \
	for (n = PCB_MAX_ELEMENTNAMES-1; n != -1; n--) \
	{	\
		text = &(element)->Name[n]

#define	PCB_ELEMENT_NAME_LOOP(element) do	{ \
	pcb_cardinal_t n; \
	char *textstring; \
	for (n = PCB_MAX_ELEMENTNAMES-1; n != -1; n--) \
	{ \
		textstring = (element)->Name[n].TextString

#define PCB_ELEMENT_PCB_LINE_LOOP(element) do {                     \
	pcb_line_t *line;                                                 \
	gdl_iterator_t __it__;                                            \
	linelist_foreach(&(element)->Line, &__it__, line) {

#define PCB_ELEMENT_ARC_LOOP(element) do {                          \
	pcb_arc_t *arc;                                                   \
	gdl_iterator_t __it__;                                            \
	linelist_foreach(&(element)->Arc, &__it__, arc) {

#endif

