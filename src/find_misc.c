/* $Id$ */

/*
 *
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

static void DrawNewConnections(void);

/* ---------------------------------------------------------------------------
 * checks if all lists of new objects are handled
 */
static bool ListsEmpty(bool AndRats)
{
	bool empty;
	int i;

	empty = (PVList.Location >= PVList.Number);
	if (AndRats)
		empty = empty && (RatList.Location >= RatList.Number);
	for (i = 0; i < max_copper_layer && empty; i++)
		empty = empty && LineList[i].Location >= LineList[i].Number
			&& ArcList[i].Location >= ArcList[i].Number && PolygonList[i].Location >= PolygonList[i].Number;
	return (empty);
}

static void reassign_no_drc_flags(void)
{
	int layer;

	for (layer = 0; layer < max_copper_layer; layer++) {
		LayerTypePtr l = LAYER_PTR(layer);
		l->no_drc = AttributeGet(l, "PCB::skip-drc") != NULL;
	}
}


/* ---------------------------------------------------------------------------
 * loops till no more connections are found 
 */
static bool DoIt(bool AndRats, bool AndDraw)
{
	bool newone = false;
	reassign_no_drc_flags();
	do {
		/* lookup connections; these are the steps (2) to (4)
		 * from the description
		 */
		newone = LookupPVConnectionsToPVList() ||
			LookupLOConnectionsToPVList(AndRats) || LookupLOConnectionsToLOList(AndRats) || LookupPVConnectionsToLOList(AndRats);
		if (AndDraw)
			DrawNewConnections();
	}
	while (!newone && !ListsEmpty(AndRats));
	if (AndDraw)
		Draw();
	return (newone);
}

/* ---------------------------------------------------------------------------
 * draws all new connections which have been found since the
 * routine was called the last time
 */
static void DrawNewConnections(void)
{
	int i;
	Cardinal position;

	/* decrement 'i' to keep layerstack order */
	for (i = max_copper_layer - 1; i != -1; i--) {
		Cardinal layer = LayerStack[i];

		if (PCB->Data->Layer[layer].On) {
			/* draw all new lines */
			position = LineList[layer].DrawLocation;
			for (; position < LineList[layer].Number; position++)
				DrawLine(LAYER_PTR(layer), LINELIST_ENTRY(layer, position));
			LineList[layer].DrawLocation = LineList[layer].Number;

			/* draw all new arcs */
			position = ArcList[layer].DrawLocation;
			for (; position < ArcList[layer].Number; position++)
				DrawArc(LAYER_PTR(layer), ARCLIST_ENTRY(layer, position));
			ArcList[layer].DrawLocation = ArcList[layer].Number;

			/* draw all new polygons */
			position = PolygonList[layer].DrawLocation;
			for (; position < PolygonList[layer].Number; position++)
				DrawPolygon(LAYER_PTR(layer), POLYGONLIST_ENTRY(layer, position));
			PolygonList[layer].DrawLocation = PolygonList[layer].Number;
		}
	}

	/* draw all new pads */
	if (PCB->PinOn)
		for (i = 0; i < 2; i++) {
			position = PadList[i].DrawLocation;

			for (; position < PadList[i].Number; position++)
				DrawPad(PADLIST_ENTRY(i, position));
			PadList[i].DrawLocation = PadList[i].Number;
		}

	/* draw all new PVs; 'PVList' holds a list of pointers to the
	 * sorted array pointers to PV data
	 */
	while (PVList.DrawLocation < PVList.Number) {
		PinTypePtr pv = PVLIST_ENTRY(PVList.DrawLocation);

		if (TEST_FLAG(PINFLAG, pv)) {
			if (PCB->PinOn)
				DrawPin(pv);
		}
		else if (PCB->ViaOn)
			DrawVia(pv);
		PVList.DrawLocation++;
	}
	/* draw the new rat-lines */
	if (PCB->RatOn) {
		position = RatList.DrawLocation;
		for (; position < RatList.Number; position++)
			DrawRat(RATLIST_ENTRY(position));
		RatList.DrawLocation = RatList.Number;
	}
}

/* ---------------------------------------------------------------------------
 * find all connections to pins of all element
 */
void LookupConnectionsToAllElements(FILE * FP)
{
	/* reset all currently marked connections */
	User = false;
	TheFlag = FOUNDFLAG;
	ResetConnections(false);
	InitConnectionLookup();

	ELEMENT_LOOP(PCB->Data);
	{
		/* break if abort dialog returned true */
		if (PrintElementConnections(element, FP, false))
			break;
		SEPARATE(FP);
		if (conf_core.editor.reset_after_element && gdl_it_idx(&__it__) != 1)
			ResetConnections(false);
	}
	END_LOOP;
	if (conf_core.editor.beep_when_finished)
		gui->beep();
	ResetConnections(false);
	FreeConnectionLookupMemory();
	Redraw();
}

/*---------------------------------------------------------------------------
 * add the starting object to the list of found objects
 */
static bool ListStart(int type, void *ptr1, void *ptr2, void *ptr3)
{
	DumpList();
	switch (type) {
	case PIN_TYPE:
	case VIA_TYPE:
		{
			if (ADD_PV_TO_LIST((PinTypePtr) ptr2, 0, NULL, FCT_START))
				return true;
			break;
		}

	case RATLINE_TYPE:
		{
			if (ADD_RAT_TO_LIST((RatTypePtr) ptr1, 0, NULL, FCT_START))
				return true;
			break;
		}

	case LINE_TYPE:
		{
			int layer = GetLayerNumber(PCB->Data,
																 (LayerTypePtr) ptr1);

			if (ADD_LINE_TO_LIST(layer, (LineTypePtr) ptr2, 0, NULL, FCT_START))
				return true;
			break;
		}

	case ARC_TYPE:
		{
			int layer = GetLayerNumber(PCB->Data,
																 (LayerTypePtr) ptr1);

			if (ADD_ARC_TO_LIST(layer, (ArcTypePtr) ptr2, 0, NULL, FCT_START))
				return true;
			break;
		}

	case POLYGON_TYPE:
		{
			int layer = GetLayerNumber(PCB->Data,
																 (LayerTypePtr) ptr1);

			if (ADD_POLYGON_TO_LIST(layer, (PolygonTypePtr) ptr2, 0, NULL, FCT_START))
				return true;
			break;
		}

	case PAD_TYPE:
		{
			PadTypePtr pad = (PadTypePtr) ptr2;
			if (ADD_PAD_TO_LIST(TEST_FLAG(ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER, pad, 0, NULL, FCT_START))
				return true;
			break;
		}
	}
	return (false);
}


/* ---------------------------------------------------------------------------
 * looks up all connections from the object at the given coordinates
 * the TheFlag (normally 'FOUNDFLAG') is set for all objects found
 * the objects are re-drawn if AndDraw is true
 * also the action is marked as undoable if AndDraw is true
 */
void LookupConnection(Coord X, Coord Y, bool AndDraw, Coord Range, int which_flag)
{
	void *ptr1, *ptr2, *ptr3;
	char *name;
	int type;

	/* check if there are any pins or pads at that position */

	reassign_no_drc_flags();

	type = SearchObjectByLocation(LOOKUP_FIRST, &ptr1, &ptr2, &ptr3, X, Y, Range);
	if (type == NO_TYPE) {
		type = SearchObjectByLocation(LOOKUP_MORE, &ptr1, &ptr2, &ptr3, X, Y, Range);
		if (type == NO_TYPE)
			return;
		if (type & SILK_TYPE) {
			int laynum = GetLayerNumber(PCB->Data,
																	(LayerTypePtr) ptr1);

			/* don't mess with non-conducting objects! */
			if (laynum >= max_copper_layer || ((LayerTypePtr) ptr1)->no_drc)
				return;
		}
	}
	else {
		name = ConnectionName(type, ptr1, ptr2);
		hid_actionl("NetlistShow", name, NULL);
	}

	TheFlag = which_flag;
	User = AndDraw;
	InitConnectionLookup();

	/* now add the object to the appropriate list and start scanning
	 * This is step (1) from the description
	 */
	ListStart(type, ptr1, ptr2, ptr3);
	DoIt(true, AndDraw);
	if (User)
		IncrementUndoSerialNumber();
	User = false;

	/* we are done */
	if (AndDraw)
		Draw();
	if (AndDraw && conf_core.editor.beep_when_finished)
		gui->beep();
	FreeConnectionLookupMemory();
}

/* ---------------------------------------------------------------------------
 * find connections for rats nesting
 * assumes InitConnectionLookup() has already been done
 */
void RatFindHook(int type, void *ptr1, void *ptr2, void *ptr3, bool undo, bool AndRats)
{
	User = undo;
	DumpList();
	ListStart(type, ptr1, ptr2, ptr3);
	DoIt(AndRats, false);
	User = false;
}

/* ---------------------------------------------------------------------------
 * resets all used flags of pins and vias
 */
bool ResetFoundPinsViasAndPads(bool AndDraw)
{
	bool change = false;

	VIA_LOOP(PCB->Data);
	{
		if (TEST_FLAG(TheFlag, via)) {
			if (AndDraw)
				AddObjectToFlagUndoList(VIA_TYPE, via, via, via);
			CLEAR_FLAG(TheFlag, via);
			if (AndDraw)
				DrawVia(via);
			change = true;
		}
	}
	END_LOOP;
	ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (TEST_FLAG(TheFlag, pin)) {
				if (AndDraw)
					AddObjectToFlagUndoList(PIN_TYPE, element, pin, pin);
				CLEAR_FLAG(TheFlag, pin);
				if (AndDraw)
					DrawPin(pin);
				change = true;
			}
		}
		END_LOOP;
		PAD_LOOP(element);
		{
			if (TEST_FLAG(TheFlag, pad)) {
				if (AndDraw)
					AddObjectToFlagUndoList(PAD_TYPE, element, pad, pad);
				CLEAR_FLAG(TheFlag, pad);
				if (AndDraw)
					DrawPad(pad);
				change = true;
			}
		}
		END_LOOP;
	}
	END_LOOP;
	if (change)
		SetChangedFlag(true);
	return change;
}

/* ---------------------------------------------------------------------------
 * resets all used flags of LOs
 */
bool ResetFoundLinesAndPolygons(bool AndDraw)
{
	bool change = false;

	RAT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(TheFlag, line)) {
			if (AndDraw)
				AddObjectToFlagUndoList(RATLINE_TYPE, line, line, line);
			CLEAR_FLAG(TheFlag, line);
			if (AndDraw)
				DrawRat(line);
			change = true;
		}
	}
	END_LOOP;
	COPPERLINE_LOOP(PCB->Data);
	{
		if (TEST_FLAG(TheFlag, line)) {
			if (AndDraw)
				AddObjectToFlagUndoList(LINE_TYPE, layer, line, line);
			CLEAR_FLAG(TheFlag, line);
			if (AndDraw)
				DrawLine(layer, line);
			change = true;
		}
	}
	ENDALL_LOOP;
	COPPERARC_LOOP(PCB->Data);
	{
		if (TEST_FLAG(TheFlag, arc)) {
			if (AndDraw)
				AddObjectToFlagUndoList(ARC_TYPE, layer, arc, arc);
			CLEAR_FLAG(TheFlag, arc);
			if (AndDraw)
				DrawArc(layer, arc);
			change = true;
		}
	}
	ENDALL_LOOP;
	COPPERPOLYGON_LOOP(PCB->Data);
	{
		if (TEST_FLAG(TheFlag, polygon)) {
			if (AndDraw)
				AddObjectToFlagUndoList(POLYGON_TYPE, layer, polygon, polygon);
			CLEAR_FLAG(TheFlag, polygon);
			if (AndDraw)
				DrawPolygon(layer, polygon);
			change = true;
		}
	}
	ENDALL_LOOP;
	if (change)
		SetChangedFlag(true);
	return change;
}

/* ---------------------------------------------------------------------------
 * resets all found connections
 */
bool ResetConnections(bool AndDraw)
{
	bool change = false;

	change = ResetFoundPinsViasAndPads(AndDraw) || change;
	change = ResetFoundLinesAndPolygons(AndDraw) || change;

	return change;
}

/*----------------------------------------------------------------------------
 * Dumps the list contents
 */
static void DumpList(void)
{
	Cardinal i;

	for (i = 0; i < 2; i++) {
		PadList[i].Number = 0;
		PadList[i].Location = 0;
		PadList[i].DrawLocation = 0;
	}

	PVList.Number = 0;
	PVList.Location = 0;

	for (i = 0; i < max_copper_layer; i++) {
		LineList[i].Location = 0;
		LineList[i].DrawLocation = 0;
		LineList[i].Number = 0;
		ArcList[i].Location = 0;
		ArcList[i].DrawLocation = 0;
		ArcList[i].Number = 0;
		PolygonList[i].Location = 0;
		PolygonList[i].DrawLocation = 0;
		PolygonList[i].Number = 0;
	}
	RatList.Number = 0;
	RatList.Location = 0;
	RatList.DrawLocation = 0;
}

/*----------------------------------------------------------------------------
 * set up a temporary flag to use
 */
void SaveFindFlag(int NewFlag)
{
	OldFlag = TheFlag;
	TheFlag = NewFlag;
}

/*----------------------------------------------------------------------------
 * restore flag
 */
void RestoreFindFlag(void)
{
	TheFlag = OldFlag;
}

void InitConnectionLookup(void)
{
	InitComponentLookup();
	InitLayoutLookup();
}

void FreeConnectionLookupMemory(void)
{
	FreeComponentLookupMemory();
	FreeLayoutLookupMemory();
}
