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

/* some commonly used macros not related to a special C-file
 * the file is included by global.h after const.h
 */

#ifndef	PCB_MACRO_H
#define	PCB_MACRO_H

/* ---------------------------------------------------------------------------
 * macros to transform coord systems
 * draw.c uses a different definition of TO_SCREEN
 */
#ifndef	SWAP_IDENT
#define	SWAP_IDENT			conf_core.editor.show_solder_side
#endif

#define	ENTRIES(x)		(sizeof((x))/sizeof((x)[0]))
#define	UNKNOWN(a)		((a) && *(a) ? (a) : "(unknown)")
#define NSTRCMP(a, b)		((a) ? ((b) ? strcmp((a),(b)) : 1) : -1)
#define	EMPTY(a)		((a) ? (a) : "")
#define	EMPTY_STRING_P(a)	((a) ? (a)[0]==0 : 1)
#define XOR(a,b)		(((a) && !(b)) || (!(a) && (b)))
#define SQUARE(x)		((float) (x) * (float) (x))

/* ---------------------------------------------------------------------------
 * returns the object ID
 */
#define	OBJECT_ID(p)		(((AnyObjectTypePtr) p)->ID)

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

/* ---------------------------------------------------------------------------
 *  Determines if object is on front or back
 */
#define FRONT(o)	\
	((TEST_FLAG(PCB_FLAG_ONSOLDER, (o)) != 0) == SWAP_IDENT)

/* ---------------------------------------------------------------------------
 *  Determines if an object is on the given side. side is either SOLDER_LAYER
 *  or COMPONENT_LAYER.
 */
#define ON_SIDE(element, side) \
        (TEST_FLAG (PCB_FLAG_ONSOLDER, element) == (side == SOLDER_LAYER))

/* ---------------------------------------------------------------------------
 * some loop shortcuts
 *
 * a pointer is created from index addressing because the base pointer
 * may change when new memory is allocated;
 *
 * all data is relative to an objects name 'top' which can be either
 * PCB or PasteBuffer
 */
#define END_LOOP  }} while (0)

#define DRILL_LOOP(top) do             {               \
        pcb_cardinal_t        n;                                      \
        DrillTypePtr    drill;                                  \
        for (n = 0; (top)->DrillN > 0 && n < (top)->DrillN; n++)                        \
        {                                                       \
                drill = &(top)->Drill[n]

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

#define POLYGON_LOOP(layer) do {                                    \
  PolygonType *polygon;                                             \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(layer)->Polygon, &__it__, polygon) {

#define	POLYGONPOINT_LOOP(polygon) do	{	\
	pcb_cardinal_t			n;		\
	PointTypePtr	point;				\
	for (n = (polygon)->PointN-1; n != -1; n--)	\
	{						\
		point = &(polygon)->Points[n]

#define ENDALL_LOOP }} while (0); }} while(0)

#define	ALLPOLYGON_LOOP(top)	do {		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	COPPERPOLYGON_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	SILKPOLYGON_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		POLYGON_LOOP(layer)

#define	VISIBLEPOLYGON_LOOP(top) do	{	\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			POLYGON_LOOP(layer)

#endif
