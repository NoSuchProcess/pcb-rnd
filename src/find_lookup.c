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

/* Connection lookup functions */

static bool ADD_PV_TO_LIST(PinTypePtr Pin, int from_type, void *from_ptr, found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(Pin->Element ? PIN_TYPE : VIA_TYPE, Pin->Element ? Pin->Element : Pin, Pin, Pin);
	SET_FLAG(TheFlag, Pin);
	make_callback(PIN_TYPE, Pin, from_type, from_ptr, type);
	PVLIST_ENTRY(PVList.Number) = Pin;
	PVList.Number++;
#ifdef DEBUG
	if (PVList.Number > PVList.Size)
		printf("ADD_PV_TO_LIST overflow! num=%d size=%d\n", PVList.Number, PVList.Size);
#endif
	if (drc && !TEST_FLAG(SELECTEDFLAG, Pin))
		return (SetThing(PIN_TYPE, Pin->Element, Pin, Pin));
	return false;
}

static bool ADD_PAD_TO_LIST(Cardinal L, PadTypePtr Pad, int from_type, void *from_ptr, found_conn_type_t type)
{
/*fprintf(stderr, "ADD_PAD_TO_LIST cardinal %d %p %d\n", L, Pad, from_type);*/
	if (User)
		AddObjectToFlagUndoList(PAD_TYPE, Pad->Element, Pad, Pad);
	SET_FLAG(TheFlag, Pad);
	make_callback(PAD_TYPE, Pad, from_type, from_ptr, type);
	PADLIST_ENTRY((L), PadList[(L)].Number) = Pad;
	PadList[(L)].Number++;
#ifdef DEBUG
	if (PadList[(L)].Number > PadList[(L)].Size)
		printf("ADD_PAD_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, PadList[(L)].Number, PadList[(L)].Size);
#endif
	if (drc && !TEST_FLAG(SELECTEDFLAG, Pad))
		return (SetThing(PAD_TYPE, Pad->Element, Pad, Pad));
	return false;
}

static bool ADD_LINE_TO_LIST(Cardinal L, LineTypePtr Ptr, int from_type, void *from_ptr, found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(LINE_TYPE, LAYER_PTR(L), (Ptr), (Ptr));
	SET_FLAG(TheFlag, (Ptr));
	make_callback(LINE_TYPE, Ptr, from_type, from_ptr, type);
	LINELIST_ENTRY((L), LineList[(L)].Number) = (Ptr);
	LineList[(L)].Number++;
#ifdef DEBUG
	if (LineList[(L)].Number > LineList[(L)].Size)
		printf("ADD_LINE_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, LineList[(L)].Number, LineList[(L)].Size);
#endif
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)))
		return (SetThing(LINE_TYPE, LAYER_PTR(L), (Ptr), (Ptr)));
	return false;
}

static bool ADD_ARC_TO_LIST(Cardinal L, ArcTypePtr Ptr, int from_type, void *from_ptr, found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(ARC_TYPE, LAYER_PTR(L), (Ptr), (Ptr));
	SET_FLAG(TheFlag, (Ptr));
	make_callback(ARC_TYPE, Ptr, from_type, from_ptr, type);
	ARCLIST_ENTRY((L), ArcList[(L)].Number) = (Ptr);
	ArcList[(L)].Number++;
#ifdef DEBUG
	if (ArcList[(L)].Number > ArcList[(L)].Size)
		printf("ADD_ARC_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, ArcList[(L)].Number, ArcList[(L)].Size);
#endif
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)))
		return (SetThing(ARC_TYPE, LAYER_PTR(L), (Ptr), (Ptr)));
	return false;
}

static bool ADD_RAT_TO_LIST(RatTypePtr Ptr, int from_type, void *from_ptr, found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(RATLINE_TYPE, (Ptr), (Ptr), (Ptr));
	SET_FLAG(TheFlag, (Ptr));
	make_callback(RATLINE_TYPE, Ptr, from_type, from_ptr, type);
	RATLIST_ENTRY(RatList.Number) = (Ptr);
	RatList.Number++;
#ifdef DEBUG
	if (RatList.Number > RatList.Size)
		printf("ADD_RAT_TO_LIST overflow! num=%d size=%d\n", RatList.Number, RatList.Size);
#endif
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)))
		return (SetThing(RATLINE_TYPE, (Ptr), (Ptr), (Ptr)));
	return false;
}

static bool ADD_POLYGON_TO_LIST(Cardinal L, PolygonTypePtr Ptr, int from_type, void *from_ptr, found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(POLYGON_TYPE, LAYER_PTR(L), (Ptr), (Ptr));
	SET_FLAG(TheFlag, (Ptr));
	make_callback(POLYGON_TYPE, Ptr, from_type, from_ptr, type);
	POLYGONLIST_ENTRY((L), PolygonList[(L)].Number) = (Ptr);
	PolygonList[(L)].Number++;
#ifdef DEBUG
	if (PolygonList[(L)].Number > PolygonList[(L)].Size)
		printf("ADD_ARC_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, PolygonList[(L)].Number, PolygonList[(L)].Size);
#endif
	if (drc && !TEST_FLAG(SELECTEDFLAG, (Ptr)))
		return (SetThing(POLYGON_TYPE, LAYER_PTR(L), (Ptr), (Ptr)));
	return false;
}

bool SetThing(int type, void *ptr1, void *ptr2, void *ptr3)
{
	thing_ptr1 = ptr1;
	thing_ptr2 = ptr2;
	thing_ptr3 = ptr3;
	thing_type = type;
	if (type == PIN_TYPE && ptr1 == NULL) {
		thing_ptr1 = ptr3;
		thing_type = VIA_TYPE;
	}
	return true;
}

/* ---------------------------------------------------------------------------
 * releases all allocated memory
 */
void FreeLayoutLookupMemory(void)
{
	Cardinal i;

	for (i = 0; i < max_copper_layer; i++) {
		free(LineList[i].Data);
		LineList[i].Data = NULL;
		free(ArcList[i].Data);
		ArcList[i].Data = NULL;
		free(PolygonList[i].Data);
		PolygonList[i].Data = NULL;
	}
	free(PVList.Data);
	PVList.Data = NULL;
	free(RatList.Data);
	RatList.Data = NULL;
}

void FreeComponentLookupMemory(void)
{
/*fprintf(stderr, "PadList free both\n");*/
	free(PadList[0].Data);
	PadList[0].Data = NULL;
	free(PadList[1].Data);
	PadList[1].Data = NULL;
}

/* ---------------------------------------------------------------------------
 * allocates memory for component related stacks ...
 * initializes index and sorts it by X1 and X2
 */
void InitComponentLookup(void)
{
	Cardinal i;

	/* initialize pad data; start by counting the total number
	 * on each of the two possible layers
	 */
	NumberOfPads[COMPONENT_LAYER] = NumberOfPads[SOLDER_LAYER] = 0;
	ALLPAD_LOOP(PCB->Data);
	{
		if (TEST_FLAG(ONSOLDERFLAG, pad))
			NumberOfPads[SOLDER_LAYER]++;
		else
			NumberOfPads[COMPONENT_LAYER]++;
	}
	ENDALL_LOOP;
	for (i = 0; i < 2; i++) {
/*fprintf(stderr, "PadList alloc %d: %d\n", i, NumberOfPads[i]);*/

		/* allocate memory for working list */
		PadList[i].Data = (void **) calloc(NumberOfPads[i], sizeof(PadTypePtr));

		/* clear some struct members */
		PadList[i].Location = 0;
		PadList[i].DrawLocation = 0;
		PadList[i].Number = 0;
		PadList[i].Size = NumberOfPads[i];
	}
}

/* ---------------------------------------------------------------------------
 * allocates memory for component related stacks ...
 * initializes index and sorts it by X1 and X2
 */
void InitLayoutLookup(void)
{
	Cardinal i;

	/* initialize line arc and polygon data */
	for (i = 0; i < max_copper_layer; i++) {
		LayerTypePtr layer = LAYER_PTR(i);

		if (linelist_length(&layer->Line)) {
			LineList[i].Size = linelist_length(&layer->Line);
			LineList[i].Data = (void **) calloc(LineList[i].Size, sizeof(LineTypePtr));
		}
		if (arclist_length(&layer->Arc)) {
			ArcList[i].Size = arclist_length(&layer->Arc);
			ArcList[i].Data = (void **) calloc(ArcList[i].Size, sizeof(ArcTypePtr));
		}
		if (polylist_length(&layer->Polygon)) {
			PolygonList[i].Size = polylist_length(&layer->Polygon);
			PolygonList[i].Data = (void **) calloc(PolygonList[i].Size, sizeof(PolygonTypePtr));
		}

		/* clear some struct members */
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

	if (PCB->Data->pin_tree)
		TotalP = PCB->Data->pin_tree->size;
	else
		TotalP = 0;
	if (PCB->Data->via_tree)
		TotalV = PCB->Data->via_tree->size;
	else
		TotalV = 0;
	/* allocate memory for 'new PV to check' list and clear struct */
	PVList.Data = (void **) calloc(TotalP + TotalV, sizeof(PinTypePtr));
	PVList.Size = TotalP + TotalV;
	PVList.Location = 0;
	PVList.DrawLocation = 0;
	PVList.Number = 0;
	/* Initialize ratline data */
	RatList.Size = ratlist_length(&PCB->Data->Rat);
	RatList.Data = (void **) calloc(RatList.Size, sizeof(RatTypePtr));
	RatList.Location = 0;
	RatList.DrawLocation = 0;
	RatList.Number = 0;
}

struct pv_info {
	Cardinal layer;
	PinType pv;
	jmp_buf env;
};

static r_dir_t LOCtoPVline_callback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!TEST_FLAG(TheFlag, line) && PinLineIntersect(&i->pv, line) && !TEST_FLAG(HOLEFLAG, &i->pv)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PIN_TYPE, &i->pv, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPVarc_callback(const BoxType * b, void *cl)
{
	ArcTypePtr arc = (ArcTypePtr) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!TEST_FLAG(TheFlag, arc) && IS_PV_ON_ARC(&i->pv, arc) && !TEST_FLAG(HOLEFLAG, &i->pv)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PIN_TYPE, &i->pv, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPVpad_callback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!TEST_FLAG(TheFlag, pad) && IS_PV_ON_PAD(&i->pv, pad) &&
			!TEST_FLAG(HOLEFLAG, &i->pv) &&
			ADD_PAD_TO_LIST(TEST_FLAG(ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER, pad, PIN_TYPE, &i->pv, FCT_COPPER))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPVrat_callback(const BoxType * b, void *cl)
{
	RatTypePtr rat = (RatTypePtr) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!TEST_FLAG(TheFlag, rat) && IS_PV_ON_RAT(&i->pv, rat) && ADD_RAT_TO_LIST(rat, PIN_TYPE, &i->pv, FCT_RAT))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPVpoly_callback(const BoxType * b, void *cl)
{
	PolygonTypePtr polygon = (PolygonTypePtr) b;
	struct pv_info *i = (struct pv_info *) cl;

	/* if the pin doesn't have a therm and polygon is clearing
	 * then it can't touch due to clearance, so skip the expensive
	 * test. If it does have a therm, you still need to test
	 * because it might not be inside the polygon, or it could
	 * be on an edge such that it doesn't actually touch.
	 */
	if (!TEST_FLAG(TheFlag, polygon) && !TEST_FLAG(HOLEFLAG, &i->pv) &&
			(TEST_THERM(i->layer, &i->pv) || !TEST_FLAG(CLEARPOLYFLAG, polygon)
			 || !i->pv.Clearance)) {
		double wide = MAX(0.5 * i->pv.Thickness + Bloat, 0);
		if (TEST_FLAG(SQUAREFLAG, &i->pv)) {
			Coord x1 = i->pv.X - (i->pv.Thickness + 1 + Bloat) / 2;
			Coord x2 = i->pv.X + (i->pv.Thickness + 1 + Bloat) / 2;
			Coord y1 = i->pv.Y - (i->pv.Thickness + 1 + Bloat) / 2;
			Coord y2 = i->pv.Y + (i->pv.Thickness + 1 + Bloat) / 2;
			if (IsRectangleInPolygon(x1, y1, x2, y2, polygon)
					&& ADD_POLYGON_TO_LIST(i->layer, polygon, PIN_TYPE, &i->pv, FCT_COPPER))
				longjmp(i->env, 1);
		}
		else if (TEST_FLAG(OCTAGONFLAG, &i->pv)) {
			POLYAREA *oct = OctagonPoly(i->pv.X, i->pv.Y, i->pv.Thickness / 2, GET_SQUARE(&i->pv));
			if (isects(oct, polygon, true)
					&& ADD_POLYGON_TO_LIST(i->layer, polygon, PIN_TYPE, &i->pv, FCT_COPPER))
				longjmp(i->env, 1);
		}
		else if (IsPointInPolygon(i->pv.X, i->pv.Y, wide, polygon)
						 && ADD_POLYGON_TO_LIST(i->layer, polygon, PIN_TYPE, &i->pv, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * checks if a PV is connected to LOs, if it is, the LO is added to
 * the appropriate list and the 'used' flag is set
 */
static bool LookupLOConnectionsToPVList(bool AndRats)
{
	Cardinal layer;
	struct pv_info info;

	/* loop over all PVs currently on list */
	while (PVList.Location < PVList.Number) {
		/* get pointer to data */
		info.pv = *(PVLIST_ENTRY(PVList.Location));
		EXPAND_BOUNDS(&info.pv);

		/* check pads */
		if (setjmp(info.env) == 0)
			r_search(PCB->Data->pad_tree, (BoxType *) & info.pv, NULL, LOCtoPVpad_callback, &info, NULL);
		else
			return true;

		/* now all lines, arcs and polygons of the several layers */
		for (layer = 0; layer < max_copper_layer; layer++) {
			if (LAYER_PTR(layer)->no_drc)
				continue;
			info.layer = layer;
			/* add touching lines */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, (BoxType *) & info.pv, NULL, LOCtoPVline_callback, &info, NULL);
			else
				return true;
			/* add touching arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, (BoxType *) & info.pv, NULL, LOCtoPVarc_callback, &info, NULL);
			else
				return true;
			/* check all polygons */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->polygon_tree, (BoxType *) & info.pv, NULL, LOCtoPVpoly_callback, &info, NULL);
			else
				return true;
		}
		/* Check for rat-lines that may intersect the PV */
		if (AndRats) {
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->rat_tree, (BoxType *) & info.pv, NULL, LOCtoPVrat_callback, &info, NULL);
			else
				return true;
		}
		PVList.Location++;
	}
	return false;
}

/* ---------------------------------------------------------------------------
 * find all connections between LO at the current list position and new LOs
 */
static bool LookupLOConnectionsToLOList(bool AndRats)
{
	bool done;
	Cardinal i, group, layer, ratposition,
		lineposition[MAX_LAYER], polyposition[MAX_LAYER], arcposition[MAX_LAYER], padposition[2];

	/* copy the current LO list positions; the original data is changed
	 * by 'LookupPVConnectionsToLOList()' which has to check the same
	 * list entries plus the new ones
	 */
	for (i = 0; i < max_copper_layer; i++) {
		lineposition[i] = LineList[i].Location;
		polyposition[i] = PolygonList[i].Location;
		arcposition[i] = ArcList[i].Location;
	}
	for (i = 0; i < 2; i++)
		padposition[i] = PadList[i].Location;
	ratposition = RatList.Location;

	/* loop over all new LOs in the list; recurse until no
	 * more new connections in the layergroup were found
	 */
	do {
		Cardinal *position;

		if (AndRats) {
			position = &ratposition;
			for (; *position < RatList.Number; (*position)++) {
				group = RATLIST_ENTRY(*position)->group1;
				if (LookupLOConnectionsToRatEnd(&(RATLIST_ENTRY(*position)->Point1), group))
					return (true);
				group = RATLIST_ENTRY(*position)->group2;
				if (LookupLOConnectionsToRatEnd(&(RATLIST_ENTRY(*position)->Point2), group))
					return (true);
			}
		}
		/* loop over all layergroups */
		for (group = 0; group < max_group; group++) {
			Cardinal entry;

			for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++) {
				layer = PCB->LayerGroups.Entries[group][entry];

				/* be aware that the layer number equal max_copper_layer
				 * and max_copper_layer+1 have a special meaning for pads
				 */
				if (layer < max_copper_layer) {
					/* try all new lines */
					position = &lineposition[layer];
					for (; *position < LineList[layer].Number; (*position)++)
						if (LookupLOConnectionsToLine(LINELIST_ENTRY(layer, *position), group, true))
							return (true);

					/* try all new arcs */
					position = &arcposition[layer];
					for (; *position < ArcList[layer].Number; (*position)++)
						if (LookupLOConnectionsToArc(ARCLIST_ENTRY(layer, *position), group))
							return (true);

					/* try all new polygons */
					position = &polyposition[layer];
					for (; *position < PolygonList[layer].Number; (*position)++)
						if (LookupLOConnectionsToPolygon(POLYGONLIST_ENTRY(layer, *position), group))
							return (true);
				}
				else {
					/* try all new pads */
					layer -= max_copper_layer;
					if (layer > 1) {
						Message(_("bad layer number %d max_copper_layer=%d in find.c\n"), layer, max_copper_layer);
						return false;
					}
					position = &padposition[layer];
					for (; *position < PadList[layer].Number; (*position)++)
						if (LookupLOConnectionsToPad(PADLIST_ENTRY(layer, *position), group))
							return (true);
				}
			}
		}

		/* check if all lists are done; Later for-loops
		 * may have changed the prior lists
		 */
		done = !AndRats || ratposition >= RatList.Number;
		for (layer = 0; layer < max_copper_layer + 2; layer++) {
			if (layer < max_copper_layer)
				done = done &&
					lineposition[layer] >= LineList[layer].Number
					&& arcposition[layer] >= ArcList[layer].Number && polyposition[layer] >= PolygonList[layer].Number;
			else
				done = done && padposition[layer - max_copper_layer] >= PadList[layer - max_copper_layer].Number;
		}
	}
	while (!done);
	return (false);
}

static r_dir_t pv_pv_callback(const BoxType * b, void *cl)
{
	PinTypePtr pin = (PinTypePtr) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!TEST_FLAG(TheFlag, pin) && PV_TOUCH_PV(&i->pv, pin)) {
		if (TEST_FLAG(HOLEFLAG, pin) || TEST_FLAG(HOLEFLAG, &i->pv)) {
			SET_FLAG(WARNFLAG, pin);
			conf_core.temp.rat_warn = true;
			if (pin->Element)
				Message(_("WARNING: Hole too close to pin.\n"));
			else
				Message(_("WARNING: Hole too close to via.\n"));
		}
		else if (ADD_PV_TO_LIST(pin, PIN_TYPE, &i->pv, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches for new PVs that are connected to PVs on the list
 */
static bool LookupPVConnectionsToPVList(void)
{
	Cardinal save_place;
	struct pv_info info;


	/* loop over all PVs on list */
	save_place = PVList.Location;
	while (PVList.Location < PVList.Number) {
		int ic;
		PinType *orig_pin;
		/* get pointer to data */
		orig_pin = (PVLIST_ENTRY(PVList.Location));
		info.pv = *orig_pin;

		/* Internal connection: if pins in the same element have the same
		   internal connection group number, they are connected */
		ic = GET_INTCONN(orig_pin);
		if ((info.pv.Element != NULL) && (ic > 0)) {
			ElementType *e = info.pv.Element;
			PIN_LOOP(e);
			{
				if ((orig_pin != pin) && (ic == GET_INTCONN(pin))) {
					if (!TEST_FLAG(TheFlag, pin))
						ADD_PV_TO_LIST(pin, PIN_TYPE, orig_pin, FCT_INTERNAL);
				}
			}
			END_LOOP;
		}


		EXPAND_BOUNDS(&info.pv);
		if (setjmp(info.env) == 0)
			r_search(PCB->Data->via_tree, (BoxType *) & info.pv, NULL, pv_pv_callback, &info, NULL);
		else
			return true;
		if (setjmp(info.env) == 0)
			r_search(PCB->Data->pin_tree, (BoxType *) & info.pv, NULL, pv_pv_callback, &info, NULL);
		else
			return true;
		PVList.Location++;
	}
	PVList.Location = save_place;
	return (false);
}

struct lo_info {
	Cardinal layer;
	LineType line;
	PadType pad;
	ArcType arc;
	PolygonType polygon;
	RatType rat;
	jmp_buf env;
};

static r_dir_t pv_line_callback(const BoxType * b, void *cl)
{
	PinTypePtr pv = (PinTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, pv) && PinLineIntersect(pv, &i->line)) {
		if (TEST_FLAG(HOLEFLAG, pv)) {
			SET_FLAG(WARNFLAG, pv);
			conf_core.temp.rat_warn = true;
			Message(_("WARNING: Hole too close to line.\n"));
		}
		else if (ADD_PV_TO_LIST(pv, LINE_TYPE, &i->line, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t pv_pad_callback(const BoxType * b, void *cl)
{
	PinTypePtr pv = (PinTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, pv) && IS_PV_ON_PAD(pv, &i->pad)) {
		if (TEST_FLAG(HOLEFLAG, pv)) {
			SET_FLAG(WARNFLAG, pv);
			conf_core.temp.rat_warn = true;
			Message(_("WARNING: Hole too close to pad.\n"));
		}
		else if (ADD_PV_TO_LIST(pv, PAD_TYPE, &i->pad, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t pv_arc_callback(const BoxType * b, void *cl)
{
	PinTypePtr pv = (PinTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, pv) && IS_PV_ON_ARC(pv, &i->arc)) {
		if (TEST_FLAG(HOLEFLAG, pv)) {
			SET_FLAG(WARNFLAG, pv);
			conf_core.temp.rat_warn = true;
			Message(_("WARNING: Hole touches arc.\n"));
		}
		else if (ADD_PV_TO_LIST(pv, ARC_TYPE, &i->arc, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t pv_poly_callback(const BoxType * b, void *cl)
{
	PinTypePtr pv = (PinTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	/* note that holes in polygons are ok, so they don't generate warnings. */
	if (!TEST_FLAG(TheFlag, pv) && !TEST_FLAG(HOLEFLAG, pv) &&
			(TEST_THERM(i->layer, pv) || !TEST_FLAG(CLEARPOLYFLAG, &i->polygon) || !pv->Clearance)) {
		if (TEST_FLAG(SQUAREFLAG, pv)) {
			Coord x1, x2, y1, y2;
			x1 = pv->X - (PIN_SIZE(pv) + 1 + Bloat) / 2;
			x2 = pv->X + (PIN_SIZE(pv) + 1 + Bloat) / 2;
			y1 = pv->Y - (PIN_SIZE(pv) + 1 + Bloat) / 2;
			y2 = pv->Y + (PIN_SIZE(pv) + 1 + Bloat) / 2;
			if (IsRectangleInPolygon(x1, y1, x2, y2, &i->polygon)
					&& ADD_PV_TO_LIST(pv, POLYGON_TYPE, &i->polygon, FCT_COPPER))
				longjmp(i->env, 1);
		}
		else if (TEST_FLAG(OCTAGONFLAG, pv)) {
			POLYAREA *oct = OctagonPoly(pv->X, pv->Y, PIN_SIZE(pv) / 2, GET_SQUARE(pv));
			if (isects(oct, &i->polygon, true) && ADD_PV_TO_LIST(pv, POLYGON_TYPE, &i->polygon, FCT_COPPER))
				longjmp(i->env, 1);
		}
		else {
			if (IsPointInPolygon(pv->X, pv->Y, PIN_SIZE(pv) * 0.5 + Bloat, &i->polygon)
					&& ADD_PV_TO_LIST(pv, POLYGON_TYPE, &i->polygon, FCT_COPPER))
				longjmp(i->env, 1);
		}
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t pv_rat_callback(const BoxType * b, void *cl)
{
	PinTypePtr pv = (PinTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	/* rats can't cause DRC so there is no early exit */
	if (!TEST_FLAG(TheFlag, pv) && IS_PV_ON_RAT(pv, &i->rat))
		ADD_PV_TO_LIST(pv, RATLINE_TYPE, &i->rat, FCT_RAT);
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches for new PVs that are connected to NEW LOs on the list
 * This routine updates the position counter of the lists too.
 */
static bool LookupPVConnectionsToLOList(bool AndRats)
{
	Cardinal layer;
	struct lo_info info;

	/* loop over all layers */
	for (layer = 0; layer < max_copper_layer; layer++) {
		if (LAYER_PTR(layer)->no_drc)
			continue;
		/* do nothing if there are no PV's */
		if (TotalP + TotalV == 0) {
			LineList[layer].Location = LineList[layer].Number;
			ArcList[layer].Location = ArcList[layer].Number;
			PolygonList[layer].Location = PolygonList[layer].Number;
			continue;
		}

		/* check all lines */
		while (LineList[layer].Location < LineList[layer].Number) {
			info.line = *(LINELIST_ENTRY(layer, LineList[layer].Location));
			EXPAND_BOUNDS(&info.line);
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->via_tree, (BoxType *) & info.line, NULL, pv_line_callback, &info, NULL);
			else
				return true;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pin_tree, (BoxType *) & info.line, NULL, pv_line_callback, &info, NULL);
			else
				return true;
			LineList[layer].Location++;
		}

		/* check all arcs */
		while (ArcList[layer].Location < ArcList[layer].Number) {
			info.arc = *(ARCLIST_ENTRY(layer, ArcList[layer].Location));
			EXPAND_BOUNDS(&info.arc);
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->via_tree, (BoxType *) & info.arc, NULL, pv_arc_callback, &info, NULL);
			else
				return true;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pin_tree, (BoxType *) & info.arc, NULL, pv_arc_callback, &info, NULL);
			else
				return true;
			ArcList[layer].Location++;
		}

		/* now all polygons */
		info.layer = layer;
		while (PolygonList[layer].Location < PolygonList[layer].Number) {
			info.polygon = *(POLYGONLIST_ENTRY(layer, PolygonList[layer].Location));
			EXPAND_BOUNDS(&info.polygon);
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->via_tree, (BoxType *) & info.polygon, NULL, pv_poly_callback, &info, NULL);
			else
				return true;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pin_tree, (BoxType *) & info.polygon, NULL, pv_poly_callback, &info, NULL);
			else
				return true;
			PolygonList[layer].Location++;
		}
	}

	/* loop over all pad-layers */
	for (layer = 0; layer < 2; layer++) {
		/* do nothing if there are no PV's */
		if (TotalP + TotalV == 0) {
			PadList[layer].Location = PadList[layer].Number;
			continue;
		}

		/* check all pads; for a detailed description see
		 * the handling of lines in this subroutine
		 */
		while (PadList[layer].Location < PadList[layer].Number) {
			info.pad = *(PADLIST_ENTRY(layer, PadList[layer].Location));
			EXPAND_BOUNDS(&info.pad);
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->via_tree, (BoxType *) & info.pad, NULL, pv_pad_callback, &info, NULL);
			else
				return true;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pin_tree, (BoxType *) & info.pad, NULL, pv_pad_callback, &info, NULL);
			else
				return true;
			PadList[layer].Location++;
		}
	}

	/* do nothing if there are no PV's */
	if (TotalP + TotalV == 0)
		RatList.Location = RatList.Number;

	/* check all rat-lines */
	if (AndRats) {
		while (RatList.Location < RatList.Number) {
			info.rat = *(RATLIST_ENTRY(RatList.Location));
			r_search_pt(PCB->Data->via_tree, &info.rat.Point1, 1, NULL, pv_rat_callback, &info, NULL);
			r_search_pt(PCB->Data->via_tree, &info.rat.Point2, 1, NULL, pv_rat_callback, &info, NULL);
			r_search_pt(PCB->Data->pin_tree, &info.rat.Point1, 1, NULL, pv_rat_callback, &info, NULL);
			r_search_pt(PCB->Data->pin_tree, &info.rat.Point2, 1, NULL, pv_rat_callback, &info, NULL);

			RatList.Location++;
		}
	}
	return (false);
}

r_dir_t pv_touch_callback(const BoxType * b, void *cl)
{
	PinTypePtr pin = (PinTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, pin) && PinLineIntersect(pin, &i->line))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoArcLine_callback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, line) && LineArcIntersect(line, &i->arc)) {
		if (ADD_LINE_TO_LIST(i->layer, line, ARC_TYPE, &i->arc, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoArcArc_callback(const BoxType * b, void *cl)
{
	ArcTypePtr arc = (ArcTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!TEST_FLAG(TheFlag, arc) && ArcArcIntersect(&i->arc, arc)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, ARC_TYPE, &i->arc, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoArcPad_callback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, pad) && i->layer == (TEST_FLAG(ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
			&& ArcPadIntersect(&i->arc, pad) && ADD_PAD_TO_LIST(i->layer, pad, ARC_TYPE, &i->arc, FCT_COPPER))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given arc on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at arc i
 */
static bool LookupLOConnectionsToArc(ArcTypePtr Arc, Cardinal LayerGroup)
{
	Cardinal entry;
	struct lo_info info;

	info.arc = *Arc;
	EXPAND_BOUNDS(&info.arc);
	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		Cardinal layer;

		layer = PCB->LayerGroups.Entries[LayerGroup][entry];

		/* handle normal layers */
		if (layer < max_copper_layer) {
			PolygonType *polygon;
			gdl_iterator_t it;

			info.layer = layer;
			/* add arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, &info.arc.BoundingBox, NULL, LOCtoArcLine_callback, &info, NULL);
			else
				return true;

			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, &info.arc.BoundingBox, NULL, LOCtoArcArc_callback, &info, NULL);
			else
				return true;

			/* now check all polygons */
			polylist_foreach(&(PCB->Data->Layer[layer].Polygon), &it, polygon) {
				if (!TEST_FLAG(TheFlag, polygon) && IsArcInPolygon(Arc, polygon)
						&& ADD_POLYGON_TO_LIST(layer, polygon, ARC_TYPE, Arc, FCT_COPPER))
					return true;
			}
		}
		else {
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pad_tree, &info.arc.BoundingBox, NULL, LOCtoArcPad_callback, &info, NULL);
			else
				return true;
		}
	}
	return (false);
}

static r_dir_t LOCtoLineLine_callback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, line) && LineLineIntersect(&i->line, line)) {
		if (ADD_LINE_TO_LIST(i->layer, line, LINE_TYPE, &i->line, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoLineArc_callback(const BoxType * b, void *cl)
{
	ArcTypePtr arc = (ArcTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!TEST_FLAG(TheFlag, arc) && LineArcIntersect(&i->line, arc)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, LINE_TYPE, &i->line, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoLineRat_callback(const BoxType * b, void *cl)
{
	RatTypePtr rat = (RatTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, rat)) {
		if ((rat->group1 == i->layer)
				&& IsRatPointOnLineEnd(&rat->Point1, &i->line)) {
			if (ADD_RAT_TO_LIST(rat, LINE_TYPE, &i->line, FCT_RAT))
				longjmp(i->env, 1);
		}
		else if ((rat->group2 == i->layer)
						 && IsRatPointOnLineEnd(&rat->Point2, &i->line)) {
			if (ADD_RAT_TO_LIST(rat, LINE_TYPE, &i->line, FCT_RAT))
				longjmp(i->env, 1);
		}
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoLinePad_callback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, pad) && i->layer == (TEST_FLAG(ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
			&& LinePadIntersect(&i->line, pad) && ADD_PAD_TO_LIST(i->layer, pad, LINE_TYPE, &i->line, FCT_COPPER))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given line on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at line i
 */
static bool LookupLOConnectionsToLine(LineTypePtr Line, Cardinal LayerGroup, bool PolysTo)
{
	Cardinal entry;
	struct lo_info info;

	info.line = *Line;
	info.layer = LayerGroup;
	EXPAND_BOUNDS(&info.line)
		/* add the new rat lines */
		if (setjmp(info.env) == 0)
		r_search(PCB->Data->rat_tree, &info.line.BoundingBox, NULL, LOCtoLineRat_callback, &info, NULL);
	else
		return true;

	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		Cardinal layer;

		layer = PCB->LayerGroups.Entries[LayerGroup][entry];

		/* handle normal layers */
		if (layer < max_copper_layer) {
			info.layer = layer;
			/* add lines */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, (BoxType *) & info.line, NULL, LOCtoLineLine_callback, &info, NULL);
			else
				return true;
			/* add arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, (BoxType *) & info.line, NULL, LOCtoLineArc_callback, &info, NULL);
			else
				return true;
			/* now check all polygons */
			if (PolysTo) {
				gdl_iterator_t it;
				PolygonType *polygon;

				polylist_foreach(&(PCB->Data->Layer[layer].Polygon), &it, polygon) {
					if (!TEST_FLAG(TheFlag, polygon) && IsLineInPolygon(Line, polygon)
							&& ADD_POLYGON_TO_LIST(layer, polygon, LINE_TYPE, Line, FCT_COPPER))
						return true;
				}
			}
		}
		else {
			/* handle special 'pad' layers */
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pad_tree, &info.line.BoundingBox, NULL, LOCtoLinePad_callback, &info, NULL);
			else
				return true;
		}
	}
	return (false);
}


struct rat_info {
	Cardinal layer;
	PointTypePtr Point;
	jmp_buf env;
};

static r_dir_t LOCtoRat_callback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct rat_info *i = (struct rat_info *) cl;

	if (!TEST_FLAG(TheFlag, line) &&
			((line->Point1.X == i->Point->X &&
				line->Point1.Y == i->Point->Y) || (line->Point2.X == i->Point->X && line->Point2.Y == i->Point->Y))) {
		if (ADD_LINE_TO_LIST(i->layer, line, RATLINE_TYPE, &i->Point, FCT_RAT))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t PolygonToRat_callback(const BoxType * b, void *cl)
{
	PolygonTypePtr polygon = (PolygonTypePtr) b;
	struct rat_info *i = (struct rat_info *) cl;

	if (!TEST_FLAG(TheFlag, polygon) && polygon->Clipped &&
			(i->Point->X == polygon->Clipped->contours->head.point[0]) &&
			(i->Point->Y == polygon->Clipped->contours->head.point[1])) {
		if (ADD_POLYGON_TO_LIST(i->layer, polygon, RATLINE_TYPE, &i->Point, FCT_RAT))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPad_callback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	struct rat_info *i = (struct rat_info *) cl;

	if (!TEST_FLAG(TheFlag, pad) && i->layer ==
			(TEST_FLAG(ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER) &&
			((pad->Point1.X == i->Point->X && pad->Point1.Y == i->Point->Y) ||
			 (pad->Point2.X == i->Point->X && pad->Point2.Y == i->Point->Y) ||
			 ((pad->Point1.X + pad->Point2.X) / 2 == i->Point->X &&
				(pad->Point1.Y + pad->Point2.Y) / 2 == i->Point->Y)) &&
			ADD_PAD_TO_LIST(i->layer, pad, RATLINE_TYPE, &i->Point, FCT_RAT))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given rat-line on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at line i
 */
static bool LookupLOConnectionsToRatEnd(PointTypePtr Point, Cardinal LayerGroup)
{
	Cardinal entry;
	struct rat_info info;

	info.Point = Point;
	/* loop over all layers of this group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		Cardinal layer;

		layer = PCB->LayerGroups.Entries[LayerGroup][entry];
		/* handle normal layers 
		   rats don't ever touch
		   arcs by definition
		 */

		if (layer < max_copper_layer) {
			info.layer = layer;
			if (setjmp(info.env) == 0)
				r_search_pt(LAYER_PTR(layer)->line_tree, Point, 1, NULL, LOCtoRat_callback, &info, NULL);
			else
				return true;
			if (setjmp(info.env) == 0)
				r_search_pt(LAYER_PTR(layer)->polygon_tree, Point, 1, NULL, PolygonToRat_callback, &info, NULL);
		}
		else {
			/* handle special 'pad' layers */
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search_pt(PCB->Data->pad_tree, Point, 1, NULL, LOCtoPad_callback, &info, NULL);
			else
				return true;
		}
	}
	return (false);
}

static r_dir_t LOCtoPadLine_callback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, line) && LinePadIntersect(line, &i->pad)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PAD_TYPE, &i->pad, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPadArc_callback(const BoxType * b, void *cl)
{
	ArcTypePtr arc = (ArcTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!TEST_FLAG(TheFlag, arc) && ArcPadIntersect(arc, &i->pad)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PAD_TYPE, &i->pad, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPadPoly_callback(const BoxType * b, void *cl)
{
	PolygonTypePtr polygon = (PolygonTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;


	if (!TEST_FLAG(TheFlag, polygon) && (!TEST_FLAG(CLEARPOLYFLAG, polygon) || !i->pad.Clearance)) {
		if (IsPadInPolygon(&i->pad, polygon) && ADD_POLYGON_TO_LIST(i->layer, polygon, PAD_TYPE, &i->pad, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPadRat_callback(const BoxType * b, void *cl)
{
	RatTypePtr rat = (RatTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, rat)) {
		if (rat->group1 == i->layer &&
				((rat->Point1.X == i->pad.Point1.X && rat->Point1.Y == i->pad.Point1.Y) ||
				 (rat->Point1.X == i->pad.Point2.X && rat->Point1.Y == i->pad.Point2.Y) ||
				 (rat->Point1.X == (i->pad.Point1.X + i->pad.Point2.X) / 2 &&
					rat->Point1.Y == (i->pad.Point1.Y + i->pad.Point2.Y) / 2))) {
			if (ADD_RAT_TO_LIST(rat, PAD_TYPE, &i->pad, FCT_RAT))
				longjmp(i->env, 1);
		}
		else if (rat->group2 == i->layer &&
						 ((rat->Point2.X == i->pad.Point1.X && rat->Point2.Y == i->pad.Point1.Y) ||
							(rat->Point2.X == i->pad.Point2.X && rat->Point2.Y == i->pad.Point2.Y) ||
							(rat->Point2.X == (i->pad.Point1.X + i->pad.Point2.X) / 2 &&
							 rat->Point2.Y == (i->pad.Point1.Y + i->pad.Point2.Y) / 2))) {
			if (ADD_RAT_TO_LIST(rat, PAD_TYPE, &i->pad, FCT_RAT))
				longjmp(i->env, 1);
		}
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPadPad_callback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, pad) && i->layer == (TEST_FLAG(ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
			&& PadPadIntersect(pad, &i->pad) && ADD_PAD_TO_LIST(i->layer, pad, PAD_TYPE, &i->pad, FCT_COPPER))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given pad on the given
 * layergroup. All found connections are added to the list
 */
static bool LookupLOConnectionsToPad(PadTypePtr Pad, Cardinal LayerGroup)
{
	Cardinal entry;
	struct lo_info info;
	int ic;
	bool retv = false;

	/* Internal connection: if pads in the same element have the same
	   internal connection group number, they are connected */
	ic = GET_INTCONN(Pad);
	if ((Pad->Element != NULL) && (ic > 0)) {
		ElementType *e = Pad->Element;
		PadTypePtr orig_pad = Pad;
		int tlayer = -1;

/*fprintf(stderr, "lg===\n");*/
		for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
			Cardinal layer;
			layer = PCB->LayerGroups.Entries[LayerGroup][entry];
/*fprintf(stderr, "lg: %d\n", layer);*/
			if (layer == COMPONENT_LAYER)
				tlayer = COMPONENT_LAYER;
			else if (layer == SOLDER_LAYER)
				tlayer = SOLDER_LAYER;
		}

/*fprintf(stderr, "tlayer=%d\n", tlayer);*/

		if (tlayer >= 0) {
			PAD_LOOP(e);
			{
				if ((orig_pad != pad) && (ic == GET_INTCONN(pad))) {
					int padlayer = TEST_FLAG(ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER;
/*fprintf(stderr, "layergroup1: %d {%d %d %d} %d \n", tlayer, TEST_FLAG(ONSOLDERFLAG, pad), SOLDER_LAYER, COMPONENT_LAYER, padlayer);*/
					if ((!TEST_FLAG(TheFlag, pad)) && (tlayer != padlayer)) {
/*fprintf(stderr, "layergroup2\n");*/
						ADD_PAD_TO_LIST(padlayer, pad, PAD_TYPE, orig_pad, FCT_INTERNAL);
						if (LookupLOConnectionsToPad(pad, LayerGroup))
							retv = true;
					}
				}
			}
			END_LOOP;
		}
	}


	if (!TEST_FLAG(SQUAREFLAG, Pad))
		return (LookupLOConnectionsToLine((LineTypePtr) Pad, LayerGroup, false));

	info.pad = *Pad;
	EXPAND_BOUNDS(&info.pad);
	/* add the new rat lines */
	info.layer = LayerGroup;
	if (setjmp(info.env) == 0)
		r_search(PCB->Data->rat_tree, &info.pad.BoundingBox, NULL, LOCtoPadRat_callback, &info, NULL);
	else
		return true;

	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		Cardinal layer;

		layer = PCB->LayerGroups.Entries[LayerGroup][entry];
		/* handle normal layers */
		if (layer < max_copper_layer) {
			info.layer = layer;
			/* add lines */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, &info.pad.BoundingBox, NULL, LOCtoPadLine_callback, &info, NULL);
			else
				return true;
			/* add arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, &info.pad.BoundingBox, NULL, LOCtoPadArc_callback, &info, NULL);
			else
				return true;
			/* add polygons */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->polygon_tree, &info.pad.BoundingBox, NULL, LOCtoPadPoly_callback, &info, NULL);
			else
				return true;
		}
		else {
			/* handle special 'pad' layers */
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pad_tree, (BoxType *) & info.pad, NULL, LOCtoPadPad_callback, &info, NULL);
			else
				return true;
		}

	}
	return retv;
}

static r_dir_t LOCtoPolyLine_callback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, line) && IsLineInPolygon(line, &i->polygon)) {
		if (ADD_LINE_TO_LIST(i->layer, line, POLYGON_TYPE, &i->polygon, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPolyArc_callback(const BoxType * b, void *cl)
{
	ArcTypePtr arc = (ArcTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!TEST_FLAG(TheFlag, arc) && IsArcInPolygon(arc, &i->polygon)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, POLYGON_TYPE, &i->polygon, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return 0;
}

static r_dir_t LOCtoPolyPad_callback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, pad) && i->layer == (TEST_FLAG(ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
			&& IsPadInPolygon(pad, &i->polygon)) {
		if (ADD_PAD_TO_LIST(i->layer, pad, POLYGON_TYPE, &i->polygon, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static r_dir_t LOCtoPolyRat_callback(const BoxType * b, void *cl)
{
	RatTypePtr rat = (RatTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, rat)) {
		if ((rat->Point1.X == (i->polygon.Clipped->contours->head.point[0]) &&
				 rat->Point1.Y == (i->polygon.Clipped->contours->head.point[1]) &&
				 rat->group1 == i->layer) ||
				(rat->Point2.X == (i->polygon.Clipped->contours->head.point[0]) &&
				 rat->Point2.Y == (i->polygon.Clipped->contours->head.point[1]) && rat->group2 == i->layer))
			if (ADD_RAT_TO_LIST(rat, POLYGON_TYPE, &i->polygon, FCT_RAT))
				longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}


/* ---------------------------------------------------------------------------
 * looks up LOs that are connected to the given polygon
 * on the given layergroup. All found connections are added to the list
 */
static bool LookupLOConnectionsToPolygon(PolygonTypePtr Polygon, Cardinal LayerGroup)
{
	Cardinal entry;
	struct lo_info info;

	if (!Polygon->Clipped)
		return false;
	info.polygon = *Polygon;
	EXPAND_BOUNDS(&info.polygon);
	info.layer = LayerGroup;
	/* check rats */
	if (setjmp(info.env) == 0)
		r_search(PCB->Data->rat_tree, (BoxType *) & info.polygon, NULL, LOCtoPolyRat_callback, &info, NULL);
	else
		return true;
/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		Cardinal layer;

		layer = PCB->LayerGroups.Entries[LayerGroup][entry];

		/* handle normal layers */
		if (layer < max_copper_layer) {
			gdl_iterator_t it;
			PolygonType *polygon;

			/* check all polygons */
			polylist_foreach(&(PCB->Data->Layer[layer].Polygon), &it, polygon) {
				if (!TEST_FLAG(TheFlag, polygon)
						&& IsPolygonInPolygon(polygon, Polygon)
						&& ADD_POLYGON_TO_LIST(layer, polygon, POLYGON_TYPE, Polygon, FCT_COPPER))
					return true;
			}

			info.layer = layer;
			/* check all lines */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, (BoxType *) & info.polygon, NULL, LOCtoPolyLine_callback, &info, NULL);
			else
				return true;
			/* check all arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, (BoxType *) & info.polygon, NULL, LOCtoPolyArc_callback, &info, NULL);
			else
				return true;
		}
		else {
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pad_tree, (BoxType *) & info.polygon, NULL, LOCtoPolyPad_callback, &info, NULL);
			else
				return true;
		}
	}
	return (false);
}
