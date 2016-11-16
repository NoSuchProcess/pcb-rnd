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
static pcb_bool ListsEmpty(pcb_bool AndRats)
{
	pcb_bool empty;
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
		pcb_layer_t *l = LAYER_PTR(layer);
		l->no_drc = pcb_attrib_get(l, "PCB::skip-drc") != NULL;
	}
}


/* ---------------------------------------------------------------------------
 * loops till no more connections are found
 */
static pcb_bool DoIt(pcb_bool AndRats, pcb_bool AndDraw)
{
	pcb_bool newone = pcb_false;
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
		pcb_draw();
	return (newone);
}

/* ---------------------------------------------------------------------------
 * draws all new connections which have been found since the
 * routine was called the last time
 */
static void DrawNewConnections(void)
{
	int i;
	pcb_cardinal_t position;

	/* decrement 'i' to keep layerstack order */
	for (i = max_copper_layer - 1; i != -1; i--) {
		pcb_cardinal_t layer = LayerStack[i];

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
		pcb_pin_t *pv = PVLIST_ENTRY(PVList.DrawLocation);

		if (PCB_FLAG_TEST(PCB_FLAG_PIN, pv)) {
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

/*---------------------------------------------------------------------------
 * add the starting object to the list of found objects
 */
static pcb_bool ListStart(int type, void *ptr1, void *ptr2, void *ptr3)
{
	DumpList();
	switch (type) {
	case PCB_TYPE_PIN:
	case PCB_TYPE_VIA:
		{
			if (ADD_PV_TO_LIST((pcb_pin_t *) ptr2, 0, NULL, FCT_START))
				return pcb_true;
			break;
		}

	case PCB_TYPE_RATLINE:
		{
			if (ADD_RAT_TO_LIST((pcb_rat_t *) ptr1, 0, NULL, FCT_START))
				return pcb_true;
			break;
		}

	case PCB_TYPE_LINE:
		{
			int layer = GetLayerNumber(PCB->Data,
																 (pcb_layer_t *) ptr1);

			if (ADD_LINE_TO_LIST(layer, (pcb_line_t *) ptr2, 0, NULL, FCT_START))
				return pcb_true;
			break;
		}

	case PCB_TYPE_ARC:
		{
			int layer = GetLayerNumber(PCB->Data,
																 (pcb_layer_t *) ptr1);

			if (ADD_ARC_TO_LIST(layer, (pcb_arc_t *) ptr2, 0, NULL, FCT_START))
				return pcb_true;
			break;
		}

	case PCB_TYPE_POLYGON:
		{
			int layer = GetLayerNumber(PCB->Data,
																 (pcb_layer_t *) ptr1);

			if (ADD_POLYGON_TO_LIST(layer, (pcb_polygon_t *) ptr2, 0, NULL, FCT_START))
				return pcb_true;
			break;
		}

	case PCB_TYPE_PAD:
		{
			pcb_pad_t *pad = (pcb_pad_t *) ptr2;
			if (ADD_PAD_TO_LIST(PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER, pad, 0, NULL, FCT_START))
				return pcb_true;
			break;
		}
	}
	return (pcb_false);
}


/* ---------------------------------------------------------------------------
 * looks up all connections from the object at the given coordinates
 * the TheFlag (normally 'PCB_FLAG_FOUND') is set for all objects found
 * the objects are re-drawn if AndDraw is pcb_true
 * also the action is marked as undoable if AndDraw is pcb_true
 */
void pcb_lookup_conn(pcb_coord_t X, pcb_coord_t Y, pcb_bool AndDraw, pcb_coord_t Range, int which_flag)
{
	void *ptr1, *ptr2, *ptr3;
	char *name;
	int type;

	/* check if there are any pins or pads at that position */

	reassign_no_drc_flags();

	type = pcb_search_obj_by_location(LOOKUP_FIRST, &ptr1, &ptr2, &ptr3, X, Y, Range);
	if (type == PCB_TYPE_NONE) {
		type = pcb_search_obj_by_location(LOOKUP_MORE, &ptr1, &ptr2, &ptr3, X, Y, Range);
		if (type == PCB_TYPE_NONE)
			return;
		if (type & SILK_TYPE) {
			int laynum = GetLayerNumber(PCB->Data,
																	(pcb_layer_t *) ptr1);

			/* don't mess with non-conducting objects! */
			if (laynum >= max_copper_layer || ((pcb_layer_t *) ptr1)->no_drc)
				return;
		}
	}
	else {
		name = pcb_connection_name(type, ptr1, ptr2);
		pcb_hid_actionl("NetlistShow", name, NULL);
	}

	TheFlag = which_flag;
	User = AndDraw;
	pcb_conn_lookup_init();

	/* now add the object to the appropriate list and start scanning
	 * This is step (1) from the description
	 */
	ListStart(type, ptr1, ptr2, ptr3);
	DoIt(pcb_true, AndDraw);
	if (User)
		pcb_undo_inc_serial();
	User = pcb_false;

	/* we are done */
	if (AndDraw)
		pcb_draw();
	if (AndDraw && conf_core.editor.beep_when_finished)
		gui->beep();
	pcb_conn_lookup_uninit();
}

void pcb_lookup_conn_by_pin(int type, void *ptr1)
{
	User = 0;
	pcb_conn_lookup_init();
	ListStart(type, NULL, ptr1, NULL);

	DoIt(pcb_true, pcb_false);

	pcb_conn_lookup_uninit();
}


/* ---------------------------------------------------------------------------
 * find connections for rats nesting
 * assumes pcb_conn_lookup_init() has already been done
 */
void pcb_rat_find_hook(int type, void *ptr1, void *ptr2, void *ptr3, pcb_bool undo, pcb_bool AndRats)
{
	User = undo;
	DumpList();
	ListStart(type, ptr1, ptr2, ptr3);
	DoIt(AndRats, pcb_false);
	User = pcb_false;
}

/* ---------------------------------------------------------------------------
 * resets all used flags of pins and vias
 */
pcb_bool pcb_reset_found_pins_vias_pads(pcb_bool AndDraw)
{
	pcb_bool change = pcb_false;

	PCB_VIA_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(TheFlag, via)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_VIA, via, via, via);
			PCB_FLAG_CLEAR(TheFlag, via);
			if (AndDraw)
				DrawVia(via);
			change = pcb_true;
		}
	}
	END_LOOP;
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		PCB_PIN_LOOP(element);
		{
			if (PCB_FLAG_TEST(TheFlag, pin)) {
				if (AndDraw)
					AddObjectToFlagUndoList(PCB_TYPE_PIN, element, pin, pin);
				PCB_FLAG_CLEAR(TheFlag, pin);
				if (AndDraw)
					DrawPin(pin);
				change = pcb_true;
			}
		}
		END_LOOP;
		PCB_PAD_LOOP(element);
		{
			if (PCB_FLAG_TEST(TheFlag, pad)) {
				if (AndDraw)
					AddObjectToFlagUndoList(PCB_TYPE_PAD, element, pad, pad);
				PCB_FLAG_CLEAR(TheFlag, pad);
				if (AndDraw)
					DrawPad(pad);
				change = pcb_true;
			}
		}
		END_LOOP;
	}
	END_LOOP;
	if (change)
		pcb_board_set_changed_flag(pcb_true);
	return change;
}

/* ---------------------------------------------------------------------------
 * resets all used flags of LOs
 */
pcb_bool pcb_reset_found_lines_polys(pcb_bool AndDraw)
{
	pcb_bool change = pcb_false;

	PCB_RAT_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(TheFlag, line)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_RATLINE, line, line, line);
			PCB_FLAG_CLEAR(TheFlag, line);
			if (AndDraw)
				DrawRat(line);
			change = pcb_true;
		}
	}
	END_LOOP;
	PCB_LINE_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(TheFlag, line)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_LINE, layer, line, line);
			PCB_FLAG_CLEAR(TheFlag, line);
			if (AndDraw)
				DrawLine(layer, line);
			change = pcb_true;
		}
	}
	ENDALL_LOOP;
	PCB_ARC_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(TheFlag, arc)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_ARC, layer, arc, arc);
			PCB_FLAG_CLEAR(TheFlag, arc);
			if (AndDraw)
				DrawArc(layer, arc);
			change = pcb_true;
		}
	}
	ENDALL_LOOP;
	PCB_POLY_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(TheFlag, polygon)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_POLYGON, layer, polygon, polygon);
			PCB_FLAG_CLEAR(TheFlag, polygon);
			if (AndDraw)
				DrawPolygon(layer, polygon);
			change = pcb_true;
		}
	}
	ENDALL_LOOP;
	if (change)
		pcb_board_set_changed_flag(pcb_true);
	return change;
}

/* ---------------------------------------------------------------------------
 * resets all found connections
 */
pcb_bool pcb_reset_conns(pcb_bool AndDraw)
{
	pcb_bool change = pcb_false;

	change = pcb_reset_found_pins_vias_pads(AndDraw) || change;
	change = pcb_reset_found_lines_polys(AndDraw) || change;

	return change;
}

/*----------------------------------------------------------------------------
 * Dumps the list contents
 */
static void DumpList(void)
{
	pcb_cardinal_t i;

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
void pcb_save_find_flag(int NewFlag)
{
	OldFlag = TheFlag;
	TheFlag = NewFlag;
}

/*----------------------------------------------------------------------------
 * restore flag
 */
void pcb_restore_find_flag(void)
{
	TheFlag = OldFlag;
}

void pcb_conn_lookup_init(void)
{
	pcb_component_lookup_init();
	pcb_layout_lookup_init();
}

void pcb_conn_lookup_uninit(void)
{
	pcb_component_lookup_uninit();
	pcb_layout_lookup_uninit();
}
