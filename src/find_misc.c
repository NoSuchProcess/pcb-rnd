/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "change.h"

static void DrawNewConnections(void);

/* checks if all lists of new objects are handled */
static pcb_bool ListsEmpty(pcb_bool AndRats)
{
	pcb_bool empty;
	int i;

	empty = (PadstackList.Location >= PadstackList.Number);
	if (AndRats)
		empty = empty && (RatList.Location >= RatList.Number);
	for (i = 0; i < pcb_max_layer && empty; i++)
		empty = empty && LineList[i].Location >= LineList[i].Number
			&& ArcList[i].Location >= ArcList[i].Number && PolygonList[i].Location >= PolygonList[i].Number;
	return empty;
}

static void reassign_no_drc_flags(void)
{
	int layer;
TODO("layer: decide whether it is from attribute or not")
	for (layer = 0; layer < pcb_max_layer; layer++) {
		pcb_layer_t *l = LAYER_PTR(layer);
		l->meta.real.no_drc = pcb_attribute_get(&l->Attributes, "PCB::skip-drc") != NULL;
	}
}


/* loops till no more connections are found */
static pcb_bool DoIt(pcb_bool AndRats, pcb_bool AndDraw)
{
	pcb_bool newone = pcb_false;
	reassign_no_drc_flags();
	do {
		/* lookup connections; these are the steps (2) to (4)
		 * from the description */
		newone =
			LookupLOConnectionsToPSList(AndRats)
			|| LookupLOConnectionsToLOList(AndRats)
			|| LookupPSConnectionsToLOList(AndRats);

		if (AndDraw)
			DrawNewConnections();
	}
	while (!newone && !ListsEmpty(AndRats));
	if (AndDraw)
		pcb_draw();
	return newone;
}

/* draws all new connections which have been found since the routine was called the last time */
static void DrawNewConnections(void)
{
	int i;
	pcb_cardinal_t position;

	/* decrement 'i' to keep layerstack order */
	for (i = pcb_max_layer; i != -1; i--) {
		pcb_cardinal_t layer = pcb_layer_stack[i];

		if (PCB->Data->Layer[layer].meta.real.vis) {
			/* draw all new lines */
			position = LineList[layer].DrawLocation;
			for (; position < LineList[layer].Number; position++)
				pcb_line_invalidate_draw(LAYER_PTR(layer), LINELIST_ENTRY(layer, position));
			LineList[layer].DrawLocation = LineList[layer].Number;

			/* draw all new arcs */
			position = ArcList[layer].DrawLocation;
			for (; position < ArcList[layer].Number; position++)
				pcb_arc_invalidate_draw(LAYER_PTR(layer), ARCLIST_ENTRY(layer, position));
			ArcList[layer].DrawLocation = ArcList[layer].Number;

			/* draw all new polygons */
			position = PolygonList[layer].DrawLocation;
			for (; position < PolygonList[layer].Number; position++)
				pcb_poly_invalidate_draw(LAYER_PTR(layer), POLYGONLIST_ENTRY(layer, position));
			PolygonList[layer].DrawLocation = PolygonList[layer].Number;
		}
	}

	/* draw all new Padstacks; 'PadstackList' holds a list of pointers to the
	 * sorted array pointers to padstack data
	 */
	while (PadstackList.DrawLocation < PadstackList.Number) {
		pcb_pstk_t *ps = PADSTACKLIST_ENTRY(PadstackList.DrawLocation);

		if (PCB_FLAG_TEST(PCB_FLAG_TERMNAME, ps)) {
			if (PCB->pstk_on)
				pcb_pstk_invalidate_draw(ps);
		}
		else if (PCB->pstk_on)
			pcb_pstk_invalidate_draw(ps);
		PadstackList.DrawLocation++;
	}

	/* draw the new rat-lines */
	if (PCB->RatOn) {
		position = RatList.DrawLocation;
		for (; position < RatList.Number; position++)
			pcb_rat_invalidate_draw(RATLIST_ENTRY(position));
		RatList.DrawLocation = RatList.Number;
	}
}

/*---------------------------------------------------------------------------
 * add the starting object to the list of found objects
 */
static pcb_bool ListStart(pcb_any_obj_t *obj)
{
	DumpList();
	switch (obj->type) {

	case PCB_OBJ_PSTK:
		{
			if (ADD_PADSTACK_TO_LIST((pcb_pstk_t *)obj, 0, NULL, PCB_FCT_START, NULL))
				return pcb_true;
			break;
		}

	case PCB_OBJ_RAT:
		{
			if (ADD_RAT_TO_LIST((pcb_rat_t *)obj, 0, NULL, PCB_FCT_START, NULL))
				return pcb_true;
			break;
		}

	case PCB_OBJ_LINE:
		{
			pcb_layer_id_t layer = pcb_layer_id(PCB->Data, obj->parent.layer);
			if (ADD_LINE_TO_LIST(layer, (pcb_line_t *)obj, 0, NULL, PCB_FCT_START, NULL))
				return pcb_true;
			break;
		}

	case PCB_OBJ_ARC:
		{
			pcb_layer_id_t layer = pcb_layer_id(PCB->Data, obj->parent.layer);
			if (ADD_ARC_TO_LIST(layer, (pcb_arc_t *)obj, 0, NULL, PCB_FCT_START, NULL))
				return pcb_true;
			break;
		}

	case PCB_OBJ_POLY:
		{
			pcb_layer_id_t layer = pcb_layer_id(PCB->Data, obj->parent.layer);
			if (ADD_POLYGON_TO_LIST(layer, (pcb_poly_t *)obj, 0, NULL, PCB_FCT_START, NULL))
				return pcb_true;
			break;
		}

	default:
		assert(!"unhandled object type: can't start a find list from this\n");
	}
	return pcb_false;
}


/* looks up all connections from the object at the given coordinates
 * the TheFlag (normally 'PCB_FLAG_FOUND') is set for all objects found
 * the objects are re-drawn if AndDraw is pcb_true
 * also the action is marked as undoable if AndDraw is pcb_true */
void pcb_lookup_conn(pcb_coord_t X, pcb_coord_t Y, pcb_bool AndDraw, pcb_coord_t Range, int which_flag)
{
	void *ptr1, *ptr2, *ptr3;
	char *name;
	int type;

	/* check if there are any pins or pads at that position */

	reassign_no_drc_flags();

	type = pcb_search_obj_by_location(PCB_LOOKUP_FIRST, &ptr1, &ptr2, &ptr3, X, Y, Range);
	if (type == PCB_OBJ_VOID) {
		type = pcb_search_obj_by_location(PCB_LOOKUP_MORE, &ptr1, &ptr2, &ptr3, X, Y, Range);
		if (type == PCB_OBJ_VOID)
			return;
		if (type & PCB_SILK_TYPE) {
			/* don't mess with non-conducting objects! */
			if (!(pcb_layer_flags_((pcb_layer_t *)ptr1) & PCB_LYT_COPPER) || ((pcb_layer_t *) ptr1)->meta.real.no_drc)
				return;
		}
	}
	else {
		name = pcb_connection_name(ptr2);
		if (name != NULL)
			pcb_actionl("NetlistShow", name, NULL);
	}

	TheFlag = which_flag;
	User = AndDraw;
	pcb_conn_lookup_init();

	/* now add the object to the appropriate list and start scanning
	 * This is step (1) from the description
	 */
	ListStart(ptr2);
	DoIt(pcb_true, AndDraw);
	if (User)
		pcb_undo_inc_serial();
	User = pcb_false;

	/* we are done */
	if (AndDraw)
		pcb_draw();
	if (AndDraw && conf_core.editor.beep_when_finished)
		pcb_gui->beep();
	pcb_conn_lookup_uninit();
}

void pcb_lookup_conn_by_pin(int type, void *ptr1)
{
	User = 0;
	pcb_conn_lookup_init();
	ListStart(ptr1);

	DoIt(pcb_true, pcb_false);

	pcb_conn_lookup_uninit();
}

TODO("cleanup: keep only PCB_OBJ_* and remove this function")
unsigned long pcb_obj_type2oldtype(pcb_objtype_t type)
{
	switch(type) {
		case PCB_OBJ_LINE:    return PCB_OBJ_LINE;
		case PCB_OBJ_TEXT:    return PCB_OBJ_TEXT;
		case PCB_OBJ_POLY:    return PCB_OBJ_POLY;
		case PCB_OBJ_ARC:     return PCB_OBJ_ARC;
		case PCB_OBJ_RAT:     return PCB_OBJ_RAT;
		case PCB_OBJ_PSTK:    return PCB_OBJ_PSTK;
		case PCB_OBJ_SUBC:    return PCB_OBJ_SUBC;

		default: return 0;
	}
	return 0;
}

/* find connections for rats nesting assumes pcb_conn_lookup_init() has already been done */
void pcb_rat_find_hook(pcb_any_obj_t *obj, pcb_bool undo, pcb_bool AndRats)
{
	User = undo;
	DumpList();
	ListStart(obj);
	DoIt(AndRats, pcb_false);
	User = pcb_false;
}

static pcb_bool pcb_reset_found_subc(pcb_subc_t *subc, pcb_bool AndDraw);

/* resets all used flags of pins and vias */
static pcb_bool pcb_reset_found_pins_vias_pads_(pcb_data_t *data, pcb_bool AndDraw)
{
	pcb_bool change = pcb_false;

	PCB_PADSTACK_LOOP(data);
	{
		if (PCB_FLAG_TEST(TheFlag, padstack)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(padstack);
			PCB_FLAG_CLEAR(TheFlag, padstack);
			if (AndDraw)
				pcb_pstk_invalidate_draw(padstack);
			change = pcb_true;
		}
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(data);
	{
		if (pcb_reset_found_subc(subc, AndDraw))
			change = pcb_true;
	}
	PCB_END_LOOP;

	return change;
}

pcb_bool pcb_reset_found_pins_vias_pads(pcb_bool AndDraw)
{
	pcb_bool change = pcb_reset_found_pins_vias_pads_(PCB->Data, AndDraw);

	if (change)
		pcb_board_set_changed_flag(pcb_true);

	return change;
}

/* resets all used flags of LOs */
static pcb_bool pcb_reset_found_lines_polys_(pcb_data_t *data, pcb_bool AndDraw)
{
	pcb_bool change = pcb_false;

	PCB_RAT_LOOP(data);
	{
		if (PCB_FLAG_TEST(TheFlag, line)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(line);
			PCB_FLAG_CLEAR(TheFlag, line);
			if (AndDraw)
				pcb_rat_invalidate_draw(line);
			change = pcb_true;
		}
	}
	PCB_END_LOOP;
	PCB_LINE_COPPER_LOOP(data);
	{
		if (PCB_FLAG_TEST(TheFlag, line)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(line);
			PCB_FLAG_CLEAR(TheFlag, line);
			if (AndDraw)
				pcb_line_invalidate_draw(layer, line);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_COPPER_LOOP(data);
	{
		if (PCB_FLAG_TEST(TheFlag, arc)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(arc);
			PCB_FLAG_CLEAR(TheFlag, arc);
			if (AndDraw)
				pcb_arc_invalidate_draw(layer, arc);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_COPPER_LOOP(data);
	{
		if (PCB_FLAG_TEST(TheFlag, polygon)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(polygon);
			PCB_FLAG_CLEAR(TheFlag, polygon);
			if (AndDraw)
				pcb_poly_invalidate_draw(layer, polygon);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;


	PCB_SUBC_LOOP(data);
	{
		if (pcb_reset_found_subc(subc, AndDraw))
			change = pcb_true;
	}
	PCB_END_LOOP;

	return change;
}

static pcb_bool pcb_reset_found_subc(pcb_subc_t *subc, pcb_bool AndDraw)
{
	pcb_bool res = pcb_false;

	if (pcb_reset_found_lines_polys_(subc->data, AndDraw))
		res = pcb_true;

	if (pcb_reset_found_pins_vias_pads_(subc->data, AndDraw))
		res = pcb_true;

	return res;
}

pcb_bool pcb_reset_found_lines_polys(pcb_bool AndDraw)
{
	pcb_bool change = pcb_reset_found_lines_polys_(PCB->Data, AndDraw);
	if (change)
		pcb_board_set_changed_flag(pcb_true);
	return change;

}


/* resets all found connections */
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

	PadstackList.Number = 0;
	PadstackList.Location = 0;

	for (i = 0; i < pcb_max_layer; i++) {
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
	pcb_layout_lookup_init();
}

void pcb_conn_lookup_uninit(void)
{
	pcb_layout_lookup_uninit();
}
