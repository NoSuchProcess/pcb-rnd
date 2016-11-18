/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
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

pcb_element_t *pcb_element_alloc(pcb_data_t * data);
void pcb_element_free(pcb_element_t * data);

pcb_element_t *pcb_element_new(pcb_data_t *Data, pcb_element_t *Element,
	pcb_font_t *PCBFont, pcb_flag_t Flags, char *Description, char *NameOnPCB,
	char *Value, pcb_coord_t TextX, pcb_coord_t TextY, pcb_uint8_t Direction,
	int TextScale, pcb_flag_t TextFlags, pcb_bool uniqueName);

void pcb_element_destroy(pcb_element_t * element);

pcb_line_t *pcb_element_line_alloc(pcb_element_t *Element);


void pcb_element_bbox(pcb_data_t *Data, pcb_element_t *Element, pcb_font_t *Font);
void pcb_element_rotate90(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y, unsigned Number);
void pcb_element_rotate(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina, pcb_angle_t angle);

pcb_bool pcb_element_change_side(pcb_element_t *Element, pcb_coord_t yoff);
pcb_bool pcb_selected_element_change_side(void);
/* Return a relative rotation for an element, useful only for
   comparing two similar footprints.  */
int pcb_element_get_orientation(pcb_element_t * e);


pcb_bool pcb_element_load_to_buffer(pcb_buffer_t *Buffer, const char *Name);
int pcb_element_load_footprint_by_name(pcb_buffer_t *Buffer, const char *Footprint);
pcb_bool pcb_element_smash_buffer(pcb_buffer_t *Buffer);
pcb_bool pcb_element_convert_from_buffer(pcb_buffer_t *Buffer);
pcb_element_t *pcb_element_copy(pcb_data_t *Data, pcb_element_t *Dest, pcb_element_t *Src, pcb_bool uniqueName, pcb_coord_t dx, pcb_coord_t dy);
char *pcb_element_uniq_name(pcb_data_t *Data, char *Name);

void r_delete_element(pcb_data_t * data, pcb_element_t * element);

void pcb_element_move(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t DX, pcb_coord_t DY);
void *pcb_element_remove(pcb_element_t *Element);
void pcb_element_mirror(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t yoff);

pcb_arc_t *pcb_element_arc_new(pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y,
	pcb_coord_t Width, pcb_coord_t Height, pcb_angle_t angle, pcb_angle_t delta, pcb_coord_t Thickness);

pcb_line_t *pcb_element_line_new(pcb_element_t *Element, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness);

void pcb_element_text_set(pcb_text_t *Text, pcb_font_t *PCBFont, pcb_coord_t X, pcb_coord_t Y,
	unsigned Direction, char *TextString, int Scale, pcb_flag_t Flags);


/* Change the specified text on an element, either on the board (give
   PCB, PCB->Data) or in a buffer (give NULL, Buffer->Data).  The old
   string is returned, and must be properly freed by the caller.  */
char *pcb_element_text_change(pcb_board_t * pcb, pcb_data_t * data, pcb_element_t *Element, int which, char *new_name);


/* ---------------------------------------------------------------------------
 * access macros for elements name structure
 */
#define PCB_ELEMNAME_IDX_DESCRIPTION 0
#define PCB_ELEMNAME_IDX_REFDES   1
#define PCB_ELEMNAME_IDX_VALUE       2
#define PCB_ELEMNAME_IDX_VISIBLE()      (conf_core.editor.name_on_pcb ? PCB_ELEMNAME_IDX_REFDES :\
	(conf_core.editor.description ? PCB_ELEMNAME_IDX_DESCRIPTION : PCB_ELEMNAME_IDX_VALUE))

#define ELEMENT_NAME(p,e) ((e)->Name[PCB_ELEMNAME_IDX_VISIBLE()].TextString)
#define DESCRIPTION_NAME(e) ((e)->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString)
#define NAMEONPCB_NAME(e) ((e)->Name[PCB_ELEMNAME_IDX_REFDES].TextString)
#define VALUE_NAME(e) ((e)->Name[PCB_ELEMNAME_IDX_VALUE].TextString)
#define ELEMENT_TEXT(p,e) ((e)->Name[PCB_ELEMNAME_IDX_VISIBLE()])
#define DESCRIPTION_TEXT(e) ((e)->Name[PCB_ELEMNAME_IDX_DESCRIPTION])
#define NAMEONPCB_TEXT(e) ((e)->Name[PCB_ELEMNAME_IDX_REFDES])
#define VALUE_TEXT(e) ((e)->Name[PCB_ELEMNAME_IDX_VALUE])

/*** loops ***/

#define PCB_ELEMENT_LOOP(top) do {                                      \
  pcb_element_t *element;                                             \
  gdl_iterator_t __it__;                                            \
  pinlist_foreach(&(top)->Element, &__it__, element) {

#define	PCB_ELEMENT_PCB_TEXT_LOOP(element) do { 	\
	pcb_cardinal_t	n;				\
	pcb_text_t *text;				\
	for (n = PCB_MAX_ELEMENTNAMES-1; n != -1; n--)	\
	{						\
		text = &(element)->Name[n]

#define	PCB_ELEMENT_NAME_LOOP(element) do	{ 			\
	pcb_cardinal_t	n;					\
	char		*textstring;				\
	for (n = PCB_MAX_ELEMENTNAMES-1; n != -1; n--)		\
	{							\
		textstring = (element)->Name[n].TextString

#define PCB_ELEMENT_PCB_LINE_LOOP(element) do {                              \
  pcb_line_t *line;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Line, &__it__, line) {

#define PCB_ELEMENT_ARC_LOOP(element) do {                               \
  pcb_arc_t *arc;                                                     \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Arc, &__it__, arc) {

#endif

