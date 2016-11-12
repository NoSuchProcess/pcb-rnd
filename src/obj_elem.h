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


struct element_st {
	ANYOBJECTFIELDS;
	TextType Name[MAX_ELEMENTNAMES]; /* the elements names: description text, name on PCB second, value third - see NAME_INDEX() below */
	Coord MarkX, MarkY;               /* position mark */
	pinlist_t Pin;
	padlist_t Pad;
	linelist_t Line;
	arclist_t Arc;
	BoxType VBox;
	gdl_elem_t link;
};

ElementType *GetElementMemory(DataType * data);
void RemoveFreeElement(ElementType * data);
void FreeElementMemory(ElementType * element);
LineType *GetElementLineMemory(ElementType *Element);


pcb_bool LoadElementToBuffer(BufferTypePtr Buffer, const char *Name);
int LoadFootprintByName(BufferTypePtr Buffer, const char *Footprint);
pcb_bool SmashBufferElement(BufferTypePtr Buffer);
pcb_bool ConvertBufferToElement(BufferTypePtr Buffer);
void FreeRotateElementLowLevel(DataTypePtr Data, ElementTypePtr Element, Coord X, Coord Y, double cosa, double sina, Angle angle);
pcb_bool ChangeElementSide(ElementTypePtr Element, Coord yoff);
pcb_bool ChangeSelectedElementSide(void);
ElementTypePtr CopyElementLowLevel(DataTypePtr Data, ElementTypePtr Dest, ElementTypePtr Src, pcb_bool uniqueName, Coord dx, Coord dy);
void SetElementBoundingBox(DataTypePtr Data, ElementTypePtr Element, FontTypePtr Font);
char *UniqueElementName(DataTypePtr Data, char *Name);
void r_delete_element(DataType * data, ElementType * element);

/* Return a relative rotation for an element, useful only for
   comparing two similar footprints.  */
int ElementOrientation(ElementType * e);

void MoveElementLowLevel(DataTypePtr Data, ElementTypePtr Element, Coord DX, Coord DY);
void *RemoveElement(ElementTypePtr Element);
void RotateElementLowLevel(DataTypePtr Data, ElementTypePtr Element, Coord X, Coord Y, unsigned Number);
void MirrorElementCoordinates(DataTypePtr Data, ElementTypePtr Element, Coord yoff);

ElementTypePtr CreateNewElement(DataTypePtr Data, ElementTypePtr Element,
	FontTypePtr PCBFont, FlagType Flags, char *Description, char *NameOnPCB,
	char *Value, Coord TextX, Coord TextY, pcb_uint8_t Direction,
	int TextScale, FlagType TextFlags, pcb_bool uniqueName);

ArcTypePtr CreateNewArcInElement(ElementTypePtr Element, Coord X, Coord Y,
	Coord Width, Coord Height, Angle angle, Angle delta, Coord Thickness);

LineTypePtr CreateNewLineInElement(ElementTypePtr Element, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness);

void AddTextToElement(TextTypePtr Text, FontTypePtr PCBFont, Coord X, Coord Y,
	unsigned Direction, char *TextString, int Scale, FlagType Flags);


/* Change the specified text on an element, either on the board (give
   PCB, PCB->Data) or in a buffer (give NULL, Buffer->Data).  The old
   string is returned, and must be properly freed by the caller.  */
char *ChangeElementText(pcb_board_t * pcb, DataType * data, ElementTypePtr Element, int which, char *new_name);


/* ---------------------------------------------------------------------------
 * access macros for elements name structure
 */
#define	DESCRIPTION_INDEX	0
#define	NAMEONPCB_INDEX		1
#define	VALUE_INDEX		2
#define	NAME_INDEX()		(conf_core.editor.name_on_pcb ? NAMEONPCB_INDEX :\
				(conf_core.editor.description ?		\
				DESCRIPTION_INDEX : VALUE_INDEX))
#define	ELEMENT_NAME(p,e)	((e)->Name[NAME_INDEX()].TextString)
#define	DESCRIPTION_NAME(e)	((e)->Name[DESCRIPTION_INDEX].TextString)
#define	NAMEONPCB_NAME(e)	((e)->Name[NAMEONPCB_INDEX].TextString)
#define	VALUE_NAME(e)		((e)->Name[VALUE_INDEX].TextString)
#define	ELEMENT_TEXT(p,e)	((e)->Name[NAME_INDEX()])
#define	DESCRIPTION_TEXT(e)	((e)->Name[DESCRIPTION_INDEX])
#define	NAMEONPCB_TEXT(e)	((e)->Name[NAMEONPCB_INDEX])
#define	VALUE_TEXT(e)		((e)->Name[VALUE_INDEX])

/*** loops ***/

#define ELEMENT_LOOP(top) do {                                      \
  ElementType *element;                                             \
  gdl_iterator_t __it__;                                            \
  pinlist_foreach(&(top)->Element, &__it__, element) {

#define	ELEMENTTEXT_LOOP(element) do { 	\
	pcb_cardinal_t	n;				\
	TextTypePtr	text;				\
	for (n = MAX_ELEMENTNAMES-1; n != -1; n--)	\
	{						\
		text = &(element)->Name[n]

#define	ELEMENTNAME_LOOP(element) do	{ 			\
	pcb_cardinal_t	n;					\
	char		*textstring;				\
	for (n = MAX_ELEMENTNAMES-1; n != -1; n--)		\
	{							\
		textstring = (element)->Name[n].TextString

#define ELEMENTLINE_LOOP(element) do {                              \
  LineType *line;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Line, &__it__, line) {

#define ELEMENTARC_LOOP(element) do {                               \
  ArcType *arc;                                                     \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Arc, &__it__, arc) {

#endif

