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

#include "compat_nls.h"
#include "board.h"
#include "obj_subc_parent.h"

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

	return pcb_r_search(rtree, &box, region_in_search, rectangle_in_region, closure, num_found);
}


/* Connection lookup functions */

static pcb_bool ADD_PS_TO_LIST(pcb_pstk_t *ps, int from_type, void *from_ptr, pcb_found_conn_type_t type, void *orig_from)
{
	if (User)
		pcb_undo_add_obj_to_flag(ps);
	PCB_FLAG_SET(TheFlag, ps);
	make_callback(PCB_OBJ_PSTK, ps, from_type, from_ptr, type);
	PADSTACKLIST_ENTRY(PadstackList.Number) = ps;
	PadstackList.Number++;
#ifdef DEBUG
	if (PadstackList.Number > PadstackList.Size)
		printf("ADD_PS_TO_LIST overflow! num=%d size=%d\n", PadstackList.Number, PadstackList.Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps))
		return SetThing(ps, orig_from);
	return pcb_false;
}

static pcb_bool ADD_PADSTACK_TO_LIST(pcb_pstk_t *ps, int from_type, void *from_ptr, pcb_found_conn_type_t type, void *orig_from)
{
	if (User)
		pcb_undo_add_obj_to_flag(ps);
	PCB_FLAG_SET(TheFlag, ps);
	make_callback(PCB_OBJ_PSTK, ps, from_type, from_ptr, type);
	PADSTACKLIST_ENTRY(PadstackList.Number) = ps;
	PadstackList.Number++;
#ifdef DEBUG
	if (PadstackList.Number > PadstackList.Size)
		printf("ADD_PADSTACK_TO_LIST overflow! num=%d size=%d\n", PVList.Number, PVList.Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps) && (ps->parent.data->parent_type == PCB_PARENT_SUBC))
		return SetThing(ps, orig_from);
	return pcb_false;
}

static pcb_bool ADD_LINE_TO_LIST(pcb_layer_id_t L, pcb_line_t *Ptr, int from_type, void *from_ptr, pcb_found_conn_type_t type, void *orig_from)
{
	if (LineList[L].Data == NULL)
		return pcb_false;
	if (User)
		pcb_undo_add_obj_to_flag(Ptr);
	PCB_FLAG_SET(TheFlag, (Ptr));
	make_callback(PCB_OBJ_LINE, Ptr, from_type, from_ptr, type);
	LINELIST_ENTRY((L), LineList[(L)].Number) = (Ptr);
	LineList[(L)].Number++;
#ifdef DEBUG
	if (LineList[(L)].Number > LineList[(L)].Size)
		printf("ADD_LINE_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, LineList[(L)].Number, LineList[(L)].Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, (Ptr)))
		return SetThing(Ptr, orig_from);
	return pcb_false;
}

static pcb_bool ADD_ARC_TO_LIST(pcb_cardinal_t L, pcb_arc_t *Ptr, int from_type, void *from_ptr, pcb_found_conn_type_t type, void *orig_from)
{
	if (ArcList[L].Data == NULL)
		return pcb_false;

	if (User)
		pcb_undo_add_obj_to_flag(Ptr);
	PCB_FLAG_SET(TheFlag, (Ptr));
	make_callback(PCB_OBJ_ARC, Ptr, from_type, from_ptr, type);
	ARCLIST_ENTRY((L), ArcList[(L)].Number) = (Ptr);
	ArcList[(L)].Number++;
#ifdef DEBUG
	if (ArcList[(L)].Number > ArcList[(L)].Size)
		printf("ADD_ARC_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, ArcList[(L)].Number, ArcList[(L)].Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, (Ptr)))
		return SetThing(Ptr, orig_from);
	return pcb_false;
}

static pcb_bool ADD_RAT_TO_LIST(pcb_rat_t *Ptr, int from_type, void *from_ptr, pcb_found_conn_type_t type, void *orig_from)
{
	if (User)
		pcb_undo_add_obj_to_flag(Ptr);
	PCB_FLAG_SET(TheFlag, (Ptr));
	make_callback(PCB_OBJ_RAT, Ptr, from_type, from_ptr, type);
	RATLIST_ENTRY(RatList.Number) = (Ptr);
	RatList.Number++;
#ifdef DEBUG
	if (RatList.Number > RatList.Size)
		printf("ADD_RAT_TO_LIST overflow! num=%d size=%d\n", RatList.Number, RatList.Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, (Ptr)))
		return SetThing(Ptr, orig_from);
	return pcb_false;
}

static pcb_bool ADD_POLYGON_TO_LIST(pcb_cardinal_t L, pcb_poly_t *Ptr, int from_type, void *from_ptr, pcb_found_conn_type_t type, void *orig_from)
{
	if (PolygonList[L].Data == NULL)
		return pcb_false;

	if (User)
		pcb_undo_add_obj_to_flag(Ptr);
	PCB_FLAG_SET(TheFlag, (Ptr));
	make_callback(PCB_OBJ_POLY, Ptr, from_type, from_ptr, type);
	POLYGONLIST_ENTRY((L), PolygonList[(L)].Number) = (Ptr);
	PolygonList[(L)].Number++;
#ifdef DEBUG
	if (PolygonList[(L)].Number > PolygonList[(L)].Size)
		printf("ADD_ARC_TO_LIST overflow! lay=%d, num=%d size=%d\n", L, PolygonList[(L)].Number, PolygonList[(L)].Size);
#endif
	if (drc && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, (Ptr)))
		return SetThing(Ptr, orig_from);
	return pcb_false;
}

pcb_bool SetThing(void *obj1, void *obj2)
{
	pcb_found_obj1 = obj1;
	pcb_found_obj2 = obj2;
	return pcb_true;
}

#include "find_intconn.c"

void pcb_layout_lookup_uninit(void)
{
	pcb_cardinal_t i;

	for (i = 0; i < pcb_max_layer; i++) {
		free(LineList[i].Data);
		LineList[i].Data = NULL;
		free(ArcList[i].Data);
		ArcList[i].Data = NULL;
		free(PolygonList[i].Data);
		PolygonList[i].Data = NULL;
	}
	free(RatList.Data);
	free(PadstackList.Data);
	RatList.Data = NULL;
	PadstackList.Data = NULL;
}

/* allocates memory for component related stacks ...
 * initializes index and sorts it by X1 and X2 */
void pcb_layout_lookup_init(void)
{
	pcb_layer_id_t i;

	/* initialize line arc and polygon data */
	for(i = 0; i < pcb_max_layer; i++) {
		pcb_layer_t *layer = LAYER_PTR(i);

		if (pcb_layer_flags(PCB, i) & PCB_LYT_COPPER) {

		if ((layer->line_tree != NULL) && (layer->line_tree->size > 0)) {
			LineList[i].Size = layer->line_tree->size;
			LineList[i].Data = (void **) calloc(LineList[i].Size, sizeof(pcb_line_t *));
		}
		if ((layer->arc_tree != NULL) && (layer->arc_tree->size > 0)) {
			ArcList[i].Size = layer->arc_tree->size;
			ArcList[i].Data = (void **) calloc(ArcList[i].Size, sizeof(pcb_arc_t *));
		}
		if ((layer->polygon_tree != NULL) && (layer->polygon_tree->size > 0)) {
			PolygonList[i].Size = layer->polygon_tree->size;
			PolygonList[i].Data = (void **) calloc(PolygonList[i].Size, sizeof(pcb_poly_t *));
		}

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

	if (PCB->Data->padstack_tree)
		TotalPs = PCB->Data->padstack_tree->size;
	else
		TotalPs = 0;
	/* allocate memory for 'new padstack to check' list and clear struct */
	PadstackList.Data = (void **) calloc(TotalPs, sizeof(pcb_pstk_t *));
	PadstackList.Size = TotalPs;
	PadstackList.Location = 0;
	PadstackList.DrawLocation = 0;
	PadstackList.Number = 0;
	/* Initialize ratline data */
	RatList.Size = ratlist_length(&PCB->Data->Rat);
	RatList.Data = (void **) calloc(RatList.Size, sizeof(pcb_rat_t *));
	RatList.Location = 0;
	RatList.DrawLocation = 0;
	RatList.Number = 0;
}

struct lo_info {
	pcb_layer_id_t layer;
	pcb_line_t line;
	pcb_arc_t arc;
	pcb_poly_t polygon;
	pcb_rat_t rat;

	pcb_layer_id_t *orig_layer;
	pcb_line_t *orig_line;
	pcb_arc_t *orig_arc;
	pcb_poly_t *orig_polygon;
	pcb_rat_t *orig_rat;

	jmp_buf env;
};

struct ps_info {
	pcb_layer_id_t layer;
	pcb_pstk_t ps;
	pcb_pstk_t *orig_ps;
	jmp_buf env;
};

static pcb_r_dir_t LOCtoPSline_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct ps_info *i = (struct ps_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_pstk_intersect_line(&i->ps, line) && !INOCN(&i->ps, line)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_OBJ_PSTK, &i->ps, PCB_FCT_COPPER, i->orig_ps))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPSarc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct ps_info *i = (struct ps_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, arc) && pcb_pstk_intersect_arc(&i->ps, arc) && !INOCN(&i->ps, arc)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_OBJ_PSTK, &i->ps, PCB_FCT_COPPER, i->orig_ps))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPSrat_callback(const pcb_box_t * b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) b;
	struct ps_info *i = (struct ps_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, rat) && pcb_pstk_intersect_rat(&i->ps, rat) && ADD_RAT_TO_LIST(rat, PCB_OBJ_PSTK, &i->ps, PCB_FCT_RAT, i->orig_ps))
		longjmp(i->env, 1);
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPSpoly_callback(const pcb_box_t * b, void *cl)
{
	pcb_poly_t *polygon = (pcb_poly_t *) b;
	struct ps_info *i = (struct ps_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, polygon) && pcb_pstk_intersect_poly(&i->ps, polygon) && !INOCN(&i->ps, polygon)) {
		if (ADD_POLYGON_TO_LIST(i->layer, polygon, PCB_OBJ_PSTK, &i->ps, PCB_FCT_COPPER, i->orig_ps))
			longjmp(i->env, 1);
	}

	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t PStoPS_callback(const pcb_box_t *b, void *cl)
{
	pcb_pstk_t *ps2 = (pcb_pstk_t *)b;
	struct ps_info *i = (struct ps_info *)cl;

	if (!PCB_FLAG_TEST(TheFlag, ps2) && pcb_pstk_intersect_pstk(&i->ps, ps2) && !INOCN(&i->ps, ps2)) {
		if (ADD_PADSTACK_TO_LIST(ps2, PCB_OBJ_PSTK, &i->ps, PCB_FCT_COPPER, i->orig_ps))
			longjmp(i->env, 1);
	}

	return PCB_R_DIR_NOT_FOUND;
}

/* checks if a padstack is connected to LOs, if it is, the LO is added to
 * the appropriate list and the 'used' flag is set */
static pcb_bool LookupLOConnectionsToPSList(pcb_bool AndRats)
{
	pcb_cardinal_t layer;
	struct ps_info info;
	pcb_pstk_t *orig_ps;

	/* loop over all PVs currently on list */
	while (PadstackList.Location < PadstackList.Number) {
		/* get pointer to data */
		orig_ps = (PADSTACKLIST_ENTRY(PadstackList.Location));
		info.ps = *orig_ps;
		info.orig_ps = orig_ps;
		EXPAND_BOUNDS(&info.ps);

		/* subc intconn jumps */
		if ((orig_ps->term != NULL) && (orig_ps->intconn > 0))
			LOC_int_conn_subc(pcb_gobj_parent_subc(orig_ps->parent_type, &orig_ps->parent), orig_ps->intconn, PCB_OBJ_PSTK, orig_ps);

		/* now all lines, arcs and polygons of the several layers */
		for(layer = 0; layer < pcb_max_layer; layer++) {
			if (!(pcb_layer_flags(PCB, layer) & PCB_LYT_COPPER))
				continue;
			if (LAYER_PTR(layer)->meta.real.no_drc)
				continue;
			info.layer = layer;
			/* add touching lines */
			if (setjmp(info.env) == 0)
				pcb_r_search(LAYER_PTR(layer)->line_tree, (pcb_box_t *) & info.ps, NULL, LOCtoPSline_callback, &info, NULL);
			else
				return pcb_true;
			/* add touching arcs */
			if (setjmp(info.env) == 0)
				pcb_r_search(LAYER_PTR(layer)->arc_tree, (pcb_box_t *) & info.ps, NULL, LOCtoPSarc_callback, &info, NULL);
			else
				return pcb_true;
			/* check all polygons */
			if (setjmp(info.env) == 0)
				pcb_r_search(LAYER_PTR(layer)->polygon_tree, (pcb_box_t *) & info.ps, NULL, LOCtoPSpoly_callback, &info, NULL);
			else
				return pcb_true;
		}

		/* Check for connections to other padstacks */
		if (setjmp(info.env) == 0)
			pcb_r_search(PCB->Data->padstack_tree, (pcb_box_t *) & info.ps, NULL, PStoPS_callback, &info, NULL);
		else
			return pcb_true;

		/* Check for rat-lines that may intersect the PV */
		if (AndRats) {
			if (setjmp(info.env) == 0)
				pcb_r_search(PCB->Data->rat_tree, (pcb_box_t *) & info.ps, NULL, LOCtoPSrat_callback, &info, NULL);
			else
				return pcb_true;
		}
		PadstackList.Location++;
	}
	return pcb_false;
}

/* find all connections between LO at the current list position and new LOs */
static pcb_bool LookupLOConnectionsToLOList(pcb_bool AndRats)
{
	pcb_bool done;
	pcb_layer_id_t layer;
	pcb_layergrp_id_t group;
	pcb_cardinal_t i, ratposition, lineposition[PCB_MAX_LAYER], polyposition[PCB_MAX_LAYER], arcposition[PCB_MAX_LAYER];

	/* copy the current LO list positions; the original data is changed
	 * by 'LookupPVPSConnectionsToLOList()' which has to check the same
	 * list entries plus the new ones
	 */
	for (i = 0; i < pcb_max_layer; i++) {
		lineposition[i] = LineList[i].Location;
		polyposition[i] = PolygonList[i].Location;
		arcposition[i] = ArcList[i].Location;
	}
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
					return pcb_true;
				group = RATLIST_ENTRY(*position)->group2;
				if (LookupLOConnectionsToRatEnd(&(RATLIST_ENTRY(*position)->Point2), group))
					return pcb_true;
			}
		}
		/* loop over all layergroups */
		for (group = 0; group < PCB->LayerGroups.len; group++) {
			pcb_cardinal_t entry;

			for (entry = 0; entry < PCB->LayerGroups.grp[group].len; entry++) {
				layer = PCB->LayerGroups.grp[group].lid[entry];

				/* be aware that the layer number equal pcb_max_layer
				 * and pcb_max_layer+1 have a special meaning for pads
				 */
				/* try all new lines */
				position = &lineposition[layer];
				for (; *position < LineList[layer].Number; (*position)++)
					if (LookupLOConnectionsToLine(LINELIST_ENTRY(layer, *position), group, pcb_true))
						return pcb_true;

				/* try all new arcs */
				position = &arcposition[layer];
				for (; *position < ArcList[layer].Number; (*position)++)
					if (LookupLOConnectionsToArc(ARCLIST_ENTRY(layer, *position), group))
						return pcb_true;

				/* try all new polygons */
				position = &polyposition[layer];
				for (; *position < PolygonList[layer].Number; (*position)++)
					if (LookupLOConnectionsToPolygon(POLYGONLIST_ENTRY(layer, *position), group))
						return pcb_true;
			}
		}

		/* check if all lists are done; Later for-loops
		 * may have changed the prior lists
		 */
		done = !AndRats || ratposition >= RatList.Number;
		for (layer = 0; layer < pcb_max_layer; layer++) {
			done = done &&
				lineposition[layer] >= LineList[layer].Number
				&& arcposition[layer] >= ArcList[layer].Number && polyposition[layer] >= PolygonList[layer].Number;
		}
	} while (!done);
	return pcb_false;
}

static pcb_r_dir_t ps_line_callback(const pcb_box_t * b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, ps) && pcb_pstk_intersect_line(ps, &i->line) && !INOCN(ps, &i->line)) {
		if (ADD_PS_TO_LIST(ps, PCB_OBJ_LINE, &i->line, PCB_FCT_COPPER, i->orig_line))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t ps_arc_callback(const pcb_box_t * b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, ps) && pcb_pstk_intersect_arc(ps, &i->arc) && !INOCN(ps, &i->arc)) {
		if (ADD_PS_TO_LIST(ps, PCB_OBJ_ARC, &i->arc, PCB_FCT_COPPER, i->orig_arc))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t ps_poly_callback(const pcb_box_t * b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, ps) && pcb_pstk_intersect_poly(ps, &i->polygon) && !INOCN(ps, &i->polygon)) {
		if (ADD_PS_TO_LIST(ps, PCB_OBJ_POLY, &i->polygon, PCB_FCT_COPPER, i->orig_polygon))
			longjmp(i->env, 1);
	}

	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t ps_rat_callback(const pcb_box_t * b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	/* rats can't cause DRC so there is no early exit */
	if (!PCB_FLAG_TEST(TheFlag, ps) && pcb_pstk_intersect_rat(ps, &i->rat))
		ADD_PS_TO_LIST(ps, PCB_OBJ_RAT, &i->rat, PCB_FCT_RAT, i->orig_rat);
	return PCB_R_DIR_NOT_FOUND;
}

/* searches for new PVs and padstacks that are connected to NEW LOs on the list
 * This routine updates the position counter of the lists too. */
static pcb_bool LookupPSConnectionsToLOList(pcb_bool AndRats)
{
	pcb_layer_id_t layer;
	struct lo_info info;

	/* loop over all layers */
	for(layer = 0; layer < pcb_max_layer; layer++) {
		if (!(pcb_layer_flags(PCB, layer) & PCB_LYT_COPPER))
			continue;
		if (LAYER_PTR(layer)->meta.real.no_drc)
			continue;
		/* do nothing if there are no PV's */
		if (TotalPs == 0) {
			LineList[layer].Location = LineList[layer].Number;
			ArcList[layer].Location = ArcList[layer].Number;
			PolygonList[layer].Location = PolygonList[layer].Number;
			continue;
		}

		/* check all lines */
		while (LineList[layer].Location < LineList[layer].Number) {
			pcb_line_t *orig_line = (LINELIST_ENTRY(layer, LineList[layer].Location));
			info.line = *orig_line;
			info.orig_line = orig_line;
			EXPAND_BOUNDS(&info.line);

			/* subc intconn jumps */
			if ((orig_line->term != NULL) && (orig_line->intconn > 0))
				LOC_int_conn_subc(pcb_lobj_parent_subc(orig_line->parent_type, &orig_line->parent), orig_line->intconn, PCB_OBJ_LINE, orig_line);

			if (setjmp(info.env) == 0)
				pcb_r_search(PCB->Data->padstack_tree, (pcb_box_t *) & info.line, NULL, ps_line_callback, &info, NULL);
			else
				return pcb_true;
			LineList[layer].Location++;
		}

		/* check all arcs */
		while (ArcList[layer].Location < ArcList[layer].Number) {
			pcb_arc_t *orig_arc = (ARCLIST_ENTRY(layer, ArcList[layer].Location));
			info.arc = *orig_arc;
			info.orig_arc = orig_arc;
			EXPAND_BOUNDS(&info.arc);

			/* subc intconn jumps */
			if ((orig_arc->term != NULL) && (orig_arc->intconn > 0))
				LOC_int_conn_subc(pcb_lobj_parent_subc(orig_arc->parent_type, &orig_arc->parent), orig_arc->intconn, PCB_OBJ_LINE, orig_arc);

			if (setjmp(info.env) == 0)
				pcb_r_search(PCB->Data->padstack_tree, (pcb_box_t *) & info.arc, NULL, ps_arc_callback, &info, NULL);
			else
				return pcb_true;
			ArcList[layer].Location++;
		}

		/* now all polygons */
		info.layer = layer;
		while (PolygonList[layer].Location < PolygonList[layer].Number) {
			pcb_poly_t *orig_poly = (POLYGONLIST_ENTRY(layer, PolygonList[layer].Location));
			info.polygon = *orig_poly;
			info.orig_polygon = orig_poly;
			EXPAND_BOUNDS(&info.polygon);

			/* subc intconn jumps */
			if ((orig_poly->term != NULL) && (orig_poly->intconn > 0))
				LOC_int_conn_subc(pcb_lobj_parent_subc(orig_poly->parent_type, &orig_poly->parent), orig_poly->intconn, PCB_OBJ_LINE, orig_poly);

			if (setjmp(info.env) == 0)
				pcb_r_search(PCB->Data->padstack_tree, (pcb_box_t *) & info.polygon, NULL, ps_poly_callback, &info, NULL);
			else
				return pcb_true;
			PolygonList[layer].Location++;
		}
	}

	/* do nothing if there are no PS's */
	if (TotalPs == 0)
		RatList.Location = RatList.Number;

	/* check all rat-lines */
	if (AndRats) {
		while (RatList.Location < RatList.Number) {
			info.rat = *(RATLIST_ENTRY(RatList.Location));
			info.orig_rat = (RATLIST_ENTRY(RatList.Location));
			r_search_pt(PCB->Data->padstack_tree, &info.rat.Point1, 1, NULL, ps_rat_callback, &info, NULL);
			r_search_pt(PCB->Data->padstack_tree, &info.rat.Point2, 1, NULL, ps_rat_callback, &info, NULL);

			RatList.Location++;
		}
	}
	return pcb_false;
}

static pcb_r_dir_t LOCtoArcLine_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_isc_line_arc(line, &i->arc) && !INOCN(line, &i->arc)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_OBJ_ARC, &i->arc, PCB_FCT_COPPER, i->orig_arc))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoArcArc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!PCB_FLAG_TEST(TheFlag, arc) && pcb_isc_arc_arc(&i->arc, arc) && !INOCN(&i->arc, arc)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_OBJ_ARC, &i->arc, PCB_FCT_COPPER, i->orig_arc))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoArcRat_callback(const pcb_box_t *b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *)b;
	struct lo_info *i = (struct lo_info *)cl;

	if (!PCB_FLAG_TEST(TheFlag, rat)) {
		if ((rat->group1 == i->layer)
				&& pcb_isc_rat_arc(&rat->Point1, &i->arc)) {
			if (ADD_RAT_TO_LIST(rat, PCB_OBJ_ARC, &i->arc, PCB_FCT_RAT, i->orig_arc))
				longjmp(i->env, 1);
		}
		else if ((rat->group2 == i->layer)
						 && pcb_isc_rat_arc(&rat->Point2, &i->arc)) {
			if (ADD_RAT_TO_LIST(rat, PCB_OBJ_ARC, &i->arc, PCB_FCT_RAT, i->orig_arc))
				longjmp(i->env, 1);
		}
	}
	return PCB_R_DIR_NOT_FOUND;
}

/* searches all LOs that are connected to the given arc on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at arc i */
static pcb_bool LookupLOConnectionsToArc(pcb_arc_t *Arc, pcb_cardinal_t LayerGroup)
{
	pcb_cardinal_t entry;
	struct lo_info info;

	info.arc = *Arc;
	info.orig_arc = Arc;
	EXPAND_BOUNDS(&info.arc);


	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.grp[LayerGroup].len; entry++) {
		pcb_layer_id_t layer;

		layer = PCB->LayerGroups.grp[LayerGroup].lid[entry];

		info.layer = LayerGroup;

		/* add the new rat lines */
		if (setjmp(info.env) == 0)
			pcb_r_search(PCB->Data->rat_tree, &info.arc.BoundingBox, NULL, LOCtoArcRat_callback, &info, NULL);
		else
			return pcb_true;

		info.layer = layer;

		/* add arcs */
		if (setjmp(info.env) == 0)
			pcb_r_search(LAYER_PTR(layer)->line_tree, &info.arc.BoundingBox, NULL, LOCtoArcLine_callback, &info, NULL);
		else
			return pcb_true;

		if (setjmp(info.env) == 0)
			pcb_r_search(LAYER_PTR(layer)->arc_tree, &info.arc.BoundingBox, NULL, LOCtoArcArc_callback, &info, NULL);
		else
			return pcb_true;

		/* now check all polygons */
		{
			pcb_rtree_it_t it;
			pcb_box_t *b;
			for(b = pcb_r_first(PCB->Data->Layer[layer].polygon_tree, &it); b != NULL; b = pcb_r_next(&it)) {
				pcb_poly_t *polygon = (pcb_poly_t *)b;
				if (!PCB_FLAG_TEST(TheFlag, polygon) && pcb_isc_arc_poly(Arc, polygon)
						&& ADD_POLYGON_TO_LIST(layer, polygon, PCB_OBJ_ARC, Arc, PCB_FCT_COPPER, Arc))
					return pcb_true;
			}
			pcb_r_end(&it);
		}
	}

	return pcb_false;
}

static pcb_r_dir_t LOCtoLineLine_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_isc_line_line(&i->line, line) && !INOCN(&i->line, line)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_OBJ_LINE, &i->line, PCB_FCT_COPPER, i->orig_line))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoLineArc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!PCB_FLAG_TEST(TheFlag, arc) && pcb_isc_line_arc(&i->line, arc) && !INOCN(&i->line, arc)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_OBJ_LINE, &i->line, PCB_FCT_COPPER, i->orig_line))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoLineRat_callback(const pcb_box_t * b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, rat)) {
		if ((rat->group1 == i->layer)
				&& pcb_isc_rat_line(&rat->Point1, &i->line)) {
			if (ADD_RAT_TO_LIST(rat, PCB_OBJ_LINE, &i->line, PCB_FCT_RAT, i->orig_line))
				longjmp(i->env, 1);
		}
		else if ((rat->group2 == i->layer)
						 && pcb_isc_rat_line(&rat->Point2, &i->line)) {
			if (ADD_RAT_TO_LIST(rat, PCB_OBJ_LINE, &i->line, PCB_FCT_RAT, i->orig_line))
				longjmp(i->env, 1);
		}
	}
	return PCB_R_DIR_NOT_FOUND;
}

/* searches all LOs that are connected to the given line on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at line i */
static pcb_bool LookupLOConnectionsToLine(pcb_line_t *Line, pcb_cardinal_t LayerGroup, pcb_bool PolysTo)
{
	pcb_cardinal_t entry;
	struct lo_info info;

	info.line = *Line;
	info.orig_line = Line;
	info.layer = LayerGroup;
	EXPAND_BOUNDS(&info.line)
		/* add the new rat lines */
	if (setjmp(info.env) == 0)
		pcb_r_search(PCB->Data->rat_tree, &info.line.BoundingBox, NULL, LOCtoLineRat_callback, &info, NULL);
	else
		return pcb_true;

	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.grp[LayerGroup].len; entry++) {
		pcb_layer_id_t layer;

		layer = PCB->LayerGroups.grp[LayerGroup].lid[entry];

		info.layer = layer;
		/* add lines */
		if (setjmp(info.env) == 0)
			pcb_r_search(LAYER_PTR(layer)->line_tree, (pcb_box_t *) & info.line, NULL, LOCtoLineLine_callback, &info, NULL);
		else
			return pcb_true;
		/* add arcs */
		if (setjmp(info.env) == 0)
			pcb_r_search(LAYER_PTR(layer)->arc_tree, (pcb_box_t *) & info.line, NULL, LOCtoLineArc_callback, &info, NULL);
		else
			return pcb_true;
		/* now check all polygons */
		if (PolysTo) {
			pcb_rtree_it_t it;
			pcb_box_t *b;
			for(b = pcb_r_first(PCB->Data->Layer[layer].polygon_tree, &it); b != NULL; b = pcb_r_next(&it)) {
				pcb_poly_t *polygon = (pcb_poly_t *)b;
				if (!PCB_FLAG_TEST(TheFlag, polygon) && pcb_is_line_in_poly(Line, polygon)
						&& ADD_POLYGON_TO_LIST(layer, polygon, PCB_OBJ_LINE, Line, PCB_FCT_COPPER, Line))
					return pcb_true;
			}
			pcb_r_end(&it);
		}
	}

	return pcb_false;
}


struct rat_info {
	pcb_cardinal_t layer;
	pcb_point_t *Point;
	jmp_buf env;
};

static pcb_r_dir_t LOCtoRatLine_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct rat_info *i = (struct rat_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_isc_rat_line(i->Point, line)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_OBJ_RAT, &i->Point, PCB_FCT_RAT, NULL))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoRatArc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct rat_info *i = (struct rat_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, arc) && pcb_isc_rat_arc(i->Point, arc)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_OBJ_RAT, &i->Point, PCB_FCT_RAT, NULL))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoRatPoly_callback(const pcb_box_t * b, void *cl)
{
	pcb_poly_t *polygon = (pcb_poly_t *) b;
	struct rat_info *i = (struct rat_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, polygon) && polygon->Clipped && pcb_isc_rat_poly(i->Point, polygon)) {
		if (ADD_POLYGON_TO_LIST(i->layer, polygon, PCB_OBJ_RAT, &i->Point, PCB_FCT_RAT, NULL))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

/* searches all LOs that are connected to the given rat-line on the given
 * layergroup. All found connections are added to the list
 *
 * the notation that is used is:
 * Xij means Xj at line i */
static pcb_bool LookupLOConnectionsToRatEnd(pcb_point_t *Point, pcb_cardinal_t LayerGroup)
{
	pcb_cardinal_t entry;
	struct rat_info info;

	info.Point = Point;
	/* loop over all layers of this group */
	for (entry = 0; entry < PCB->LayerGroups.grp[LayerGroup].len; entry++) {
		pcb_layer_id_t layer;

		layer = PCB->LayerGroups.grp[LayerGroup].lid[entry];
		info.layer = layer;
		if (setjmp(info.env) == 0)
			r_search_pt(LAYER_PTR(layer)->arc_tree, Point, 1, NULL, LOCtoRatArc_callback, &info, NULL);
		else
			return pcb_true;
		if (setjmp(info.env) == 0)
			r_search_pt(LAYER_PTR(layer)->line_tree, Point, 1, NULL, LOCtoRatLine_callback, &info, NULL);
		else
			return pcb_true;
		if (setjmp(info.env) == 0)
			r_search_pt(LAYER_PTR(layer)->polygon_tree, Point, 1, NULL, LOCtoRatPoly_callback, &info, NULL);
	}

	return pcb_false;
}

static pcb_r_dir_t LOCtoPolyLine_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_is_line_in_poly(line, &i->polygon) && !INOCN(line, &i->polygon)) {
		if (ADD_LINE_TO_LIST(i->layer, line, PCB_OBJ_POLY, &i->polygon, PCB_FCT_COPPER, i->orig_polygon))
			longjmp(i->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t LOCtoPolyArc_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!PCB_FLAG_TEST(TheFlag, arc) && pcb_isc_arc_poly(arc, &i->polygon) && !INOCN(arc, &i->polygon)) {
		if (ADD_ARC_TO_LIST(i->layer, arc, PCB_OBJ_POLY, &i->polygon, PCB_FCT_COPPER, i->orig_polygon))
			longjmp(i->env, 1);
	}
	return 0;
}

static pcb_r_dir_t LOCtoPolyRat_callback(const pcb_box_t * b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, rat)) {
		if (((rat->group1 == i->layer) && pcb_isc_rat_poly(&rat->Point1, &i->polygon))
			|| ((rat->group2 == i->layer) && pcb_isc_rat_poly(&rat->Point2, &i->polygon))) {
			if (ADD_RAT_TO_LIST(rat, PCB_OBJ_POLY, &i->polygon, PCB_FCT_RAT, i->orig_polygon))
				longjmp(i->env, 1);
		}
	}
	return PCB_R_DIR_NOT_FOUND;
}


/* looks up LOs that are connected to the given polygon
 * on the given layergroup. All found connections are added to the list */
static pcb_bool LookupLOConnectionsToPolygon(pcb_poly_t *Polygon, pcb_cardinal_t LayerGroup)
{
	pcb_cardinal_t entry;
	struct lo_info info;

	if (!Polygon->Clipped)
		return pcb_false;
	info.polygon = *Polygon;
	info.orig_polygon = Polygon;
	EXPAND_BOUNDS(&info.polygon);
	info.layer = LayerGroup;
	/* check rats */
	if (setjmp(info.env) == 0)
		pcb_r_search(PCB->Data->rat_tree, (pcb_box_t *) & info.polygon, NULL, LOCtoPolyRat_callback, &info, NULL);
	else
		return pcb_true;
/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.grp[LayerGroup].len; entry++) {
		pcb_layer_id_t layer;

		layer = PCB->LayerGroups.grp[LayerGroup].lid[entry];

		/* handle normal layers */
		/* check all polygons */
		{
			pcb_rtree_it_t it;
			pcb_box_t *b;
			for(b = pcb_r_first(PCB->Data->Layer[layer].polygon_tree, &it); b != NULL; b = pcb_r_next(&it)) {
				pcb_poly_t *polygon = (pcb_poly_t *)b;
				if (!PCB_FLAG_TEST(TheFlag, polygon)
						&& pcb_is_poly_in_poly(polygon, Polygon)
						&& ADD_POLYGON_TO_LIST(layer, polygon, PCB_OBJ_POLY, Polygon, PCB_FCT_COPPER, Polygon))
					return pcb_true;
			}
			pcb_r_end(&it);
		}

		info.layer = layer;
		/* check all lines */
		if (setjmp(info.env) == 0)
			pcb_r_search(LAYER_PTR(layer)->line_tree, (pcb_box_t *) & info.polygon, NULL, LOCtoPolyLine_callback, &info, NULL);
		else
			return pcb_true;
		/* check all arcs */
		if (setjmp(info.env) == 0)
			pcb_r_search(LAYER_PTR(layer)->arc_tree, (pcb_box_t *) & info.polygon, NULL, LOCtoPolyArc_callback, &info, NULL);
		else
			return pcb_true;
	}

	return pcb_false;
}
