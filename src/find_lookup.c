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

#include "compat_nls.h"
#include "board.h"

static inline pcb_r_dir_t r_search_pt(pcb_rtree_t * rtree, const pcb_point_t * pt,
															int radius,
															pcb_r_dir_t (*region_in_search) (const pcb_box_t * region, void *cl),
															pcb_r_dir_t (*rectangle_in_region) (const pcb_box_t * box, void *cl), void *closure,
															int *num_found)
{
	pcb_box_t box;

	box.X1 = pt->X - radius;
	box.X2 = pt->X + radius;
	box.Y1 = pt->Y - radius;
	box.Y2 = pt->Y + radius;

	return r_search(rtree, &box, region_in_search, rectangle_in_region, closure, num_found);
}


/* Connection lookup functions */

static pcb_bool ADD_PV_TO_LIST(pcb_pin_t *Pin, int from_type, void *from_ptr, pcb_found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(Pin->Element ? PCB_TYPE_PIN : PCB_TYPE_VIA, Pin->Element ? Pin->Element : Pin, Pin, Pin);
	PCB_FLAG_SET(TheFlag, Pin);
	make_callback(PCB_TYPE_PIN, Pin, from_type, from_ptr, type);
	PVLIST_ENTRY(PVList.Number) = Pin;
	PVList.Number++;
#ifdef DEBUG
	if (PVList.Number > PVList.Size)
		printf("ADD_PV_TO_LIST overflow! num=%d size=%d\n", PVList.Number, PVList.Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, Pin))
		return (SetThing(PCB_TYPE_PIN, Pin->Element, Pin, Pin));
	return pcb_false;
}

static pcb_bool ADD_PAD_TO_LIST(pcb_cardinal_t L, pcb_pad_t *Pad, int from_type, void *from_ptr, pcb_found_conn_type_t type)
{
/*fprintf(stderr, "ADD_PAD_TO_LIST cardinal %d %p %d\n", L, Pad, from_type);*/
	if (User)
		AddObjectToFlagUndoList(PCB_TYPE_PAD, Pad->Element, Pad, Pad);
	PCB_FLAG_SET(TheFlag, Pad);
	make_callback(PCB_TYPE_PAD, Pad, from_type, from_ptr, type);
	PADLIST_ENTRY((L), PadList[(L)].Number) = Pad;
	PadList[(L)].Number++;
#ifdef DEBUG
	if (PadList[(L)].Number > PadList[(L)].Size)
		printf("ADD_PAD_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, PadList[(L)].Number, PadList[(L)].Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, Pad))
		return (SetThing(PCB_TYPE_PAD, Pad->Element, Pad, Pad));
	return pcb_false;
}

static pcb_bool ADD_LINE_TO_LIST(pcb_cardinal_t L, pcb_line_t *Ptr, int from_type, void *from_ptr, pcb_found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(PCB_TYPE_LINE, LAYER_PTR(L), (Ptr), (Ptr));
	PCB_FLAG_SET(TheFlag, (Ptr));
	make_callback(PCB_TYPE_LINE, Ptr, from_type, from_ptr, type);
	LINELIST_ENTRY((L), LineList[(L)].Number) = (Ptr);
	LineList[(L)].Number++;
#ifdef DEBUG
	if (LineList[(L)].Number > LineList[(L)].Size)
		printf("ADD_LINE_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, LineList[(L)].Number, LineList[(L)].Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, (Ptr)))
		return (SetThing(PCB_TYPE_LINE, LAYER_PTR(L), (Ptr), (Ptr)));
	return pcb_false;
}

static pcb_bool ADD_ARC_TO_LIST(pcb_cardinal_t L, pcb_arc_t *Ptr, int from_type, void *from_ptr, pcb_found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(PCB_TYPE_ARC, LAYER_PTR(L), (Ptr), (Ptr));
	PCB_FLAG_SET(TheFlag, (Ptr));
	make_callback(PCB_TYPE_ARC, Ptr, from_type, from_ptr, type);
	ARCLIST_ENTRY((L), ArcList[(L)].Number) = (Ptr);
	ArcList[(L)].Number++;
#ifdef DEBUG
	if (ArcList[(L)].Number > ArcList[(L)].Size)
		printf("ADD_ARC_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, ArcList[(L)].Number, ArcList[(L)].Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, (Ptr)))
		return (SetThing(PCB_TYPE_ARC, LAYER_PTR(L), (Ptr), (Ptr)));
	return pcb_false;
}

static pcb_bool ADD_RAT_TO_LIST(pcb_rat_t *Ptr, int from_type, void *from_ptr, pcb_found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(PCB_TYPE_RATLINE, (Ptr), (Ptr), (Ptr));
	PCB_FLAG_SET(TheFlag, (Ptr));
	make_callback(PCB_TYPE_RATLINE, Ptr, from_type, from_ptr, type);
	RATLIST_ENTRY(RatList.Number) = (Ptr);
	RatList.Number++;
#ifdef DEBUG
	if (RatList.Number > RatList.Size)
		printf("ADD_RAT_TO_LIST overflow! num=%d size=%d\n", RatList.Number, RatList.Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, (Ptr)))
		return (SetThing(PCB_TYPE_RATLINE, (Ptr), (Ptr), (Ptr)));
	return pcb_false;
}

static pcb_bool ADD_POLYGON_TO_LIST(pcb_cardinal_t L, pcb_polygon_t *Ptr, int from_type, void *from_ptr, pcb_found_conn_type_t type)
{
	if (User)
		AddObjectToFlagUndoList(PCB_TYPE_POLYGON, LAYER_PTR(L), (Ptr), (Ptr));
	PCB_FLAG_SET(TheFlag, (Ptr));
	make_callback(PCB_TYPE_POLYGON, Ptr, from_type, from_ptr, type);
	POLYGONLIST_ENTRY((L), PolygonList[(L)].Number) = (Ptr);
	PolygonList[(L)].Number++;
#ifdef DEBUG
	if (PolygonList[(L)].Number > PolygonList[(L)].Size)
		printf("ADD_ARC_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, PolygonList[(L)].Number, PolygonList[(L)].Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, (Ptr)))
		return (SetThing(PCB_TYPE_POLYGON, LAYER_PTR(L), (Ptr), (Ptr)));
	return pcb_false;
}

pcb_bool SetThing(int type, void *ptr1, void *ptr2, void *ptr3)
{
	thing_ptr1 = ptr1;
	thing_ptr2 = ptr2;
	thing_ptr3 = ptr3;
	thing_type = type;
	if (type == PCB_TYPE_PIN && ptr1 == NULL) {
		thing_ptr1 = ptr3;
		thing_type = PCB_TYPE_VIA;
	}
	return pcb_true;
}

/* ---------------------------------------------------------------------------
 * releases all allocated memory
 */
void pcb_layout_lookup_uninit(void)
{
	pcb_cardinal_t i;

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

void pcb_component_lookup_uninit(void)
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
void pcb_component_lookup_init(void)
{
	pcb_cardinal_t i;

	/* initialize pad data; start by counting the total number
	 * on each of the two possible layers
	 */
	NumberOfPads[COMPONENT_LAYER] = NumberOfPads[SOLDER_LAYER] = 0;
	PCB_PAD_ALL_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad))
			NumberOfPads[SOLDER_LAYER]++;
		else
			NumberOfPads[COMPONENT_LAYER]++;
	}
	ENDALL_LOOP;
	for (i = 0; i < 2; i++) {
/*fprintf(stderr, "PadList alloc %d: %d\n", i, NumberOfPads[i]);*/

		/* allocate memory for working list */
		PadList[i].Data = (void **) calloc(NumberOfPads[i], sizeof(pcb_pad_t *));

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
void pcb_layout_lookup_init(void)
{
	pcb_cardinal_t i;

	/* initialize line arc and polygon data */
	for (i = 0; i < max_copper_layer; i++) {
		pcb_layer_t *layer = LAYER_PTR(i);

		if (linelist_length(&layer->Line)) {
			LineList[i].Size = linelist_length(&layer->Line);
			LineList[i].Data = (void **) calloc(LineList[i].Size, sizeof(pcb_line_t *));
		}
		if (arclist_length(&layer->Arc)) {
			ArcList[i].Size = arclist_length(&layer->Arc);
			ArcList[i].Data = (void **) calloc(ArcList[i].Size, sizeof(pcb_arc_t *));
		}
		if (polylist_length(&layer->Polygon)) {
			PolygonList[i].Size = polylist_length(&layer->Polygon);
			PolygonList[i].Data = (void **) calloc(PolygonList[i].Size, sizeof(pcb_polygon_t *));
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
	PVList.Data = (void **) calloc(TotalP + TotalV, sizeof(pcb_pin_t *));
	PVList.Size = TotalP + TotalV;
	PVList.Location = 0;
	PVList.DrawLocation = 0;
	PVList.Number = 0;
	/* Initialize ratline data */
	RatList.Size = ratlist_length(&PCB->Data->Rat);
	RatList.Data = (void **) calloc(RatList.Size, sizeof(pcb_rat_t *));
	RatList.Location = 0;
	RatList.DrawLocation = 0;
	RatList.Number = 0;
}

struct pv_info {
	pcb_cardinal_t layer;
	pcb_pin_t pv;
	jmp_buf env;
};

static pcb_r_dir_t LOCtoPVline_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_intersect_line_pin(&i->pv, line) && !PCB_FLAG_TEST(PCB_FLAG_HOLE, &i->pv)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_TYPE_PIN, &i->pv, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPVarc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, arc) && IS_PV_ON_ARC(&i->pv, arc) && !PCB_FLAG_TEST(PCB_FLAG_HOLE, &i->pv)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_TYPE_PIN, &i->pv, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPVpad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pad) && IS_PV_ON_PAD(&i->pv, pad) &&
			!PCB_FLAG_TEST(PCB_FLAG_HOLE, &i->pv) &&
			ADD_PAD_TO_LIST(PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER, pad, PCB_TYPE_PIN, &i->pv, FCT_COPPER))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPVrat_callback(const pcb_box_t * b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, rat) && IS_PV_ON_RAT(&i->pv, rat) && ADD_RAT_TO_LIST(rat, PCB_TYPE_PIN, &i->pv, FCT_RAT))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPVpoly_callback(const pcb_box_t * b, void *cl)
{
	pcb_polygon_t *polygon = (pcb_polygon_t *) b;
	struct pv_info *i = (struct pv_info *) cl;

	/* if the pin doesn't have a therm and polygon is clearing
	 * then it can't touch due to clearance, so skip the expensive
	 * test. If it does have a therm, you still need to test
	 * because it might not be inside the polygon, or it could
	 * be on an edge such that it doesn't actually touch.
	 */
	if (!PCB_FLAG_TEST(TheFlag, polygon) && !PCB_FLAG_TEST(PCB_FLAG_HOLE, &i->pv) &&
			(PCB_FLAG_THERM_TEST(i->layer, &i->pv) || !PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, polygon)
			 || !i->pv.Clearance)) {
		double wide = MAX(0.5 * i->pv.Thickness + Bloat, 0);
		if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, &i->pv)) {
			pcb_coord_t x1 = i->pv.X - (i->pv.Thickness + 1 + Bloat) / 2;
			pcb_coord_t x2 = i->pv.X + (i->pv.Thickness + 1 + Bloat) / 2;
			pcb_coord_t y1 = i->pv.Y - (i->pv.Thickness + 1 + Bloat) / 2;
			pcb_coord_t y2 = i->pv.Y + (i->pv.Thickness + 1 + Bloat) / 2;
			if (IsRectangleInPolygon(x1, y1, x2, y2, polygon)
					&& ADD_POLYGON_TO_LIST(i->layer, polygon, PCB_TYPE_PIN, &i->pv, FCT_COPPER))
				longjmp(i->env, 1);
		}
		else if (PCB_FLAG_TEST(PCB_FLAG_OCTAGON, &i->pv)) {
			pcb_polyarea_t *oct = OctagonPoly(i->pv.X, i->pv.Y, i->pv.Thickness / 2, PCB_FLAG_SQUARE_GET(&i->pv));
			if (isects(oct, polygon, pcb_true)
					&& ADD_POLYGON_TO_LIST(i->layer, polygon, PCB_TYPE_PIN, &i->pv, FCT_COPPER))
				longjmp(i->env, 1);
		}
		else if (IsPointInPolygon(i->pv.X, i->pv.Y, wide, polygon)
						 && ADD_POLYGON_TO_LIST(i->layer, polygon, PCB_TYPE_PIN, &i->pv, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * checks if a PV is connected to LOs, if it is, the LO is added to
 * the appropriate list and the 'used' flag is set
 */
static pcb_bool LookupLOConnectionsToPVList(pcb_bool AndRats)
{
	pcb_cardinal_t layer;
	struct pv_info info;

	/* loop over all PVs currently on list */
	while (PVList.Location < PVList.Number) {
		/* get pointer to data */
		info.pv = *(PVLIST_ENTRY(PVList.Location));
		EXPAND_BOUNDS(&info.pv);

		/* check pads */
		if (setjmp(info.env) == 0)
			r_search(PCB->Data->pad_tree, (pcb_box_t *) & info.pv, NULL, LOCtoPVpad_callback, &info, NULL);
		else
			return pcb_true;

		/* now all lines, arcs and polygons of the several layers */
		for (layer = 0; layer < max_copper_layer; layer++) {
			if (LAYER_PTR(layer)->no_drc)
				continue;
			info.layer = layer;
			/* add touching lines */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, (pcb_box_t *) & info.pv, NULL, LOCtoPVline_callback, &info, NULL);
			else
				return pcb_true;
			/* add touching arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, (pcb_box_t *) & info.pv, NULL, LOCtoPVarc_callback, &info, NULL);
			else
				return pcb_true;
			/* check all polygons */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->polygon_tree, (pcb_box_t *) & info.pv, NULL, LOCtoPVpoly_callback, &info, NULL);
			else
				return pcb_true;
		}
		/* Check for rat-lines that may intersect the PV */
		if (AndRats) {
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->rat_tree, (pcb_box_t *) & info.pv, NULL, LOCtoPVrat_callback, &info, NULL);
			else
				return pcb_true;
		}
		PVList.Location++;
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * find all connections between LO at the current list position and new LOs
 */
static pcb_bool LookupLOConnectionsToLOList(pcb_bool AndRats)
{
	pcb_bool done;
	pcb_cardinal_t i, group, layer, ratposition,
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
		pcb_cardinal_t *position;

		if (AndRats) {
			position = &ratposition;
			for (; *position < RatList.Number; (*position)++) {
				group = RATLIST_ENTRY(*position)->group1;
				if (LookupLOConnectionsToRatEnd(&(RATLIST_ENTRY(*position)->Point1), group))
					return (pcb_true);
				group = RATLIST_ENTRY(*position)->group2;
				if (LookupLOConnectionsToRatEnd(&(RATLIST_ENTRY(*position)->Point2), group))
					return (pcb_true);
			}
		}
		/* loop over all layergroups */
		for (group = 0; group < max_group; group++) {
			pcb_cardinal_t entry;

			for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++) {
				layer = PCB->LayerGroups.Entries[group][entry];

				/* be aware that the layer number equal max_copper_layer
				 * and max_copper_layer+1 have a special meaning for pads
				 */
				if (layer < max_copper_layer) {
					/* try all new lines */
					position = &lineposition[layer];
					for (; *position < LineList[layer].Number; (*position)++)
						if (LookupLOConnectionsToLine(LINELIST_ENTRY(layer, *position), group, pcb_true))
							return (pcb_true);

					/* try all new arcs */
					position = &arcposition[layer];
					for (; *position < ArcList[layer].Number; (*position)++)
						if (LookupLOConnectionsToArc(ARCLIST_ENTRY(layer, *position), group))
							return (pcb_true);

					/* try all new polygons */
					position = &polyposition[layer];
					for (; *position < PolygonList[layer].Number; (*position)++)
						if (LookupLOConnectionsToPolygon(POLYGONLIST_ENTRY(layer, *position), group))
							return (pcb_true);
				}
				else {
					/* try all new pads */
					layer -= max_copper_layer;
					if (layer > 1) {
						pcb_message(PCB_MSG_DEFAULT, _("bad layer number %d max_copper_layer=%d in find.c\n"), layer, max_copper_layer);
						return pcb_false;
					}
					position = &padposition[layer];
					for (; *position < PadList[layer].Number; (*position)++)
						if (LookupLOConnectionsToPad(PADLIST_ENTRY(layer, *position), group))
							return (pcb_true);
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
	return (pcb_false);
}

static pcb_r_dir_t pv_pv_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pin = (pcb_pin_t *) b;
	struct pv_info *i = (struct pv_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pin) && PV_TOUCH_PV(&i->pv, pin)) {
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin) || PCB_FLAG_TEST(PCB_FLAG_HOLE, &i->pv)) {
			PCB_FLAG_SET(PCB_FLAG_WARN, pin);
			conf_core.temp.rat_warn = pcb_true;
			if (pin->Element)
				pcb_message(PCB_MSG_DEFAULT, _("WARNING: Hole too close to pin.\n"));
			else
				pcb_message(PCB_MSG_DEFAULT, _("WARNING: Hole too close to via.\n"));
		}
		else if (ADD_PV_TO_LIST(pin, PCB_TYPE_PIN, &i->pv, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches for new PVs that are connected to PVs on the list
 */
static pcb_bool LookupPVConnectionsToPVList(void)
{
	pcb_cardinal_t save_place;
	struct pv_info info;


	/* loop over all PVs on list */
	save_place = PVList.Location;
	while (PVList.Location < PVList.Number) {
		int ic;
		pcb_pin_t *orig_pin;
		/* get pointer to data */
		orig_pin = (PVLIST_ENTRY(PVList.Location));
		info.pv = *orig_pin;

		/* Internal connection: if pins in the same element have the same
		   internal connection group number, they are connected */
		ic = PCB_FLAG_INTCONN_GET(orig_pin);
		if ((info.pv.Element != NULL) && (ic > 0)) {
			pcb_element_t *e = info.pv.Element;
			PCB_PIN_LOOP(e);
			{
				if ((orig_pin != pin) && (ic == PCB_FLAG_INTCONN_GET(pin))) {
					if (!PCB_FLAG_TEST(TheFlag, pin))
						ADD_PV_TO_LIST(pin, PCB_TYPE_PIN, orig_pin, FCT_INTERNAL);
				}
			}
			END_LOOP;
		}


		EXPAND_BOUNDS(&info.pv);
		if (setjmp(info.env) == 0)
			r_search(PCB->Data->via_tree, (pcb_box_t *) & info.pv, NULL, pv_pv_callback, &info, NULL);
		else
			return pcb_true;
		if (setjmp(info.env) == 0)
			r_search(PCB->Data->pin_tree, (pcb_box_t *) & info.pv, NULL, pv_pv_callback, &info, NULL);
		else
			return pcb_true;
		PVList.Location++;
	}
	PVList.Location = save_place;
	return (pcb_false);
}

struct lo_info {
	pcb_cardinal_t layer;
	pcb_line_t line;
	pcb_pad_t pad;
	pcb_arc_t arc;
	pcb_polygon_t polygon;
	pcb_rat_t rat;
	jmp_buf env;
};

static pcb_r_dir_t pv_line_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pv = (pcb_pin_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pv) && pcb_intersect_line_pin(pv, &i->line)) {
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)) {
			PCB_FLAG_SET(PCB_FLAG_WARN, pv);
			conf_core.temp.rat_warn = pcb_true;
			pcb_message(PCB_MSG_DEFAULT, _("WARNING: Hole too close to line.\n"));
		}
		else if (ADD_PV_TO_LIST(pv, PCB_TYPE_LINE, &i->line, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t pv_pad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pv = (pcb_pin_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pv) && IS_PV_ON_PAD(pv, &i->pad)) {
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)) {
			PCB_FLAG_SET(PCB_FLAG_WARN, pv);
			conf_core.temp.rat_warn = pcb_true;
			pcb_message(PCB_MSG_DEFAULT, _("WARNING: Hole too close to pad.\n"));
		}
		else if (ADD_PV_TO_LIST(pv, PCB_TYPE_PAD, &i->pad, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t pv_arc_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pv = (pcb_pin_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pv) && IS_PV_ON_ARC(pv, &i->arc)) {
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)) {
			PCB_FLAG_SET(PCB_FLAG_WARN, pv);
			conf_core.temp.rat_warn = pcb_true;
			pcb_message(PCB_MSG_DEFAULT, _("WARNING: Hole touches arc.\n"));
		}
		else if (ADD_PV_TO_LIST(pv, PCB_TYPE_ARC, &i->arc, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t pv_poly_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pv = (pcb_pin_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	/* note that holes in polygons are ok, so they don't generate warnings. */
	if (!PCB_FLAG_TEST(TheFlag, pv) && !PCB_FLAG_TEST(PCB_FLAG_HOLE, pv) &&
			(PCB_FLAG_THERM_TEST(i->layer, pv) || !PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, &i->polygon) || !pv->Clearance)) {
		if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pv)) {
			pcb_coord_t x1, x2, y1, y2;
			x1 = pv->X - (PIN_SIZE(pv) + 1 + Bloat) / 2;
			x2 = pv->X + (PIN_SIZE(pv) + 1 + Bloat) / 2;
			y1 = pv->Y - (PIN_SIZE(pv) + 1 + Bloat) / 2;
			y2 = pv->Y + (PIN_SIZE(pv) + 1 + Bloat) / 2;
			if (IsRectangleInPolygon(x1, y1, x2, y2, &i->polygon)
					&& ADD_PV_TO_LIST(pv, PCB_TYPE_POLYGON, &i->polygon, FCT_COPPER))
				longjmp(i->env, 1);
		}
		else if (PCB_FLAG_TEST(PCB_FLAG_OCTAGON, pv)) {
			pcb_polyarea_t *oct = OctagonPoly(pv->X, pv->Y, PIN_SIZE(pv) / 2, PCB_FLAG_SQUARE_GET(pv));
			if (isects(oct, &i->polygon, pcb_true) && ADD_PV_TO_LIST(pv, PCB_TYPE_POLYGON, &i->polygon, FCT_COPPER))
				longjmp(i->env, 1);
		}
		else {
			if (IsPointInPolygon(pv->X, pv->Y, PIN_SIZE(pv) * 0.5 + Bloat, &i->polygon)
					&& ADD_PV_TO_LIST(pv, PCB_TYPE_POLYGON, &i->polygon, FCT_COPPER))
				longjmp(i->env, 1);
		}
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t pv_rat_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pv = (pcb_pin_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	/* rats can't cause DRC so there is no early exit */
	if (!PCB_FLAG_TEST(TheFlag, pv) && IS_PV_ON_RAT(pv, &i->rat))
		ADD_PV_TO_LIST(pv, PCB_TYPE_RATLINE, &i->rat, FCT_RAT);
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches for new PVs that are connected to NEW LOs on the list
 * This routine updates the position counter of the lists too.
 */
static pcb_bool LookupPVConnectionsToLOList(pcb_bool AndRats)
{
	pcb_cardinal_t layer;
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
				r_search(PCB->Data->via_tree, (pcb_box_t *) & info.line, NULL, pv_line_callback, &info, NULL);
			else
				return pcb_true;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pin_tree, (pcb_box_t *) & info.line, NULL, pv_line_callback, &info, NULL);
			else
				return pcb_true;
			LineList[layer].Location++;
		}

		/* check all arcs */
		while (ArcList[layer].Location < ArcList[layer].Number) {
			info.arc = *(ARCLIST_ENTRY(layer, ArcList[layer].Location));
			EXPAND_BOUNDS(&info.arc);
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->via_tree, (pcb_box_t *) & info.arc, NULL, pv_arc_callback, &info, NULL);
			else
				return pcb_true;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pin_tree, (pcb_box_t *) & info.arc, NULL, pv_arc_callback, &info, NULL);
			else
				return pcb_true;
			ArcList[layer].Location++;
		}

		/* now all polygons */
		info.layer = layer;
		while (PolygonList[layer].Location < PolygonList[layer].Number) {
			info.polygon = *(POLYGONLIST_ENTRY(layer, PolygonList[layer].Location));
			EXPAND_BOUNDS(&info.polygon);
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->via_tree, (pcb_box_t *) & info.polygon, NULL, pv_poly_callback, &info, NULL);
			else
				return pcb_true;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pin_tree, (pcb_box_t *) & info.polygon, NULL, pv_poly_callback, &info, NULL);
			else
				return pcb_true;
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
				r_search(PCB->Data->via_tree, (pcb_box_t *) & info.pad, NULL, pv_pad_callback, &info, NULL);
			else
				return pcb_true;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pin_tree, (pcb_box_t *) & info.pad, NULL, pv_pad_callback, &info, NULL);
			else
				return pcb_true;
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
	return (pcb_false);
}

pcb_r_dir_t pv_touch_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pin = (pcb_pin_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pin) && pcb_intersect_line_pin(pin, &i->line))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoArcLine_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_intersect_line_arc(line, &i->arc)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_TYPE_ARC, &i->arc, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoArcArc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!PCB_FLAG_TEST(TheFlag, arc) && ArcArcIntersect(&i->arc, arc)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_TYPE_ARC, &i->arc, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoArcPad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pad) && i->layer == (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
			&& pcb_intersect_arc_pad(&i->arc, pad) && ADD_PAD_TO_LIST(i->layer, pad, PCB_TYPE_ARC, &i->arc, FCT_COPPER))
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
static pcb_bool LookupLOConnectionsToArc(pcb_arc_t *Arc, pcb_cardinal_t LayerGroup)
{
	pcb_cardinal_t entry;
	struct lo_info info;

	info.arc = *Arc;
	EXPAND_BOUNDS(&info.arc);
	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		pcb_cardinal_t layer;

		layer = PCB->LayerGroups.Entries[LayerGroup][entry];

		/* handle normal layers */
		if (layer < max_copper_layer) {
			pcb_polygon_t *polygon;
			gdl_iterator_t it;

			info.layer = layer;
			/* add arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, &info.arc.BoundingBox, NULL, LOCtoArcLine_callback, &info, NULL);
			else
				return pcb_true;

			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, &info.arc.BoundingBox, NULL, LOCtoArcArc_callback, &info, NULL);
			else
				return pcb_true;

			/* now check all polygons */
			polylist_foreach(&(PCB->Data->Layer[layer].Polygon), &it, polygon) {
				if (!PCB_FLAG_TEST(TheFlag, polygon) && pcb_is_arc_in_poly(Arc, polygon)
						&& ADD_POLYGON_TO_LIST(layer, polygon, PCB_TYPE_ARC, Arc, FCT_COPPER))
					return pcb_true;
			}
		}
		else {
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pad_tree, &info.arc.BoundingBox, NULL, LOCtoArcPad_callback, &info, NULL);
			else
				return pcb_true;
		}
	}
	return (pcb_false);
}

static pcb_r_dir_t LOCtoLineLine_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_intersect_line_line(&i->line, line)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_TYPE_LINE, &i->line, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoLineArc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!PCB_FLAG_TEST(TheFlag, arc) && pcb_intersect_line_arc(&i->line, arc)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_TYPE_LINE, &i->line, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoLineRat_callback(const pcb_box_t * b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, rat)) {
		if ((rat->group1 == i->layer)
				&& IsRatPointOnLineEnd(&rat->Point1, &i->line)) {
			if (ADD_RAT_TO_LIST(rat, PCB_TYPE_LINE, &i->line, FCT_RAT))
				longjmp(i->env, 1);
		}
		else if ((rat->group2 == i->layer)
						 && IsRatPointOnLineEnd(&rat->Point2, &i->line)) {
			if (ADD_RAT_TO_LIST(rat, PCB_TYPE_LINE, &i->line, FCT_RAT))
				longjmp(i->env, 1);
		}
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoLinePad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pad) && i->layer == (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
			&& pcb_intersect_line_pad(&i->line, pad) && ADD_PAD_TO_LIST(i->layer, pad, PCB_TYPE_LINE, &i->line, FCT_COPPER))
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
static pcb_bool LookupLOConnectionsToLine(pcb_line_t *Line, pcb_cardinal_t LayerGroup, pcb_bool PolysTo)
{
	pcb_cardinal_t entry;
	struct lo_info info;

	info.line = *Line;
	info.layer = LayerGroup;
	EXPAND_BOUNDS(&info.line)
		/* add the new rat lines */
		if (setjmp(info.env) == 0)
		r_search(PCB->Data->rat_tree, &info.line.BoundingBox, NULL, LOCtoLineRat_callback, &info, NULL);
	else
		return pcb_true;

	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		pcb_cardinal_t layer;

		layer = PCB->LayerGroups.Entries[LayerGroup][entry];

		/* handle normal layers */
		if (layer < max_copper_layer) {
			info.layer = layer;
			/* add lines */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, (pcb_box_t *) & info.line, NULL, LOCtoLineLine_callback, &info, NULL);
			else
				return pcb_true;
			/* add arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, (pcb_box_t *) & info.line, NULL, LOCtoLineArc_callback, &info, NULL);
			else
				return pcb_true;
			/* now check all polygons */
			if (PolysTo) {
				gdl_iterator_t it;
				pcb_polygon_t *polygon;

				polylist_foreach(&(PCB->Data->Layer[layer].Polygon), &it, polygon) {
					if (!PCB_FLAG_TEST(TheFlag, polygon) && pcb_is_line_in_poly(Line, polygon)
							&& ADD_POLYGON_TO_LIST(layer, polygon, PCB_TYPE_LINE, Line, FCT_COPPER))
						return pcb_true;
				}
			}
		}
		else {
			/* handle special 'pad' layers */
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pad_tree, &info.line.BoundingBox, NULL, LOCtoLinePad_callback, &info, NULL);
			else
				return pcb_true;
		}
	}
	return (pcb_false);
}


struct rat_info {
	pcb_cardinal_t layer;
	pcb_point_t *Point;
	jmp_buf env;
};

static pcb_r_dir_t LOCtoRat_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct rat_info *i = (struct rat_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) &&
			((line->Point1.X == i->Point->X &&
				line->Point1.Y == i->Point->Y) || (line->Point2.X == i->Point->X && line->Point2.Y == i->Point->Y))) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_TYPE_RATLINE, &i->Point, FCT_RAT))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t PolygonToRat_callback(const pcb_box_t * b, void *cl)
{
	pcb_polygon_t *polygon = (pcb_polygon_t *) b;
	struct rat_info *i = (struct rat_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, polygon) && polygon->Clipped &&
			(i->Point->X == polygon->Clipped->contours->head.point[0]) &&
			(i->Point->Y == polygon->Clipped->contours->head.point[1])) {
		if (ADD_POLYGON_TO_LIST(i->layer, polygon, PCB_TYPE_RATLINE, &i->Point, FCT_RAT))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	struct rat_info *i = (struct rat_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pad) && i->layer ==
			(PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER) &&
			((pad->Point1.X == i->Point->X && pad->Point1.Y == i->Point->Y) ||
			 (pad->Point2.X == i->Point->X && pad->Point2.Y == i->Point->Y) ||
			 ((pad->Point1.X + pad->Point2.X) / 2 == i->Point->X &&
				(pad->Point1.Y + pad->Point2.Y) / 2 == i->Point->Y)) &&
			ADD_PAD_TO_LIST(i->layer, pad, PCB_TYPE_RATLINE, &i->Point, FCT_RAT))
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
static pcb_bool LookupLOConnectionsToRatEnd(pcb_point_t *Point, pcb_cardinal_t LayerGroup)
{
	pcb_cardinal_t entry;
	struct rat_info info;

	info.Point = Point;
	/* loop over all layers of this group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		pcb_cardinal_t layer;

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
				return pcb_true;
			if (setjmp(info.env) == 0)
				r_search_pt(LAYER_PTR(layer)->polygon_tree, Point, 1, NULL, PolygonToRat_callback, &info, NULL);
		}
		else {
			/* handle special 'pad' layers */
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search_pt(PCB->Data->pad_tree, Point, 1, NULL, LOCtoPad_callback, &info, NULL);
			else
				return pcb_true;
		}
	}
	return (pcb_false);
}

static pcb_r_dir_t LOCtoPadLine_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_intersect_line_pad(line, &i->pad)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_TYPE_PAD, &i->pad, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPadArc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!PCB_FLAG_TEST(TheFlag, arc) && pcb_intersect_arc_pad(arc, &i->pad)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_TYPE_PAD, &i->pad, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPadPoly_callback(const pcb_box_t * b, void *cl)
{
	pcb_polygon_t *polygon = (pcb_polygon_t *) b;
	struct lo_info *i = (struct lo_info *) cl;


	if (!PCB_FLAG_TEST(TheFlag, polygon) && (!PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, polygon) || !i->pad.Clearance)) {
		if (pcb_is_pad_in_poly(&i->pad, polygon) && ADD_POLYGON_TO_LIST(i->layer, polygon, PCB_TYPE_PAD, &i->pad, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPadRat_callback(const pcb_box_t * b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, rat)) {
		if (rat->group1 == i->layer &&
				((rat->Point1.X == i->pad.Point1.X && rat->Point1.Y == i->pad.Point1.Y) ||
				 (rat->Point1.X == i->pad.Point2.X && rat->Point1.Y == i->pad.Point2.Y) ||
				 (rat->Point1.X == (i->pad.Point1.X + i->pad.Point2.X) / 2 &&
					rat->Point1.Y == (i->pad.Point1.Y + i->pad.Point2.Y) / 2))) {
			if (ADD_RAT_TO_LIST(rat, PCB_TYPE_PAD, &i->pad, FCT_RAT))
				longjmp(i->env, 1);
		}
		else if (rat->group2 == i->layer &&
						 ((rat->Point2.X == i->pad.Point1.X && rat->Point2.Y == i->pad.Point1.Y) ||
							(rat->Point2.X == i->pad.Point2.X && rat->Point2.Y == i->pad.Point2.Y) ||
							(rat->Point2.X == (i->pad.Point1.X + i->pad.Point2.X) / 2 &&
							 rat->Point2.Y == (i->pad.Point1.Y + i->pad.Point2.Y) / 2))) {
			if (ADD_RAT_TO_LIST(rat, PCB_TYPE_PAD, &i->pad, FCT_RAT))
				longjmp(i->env, 1);
		}
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPadPad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pad) && i->layer == (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
			&& PadPadIntersect(pad, &i->pad) && ADD_PAD_TO_LIST(i->layer, pad, PCB_TYPE_PAD, &i->pad, FCT_COPPER))
		longjmp(i->env, 1);
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches all LOs that are connected to the given pad on the given
 * layergroup. All found connections are added to the list
 */
static pcb_bool LookupLOConnectionsToPad(pcb_pad_t *Pad, pcb_cardinal_t LayerGroup)
{
	pcb_cardinal_t entry;
	struct lo_info info;
	int ic;
	pcb_bool retv = pcb_false;

	/* Internal connection: if pads in the same element have the same
	   internal connection group number, they are connected */
	ic = PCB_FLAG_INTCONN_GET(Pad);
	if ((Pad->Element != NULL) && (ic > 0)) {
		pcb_element_t *e = Pad->Element;
		pcb_pad_t *orig_pad = Pad;
		int tlayer = -1;

/*fprintf(stderr, "lg===\n");*/
		for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
			pcb_cardinal_t layer;
			layer = PCB->LayerGroups.Entries[LayerGroup][entry];
/*fprintf(stderr, "lg: %d\n", layer);*/
			if (layer == COMPONENT_LAYER)
				tlayer = COMPONENT_LAYER;
			else if (layer == SOLDER_LAYER)
				tlayer = SOLDER_LAYER;
		}

/*fprintf(stderr, "tlayer=%d\n", tlayer);*/

		if (tlayer >= 0) {
			PCB_PAD_LOOP(e);
			{
				if ((orig_pad != pad) && (ic == PCB_FLAG_INTCONN_GET(pad))) {
					int padlayer = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER;
/*fprintf(stderr, "layergroup1: %d {%d %d %d} %d \n", tlayer, PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad), SOLDER_LAYER, COMPONENT_LAYER, padlayer);*/
					if ((!PCB_FLAG_TEST(TheFlag, pad)) && (tlayer != padlayer)) {
/*fprintf(stderr, "layergroup2\n");*/
						ADD_PAD_TO_LIST(padlayer, pad, PCB_TYPE_PAD, orig_pad, FCT_INTERNAL);
						if (LookupLOConnectionsToPad(pad, LayerGroup))
							retv = pcb_true;
					}
				}
			}
			END_LOOP;
		}
	}


	if (!PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pad))
		return (LookupLOConnectionsToLine((pcb_line_t *) Pad, LayerGroup, pcb_false));

	info.pad = *Pad;
	EXPAND_BOUNDS(&info.pad);
	/* add the new rat lines */
	info.layer = LayerGroup;
	if (setjmp(info.env) == 0)
		r_search(PCB->Data->rat_tree, &info.pad.BoundingBox, NULL, LOCtoPadRat_callback, &info, NULL);
	else
		return pcb_true;

	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		pcb_cardinal_t layer;

		layer = PCB->LayerGroups.Entries[LayerGroup][entry];
		/* handle normal layers */
		if (layer < max_copper_layer) {
			info.layer = layer;
			/* add lines */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, &info.pad.BoundingBox, NULL, LOCtoPadLine_callback, &info, NULL);
			else
				return pcb_true;
			/* add arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, &info.pad.BoundingBox, NULL, LOCtoPadArc_callback, &info, NULL);
			else
				return pcb_true;
			/* add polygons */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->polygon_tree, &info.pad.BoundingBox, NULL, LOCtoPadPoly_callback, &info, NULL);
			else
				return pcb_true;
		}
		else {
			/* handle special 'pad' layers */
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pad_tree, (pcb_box_t *) & info.pad, NULL, LOCtoPadPad_callback, &info, NULL);
			else
				return pcb_true;
		}

	}
	return retv;
}

static pcb_r_dir_t LOCtoPolyLine_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_is_line_in_poly(line, &i->polygon)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_TYPE_POLYGON, &i->polygon, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPolyArc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!PCB_FLAG_TEST(TheFlag, arc) && pcb_is_arc_in_poly(arc, &i->polygon)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_TYPE_POLYGON, &i->polygon, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return 0;
}

static pcb_r_dir_t LOCtoPolyPad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pad) && i->layer == (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
			&& pcb_is_pad_in_poly(pad, &i->polygon)) {
		if (ADD_PAD_TO_LIST(i->layer, pad, PCB_TYPE_POLYGON, &i->polygon, FCT_COPPER))
			longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPolyRat_callback(const pcb_box_t * b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, rat)) {
		if ((rat->Point1.X == (i->polygon.Clipped->contours->head.point[0]) &&
				 rat->Point1.Y == (i->polygon.Clipped->contours->head.point[1]) &&
				 rat->group1 == i->layer) ||
				(rat->Point2.X == (i->polygon.Clipped->contours->head.point[0]) &&
				 rat->Point2.Y == (i->polygon.Clipped->contours->head.point[1]) && rat->group2 == i->layer))
			if (ADD_RAT_TO_LIST(rat, PCB_TYPE_POLYGON, &i->polygon, FCT_RAT))
				longjmp(i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}


/* ---------------------------------------------------------------------------
 * looks up LOs that are connected to the given polygon
 * on the given layergroup. All found connections are added to the list
 */
static pcb_bool LookupLOConnectionsToPolygon(pcb_polygon_t *Polygon, pcb_cardinal_t LayerGroup)
{
	pcb_cardinal_t entry;
	struct lo_info info;

	if (!Polygon->Clipped)
		return pcb_false;
	info.polygon = *Polygon;
	EXPAND_BOUNDS(&info.polygon);
	info.layer = LayerGroup;
	/* check rats */
	if (setjmp(info.env) == 0)
		r_search(PCB->Data->rat_tree, (pcb_box_t *) & info.polygon, NULL, LOCtoPolyRat_callback, &info, NULL);
	else
		return pcb_true;
/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		pcb_cardinal_t layer;

		layer = PCB->LayerGroups.Entries[LayerGroup][entry];

		/* handle normal layers */
		if (layer < max_copper_layer) {
			gdl_iterator_t it;
			pcb_polygon_t *polygon;

			/* check all polygons */
			polylist_foreach(&(PCB->Data->Layer[layer].Polygon), &it, polygon) {
				if (!PCB_FLAG_TEST(TheFlag, polygon)
						&& pcb_is_poly_in_poly(polygon, Polygon)
						&& ADD_POLYGON_TO_LIST(layer, polygon, PCB_TYPE_POLYGON, Polygon, FCT_COPPER))
					return pcb_true;
			}

			info.layer = layer;
			/* check all lines */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, (pcb_box_t *) & info.polygon, NULL, LOCtoPolyLine_callback, &info, NULL);
			else
				return pcb_true;
			/* check all arcs */
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, (pcb_box_t *) & info.polygon, NULL, LOCtoPolyArc_callback, &info, NULL);
			else
				return pcb_true;
		}
		else {
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pad_tree, (pcb_box_t *) & info.polygon, NULL, LOCtoPolyPad_callback, &info, NULL);
			else
				return pcb_true;
		}
	}
	return (pcb_false);
}
