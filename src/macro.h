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

#define	SWAP_SIGN_X(x)		(x)
#define	SWAP_SIGN_Y(y)		(-(y))
#define	SWAP_ANGLE(a)		(-(a))
#define	SWAP_DELTA(d)		(-(d))
#define	SWAP_X(x)		(SWAP_SIGN_X(x))
#define	SWAP_Y(y)		(PCB->MaxHeight +SWAP_SIGN_Y(y))

/* ---------------------------------------------------------------------------
 * misc macros, some might already be defined by <limits.h>
 */
#ifndef	MIN
#define	MIN(a,b)		((a) < (b) ? (a) : (b))
#define	MAX(a,b)		((a) > (b) ? (a) : (b))
#endif
#ifndef SGN
#define SGN(a)			((a) >0 ? 1 : ((a) == 0 ? 0 : -1))
#endif
#define SGNZ(a)                 ((a) >=0 ? 1 : -1)
#define MAKEMIN(a,b)            if ((b) < (a)) (a) = (b)
#define MAKEMAX(a,b)            if ((b) > (a)) (a) = (b)

#define	ENTRIES(x)		(sizeof((x))/sizeof((x)[0]))
#define	UNKNOWN(a)		((a) && *(a) ? (a) : "(unknown)")
#define NSTRCMP(a, b)		((a) ? ((b) ? strcmp((a),(b)) : 1) : -1)
#define	EMPTY(a)		((a) ? (a) : "")
#define	EMPTY_STRING_P(a)	((a) ? (a)[0]==0 : 1)
#define XOR(a,b)		(((a) && !(b)) || (!(a) && (b)))
#define SQUARE(x)		((float) (x) * (float) (x))
#define TO_RADIANS(degrees)	(PCB_M180 * (degrees))

/* ---------------------------------------------------------------------------
 * returns the object ID
 */
#define	OBJECT_ID(p)		(((AnyObjectTypePtr) p)->ID)

/* ---------------------------------------------------------------------------
 * access macro for current buffer
 */
#define	PASTEBUFFER		(&Buffers[conf_core.editor.buffer_number])

/* ---------------------------------------------------------------------------
 * some routines for flag setting, clearing, changing and testing
 */
#define	SET_FLAG(F,P)		((P)->Flags.f |= (F))
#define	CLEAR_FLAG(F,P)		((P)->Flags.f &= (~(F)))
#define	TEST_FLAG(F,P)		((P)->Flags.f & (F) ? 1 : 0)
#define	TOGGLE_FLAG(F,P)	((P)->Flags.f ^= (F))
#define	ASSIGN_FLAG(F,V,P)	((P)->Flags.f = ((P)->Flags.f & (~(F))) | ((V) ? (F) : 0))
#define TEST_FLAGS(F,P)         (((P)->Flags.f & (F)) == (F) ? 1 : 0)

#define FLAGS_EQUAL(F1,F2)	(memcmp (&F1, &F2, sizeof(FlagType)) == 0)

#define THERMFLAG(L)		(0xf << (4 *((L) % 2)))

#define TEST_THERM(L,P)		((P)->Flags.t[(L)/2] & THERMFLAG(L) ? 1 : 0)
#define GET_THERM(L,P)		(((P)->Flags.t[(L)/2] >> (4 * ((L) % 2))) & 0xf)
#define CLEAR_THERM(L,P)	(P)->Flags.t[(L)/2] &= ~THERMFLAG(L)
#define ASSIGN_THERM(L,V,P)	(P)->Flags.t[(L)/2] = ((P)->Flags.t[(L)/2] & ~THERMFLAG(L)) | ((V)  << (4 * ((L) % 2)))


#define GET_SQUARE(P)		((P)->Flags.q)
#define CLEAR_SQUARE(P)		(P)->Flags.q = 0
#define ASSIGN_SQUARE(V,P)	(P)->Flags.q = V


#define GET_INTCONN(P)		((P)->Flags.int_conn_grp)

extern int mem_any_set(unsigned char *, int);
#define TEST_ANY_THERMS(P)	mem_any_set((P)->Flags.t, sizeof((P)->Flags.t))

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
 *  Determines if text is actually visible
 */
#define TEXT_IS_VISIBLE(b, l, t) \
	((l)->On)

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

#define VIA_LOOP(top) do {                                          \
  PinType *via;                                                     \
  gdl_iterator_t __it__;                                            \
  pinlist_foreach(&(top)->Via, &__it__, via) {

#define DRILL_LOOP(top) do             {               \
        pcb_cardinal_t        n;                                      \
        DrillTypePtr    drill;                                  \
        for (n = 0; (top)->DrillN > 0 && n < (top)->DrillN; n++)                        \
        {                                                       \
                drill = &(top)->Drill[n]

#define NETLIST_LOOP(top) do   {                         \
        pcb_cardinal_t        n;                                      \
        NetListTypePtr   netlist;                               \
        for (n = (top)->NetListN-1; n != -1; n--)               \
        {                                                       \
                netlist = &(top)->NetList[n]

#define NET_LOOP(top) do   {                             \
        pcb_cardinal_t        n;                                      \
        NetTypePtr   net;                                       \
        for (n = (top)->NetN-1; n != -1; n--)                   \
        {                                                       \
                net = &(top)->Net[n]

#define CONNECTION_LOOP(net) do {                         \
        pcb_cardinal_t        n;                                      \
        ConnectionTypePtr       connection;                     \
        for (n = (net)->ConnectionN-1; n != -1; n--)            \
        {                                                       \
                connection = & (net)->Connection[n]

#define ELEMENT_LOOP(top) do {                                      \
  ElementType *element;                                             \
  gdl_iterator_t __it__;                                            \
  pinlist_foreach(&(top)->Element, &__it__, element) {

#define RAT_LOOP(top) do {                                          \
  RatType *line;                                                    \
  gdl_iterator_t __it__;                                            \
  ratlist_foreach(&(top)->Rat, &__it__, line) {

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

#define PIN_LOOP(element) do {                                      \
  PinType *pin;                                                     \
  gdl_iterator_t __it__;                                            \
  pinlist_foreach(&(element)->Pin, &__it__, pin) {

#define PAD_LOOP(element) do {                                      \
  PadType *pad;                                                     \
  gdl_iterator_t __it__;                                            \
  padlist_foreach(&(element)->Pad, &__it__, pad) {

#define ARC_LOOP(element) do {                                      \
  ArcType *arc;                                                     \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Arc, &__it__, arc) {

#define ELEMENTLINE_LOOP(element) do {                              \
  LineType *line;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Line, &__it__, line) {

#define ELEMENTARC_LOOP(element) do {                               \
  ArcType *arc;                                                     \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Arc, &__it__, arc) {

#define LINE_LOOP(layer) do {                                       \
  LineType *line;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(layer)->Line, &__it__, line) {

#define TEXT_LOOP(layer) do {                                       \
  TextType *text;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(layer)->Text, &__it__, text) {

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

#define	ALLPIN_LOOP(top)	\
        ELEMENT_LOOP(top); \
	        PIN_LOOP(element)\

#define	ALLPAD_LOOP(top) \
	ELEMENT_LOOP(top); \
	  PAD_LOOP(element)

#define	ALLLINE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		LINE_LOOP(layer)

#define ALLARC_LOOP(top) do {		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer + 2; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	ALLPOLYGON_LOOP(top)	do {		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	COPPERLINE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer; l++, layer++)	\
	{ \
		LINE_LOOP(layer)

#define COPPERARC_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	COPPERPOLYGON_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	SILKLINE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		LINE_LOOP(layer)

#define SILKARC_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	SILKPOLYGON_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		POLYGON_LOOP(layer)

#define	ALLTEXT_LOOP(top)	do {		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		TEXT_LOOP(layer)

#define	VISIBLELINE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			LINE_LOOP(layer)

#define	VISIBLEARC_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			ARC_LOOP(layer)

#define	VISIBLETEXT_LOOP(board) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (board)->Data->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
                TEXT_LOOP(layer);                                      \
                  if (TEXT_IS_VISIBLE((board), layer, text))

#define	VISIBLEPOLYGON_LOOP(top) do	{	\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			POLYGON_LOOP(layer)

#define POINTER_LOOP(top) do	{	\
	pcb_cardinal_t	n;			\
	void	**ptr;				\
	for (n = (top)->PtrN-1; n != -1; n--)	\
	{					\
		ptr = &(top)->Ptr[n]

#define MENU_LOOP(top)	do {	\
	pcb_cardinal_t	l;			\
	LibraryMenuTypePtr menu;		\
	for (l = (top)->MenuN-1; l != -1; l--)	\
	{					\
		menu = &(top)->Menu[l]

#define ENTRY_LOOP(top) do	{	\
	pcb_cardinal_t	n;			\
	LibraryEntryTypePtr entry;		\
	for (n = (top)->EntryN-1; n != -1; n--)	\
	{					\
		entry = &(top)->Entry[n]

#define GROUP_LOOP(data, group) do { 	\
	pcb_cardinal_t entry; \
        for (entry = 0; entry < ((PCBTypePtr)(data->pcb))->LayerGroups.Number[(group)]; entry++) \
        { \
		LayerTypePtr layer;		\
		pcb_cardinal_t number; 		\
		number = ((PCBTypePtr)(data->pcb))->LayerGroups.Entries[(group)][entry]; \
		if (number >= max_copper_layer)	\
		  continue;			\
		layer = &data->Layer[number];

#define LAYER_LOOP(data, ml) do { \
        pcb_cardinal_t n; \
	for (n = 0; n < ml; n++) \
	{ \
	   LayerTypePtr layer = (&data->Layer[(n)]);


#define LAYER_IS_EMPTY(layer) LAYER_IS_EMPTY_((layer))
#define LAYER_IS_EMPTY_(layer) \
 ((linelist_length(&layer->Line) == 0) && (arclist_length(&layer->Arc) == 0) && (polylist_length(&layer->Polygon) == 0) && (textlist_length(&layer->Text) == 0))
#endif
