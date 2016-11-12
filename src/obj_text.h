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
	ANYOBJECTFIELDS;
	int Scale;                    /* text scaling in percent */
	Coord X, Y;                   /* origin */
	pcb_uint8_t Direction;
	char *TextString;             /* string */
	void *Element;
	gdl_elem_t link;              /* a text is in a list of a layer or an element */
};


TextTypePtr GetTextMemory(pcb_layer_t * layer);
void RemoveFreeText(TextType * data);

TextTypePtr CreateNewText(pcb_layer_t *Layer, FontTypePtr PCBFont, Coord X, Coord Y, unsigned Direction, int Scale, char *TextString, FlagType Flags);

/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_text_on_layer(pcb_layer_t *Layer, TextType *text, FontType *PCBFont);

void SetTextBoundingBox(FontTypePtr FontPtr, TextTypePtr Text);
void *RemoveText(pcb_layer_t *Layer, TextTypePtr Text);
void RotateTextLowLevel(TextTypePtr Text, Coord X, Coord Y, unsigned Number);

#define	MOVE_TEXT_LOWLEVEL(t,dx,dy)                    \
	{                                                    \
		MOVE_BOX_LOWLEVEL(&((t)->BoundingBox),(dx),(dy));  \
		MOVE((t)->X, (t)->Y, (dx), (dy));                  \
	}


/* Determines if text is actually visible */
#define TEXT_IS_VISIBLE(b, l, t)      ((l)->On)

#define TEXT_LOOP(layer) do {                                       \
  TextType *text;                                                   \
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
		if (TEXT_IS_VISIBLE((board), layer, text))

#endif
