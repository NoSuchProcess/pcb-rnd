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

/* Drawing primitive: text */

#ifndef PCB_OBJ_TEXT_H
#define PCB_OBJ_TEXT_H

#include "obj_common.h"

struct pcb_text_s {
	PCB_ANYOBJECTFIELDS;
	int Scale;                    /* text scaling in percent */
	pcb_coord_t X, Y;                   /* origin */
	pcb_uint8_t Direction;
	char *TextString;             /* string */
	void *Element;
	gdl_elem_t link;              /* a text is in a list of a layer or an element */
};


pcb_text_t *pcb_text_alloc(pcb_layer_t * layer);
void pcb_text_free(pcb_text_t * data);
pcb_text_t *pcb_text_new(pcb_layer_t *Layer, pcb_font_t *PCBFont, pcb_coord_t X, pcb_coord_t Y, unsigned Direction, int Scale, char *TextString, pcb_flag_t Flags);
void *pcb_text_destroy(pcb_layer_t *Layer, pcb_text_t *Text);


/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_text_on_layer(pcb_layer_t *Layer, pcb_text_t *text, pcb_font_t *PCBFont);

void pcb_text_bbox(pcb_font_t *FontPtr, pcb_text_t *Text);
void pcb_text_rotate90(pcb_text_t *Text, pcb_coord_t X, pcb_coord_t Y, unsigned Number);

#define	pcb_text_move(t,dx,dy)                                  \
	{                                                                  \
		pcb_coord_t __dx__ = (dx), __dy__ = (dy);                        \
		pcb_text_t *__t__ = (t);                                         \
		PCB_BOX_MOVE_LOWLEVEL(&((__t__)->BoundingBox), __dx__, __dy__);  \
		PCB_MOVE((__t__)->X, (__t__)->Y, __dx__, __dy__);                \
	}

/* Determines if text is actually visible */
#define pcb_text_is_visible(b, l, t)      ((l)->On)

#define TEXT_LOOP(layer) do {                                       \
  pcb_text_t *text;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(layer)->Text, &__it__, text) {

#define	ALLTEXT_LOOP(top) do {                        \
	pcb_cardinal_t l;                                   \
	pcb_layer_t *layer = (top)->Layer;                  \
	for (l = 0; l < max_copper_layer + 2; l++, layer++) \
	{                                                   \
		TEXT_LOOP(layer)

#define VISIBLETEXT_LOOP(board) do {                       \
	pcb_cardinal_t l;                                        \
	pcb_layer_t *layer = (board)->Data->Layer;               \
	for (l = 0; l < max_copper_layer + 2; l++, layer++)      \
	{                                                        \
		TEXT_LOOP(layer);                                      \
		if (pcb_text_is_visible((board), layer, text))

#endif
