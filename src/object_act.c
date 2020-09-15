/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"

#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>

#include "data.h"
#include "board.h"
#include <librnd/core/tool.h>
#include "change.h"
#include <librnd/core/error.h>
#include "undo.h"
#include "event.h"
#include "funchash_core.h"
#include <librnd/core/funchash_core.h>

#include "search.h"
#include "draw.h"
#include "move.h"
#include "remove.h"
#include "actions_pcb.h"
#include <librnd/core/compat_misc.h>
#include "layer_vis.h"
#include "operation.h"
#include "obj_pstk.h"
#include "rotate.h"
#include <librnd/core/actions.h>
#include "view.h"

static const char pcb_acts_DisperseElements[] = "DisperseElements(All|Selected)";
static const char pcb_acth_DisperseElements[] = "Disperses subcircuits.";

#define GAP RND_MIL_TO_COORD(100)

/* DOC: disperseelements.html */

static void disperse_obj(pcb_board_t *pcb, pcb_any_obj_t *obj, rnd_coord_t ox, rnd_coord_t oy, rnd_coord_t *dx, rnd_coord_t *dy, rnd_coord_t *minx, rnd_coord_t *miny, rnd_coord_t *maxy)
{
	rnd_coord_t newx2, newy2;

	/* If we want to disperse selected objects, maybe we need smarter
	   code here to avoid putting components on top of others which
	   are not selected.  For now, I'm assuming that this is typically
	   going to be used either with a brand new design or a scratch
	   design holding some new components */

	/* figure out how much to move the object */
	*dx = *minx - obj->BoundingBox.X1;

	/* snap to the grid */
	*dx -= (ox + *dx) % pcb->hidlib.grid;

	/* and add one grid size so we make sure we always space by GAP or more */
	*dx += pcb->hidlib.grid;

	/* Figure out if this row has room.  If not, start a new row */
	if (GAP + obj->BoundingBox.X2 + *dx > pcb->hidlib.size_x) {
		*miny = *maxy + GAP;
		*minx = GAP;
	}

	/* figure out how much to move the object */
	*dx = *minx - obj->BoundingBox.X1;
	*dy = *miny - obj->BoundingBox.Y1;

	/* snap to the grid */
	*dx -= (ox + *dx) % pcb->hidlib.grid;
	*dx += pcb->hidlib.grid;
	*dy -= (oy + *dy) % pcb->hidlib.grid;
	*dy += pcb->hidlib.grid;

	/* new X2 and Y2 coords with snapping considered */
	newx2 = obj->BoundingBox.X2 + *dx;
	newy2 = obj->BoundingBox.Y2 + *dy;

	/* keep track of how tall this row is */
	*minx = newx2 + GAP;
	if (*maxy < newy2) {
		*maxy = newy2;
		if (*maxy > pcb->hidlib.size_y - GAP) {
			*maxy = GAP;
			rnd_message(RND_MSG_WARNING, "The board is too small for hosting all subcircuits,\ndiesperse restarted from the top.\nExpect overlapping subcircuits\n");
		}
	}
}

static fgw_error_t pcb_act_DisperseElements(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	rnd_coord_t minx = GAP, miny = GAP, maxy = GAP, dx, dy;
	int all = 0, id;

	RND_ACT_CONVARG(1, FGW_KEYWORD, DisperseElements, id = fgw_keyword(&argv[1]));
	RND_ACT_IRES(0);

	switch(id) {
		case F_All:      all = 1; break;
		case F_Selected: all = 0; break;
		default:         RND_ACT_FAIL(DisperseElements);
	}

	pcb_draw_inhibit_inc();
	PCB_SUBC_LOOP(pcb->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, subc) && (all || PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc))) {
			rnd_coord_t ox, oy;
			if (pcb_subc_get_origin(subc, &ox, &oy) != 0) {
				ox = (subc->BoundingBox.X1 + subc->BoundingBox.X2)/2;
				oy = (subc->BoundingBox.Y1 + subc->BoundingBox.Y2)/2;
			}
			disperse_obj(pcb, (pcb_any_obj_t *)subc, ox, oy, &dx, &dy, &minx, &miny, &maxy);
			pcb_move_obj(PCB_OBJ_SUBC, subc, subc, subc, dx, dy);
		}
	}
	PCB_END_LOOP;
	pcb_draw_inhibit_dec();

	/* done with our action so increment the undo # */
	pcb_undo_inc_serial();

	rnd_hid_redraw(pcb);
	pcb_board_set_changed_flag(pcb, rnd_true);

	return 0;
}

#undef GAP

static const char pcb_acts_Flip[] = "Flip(Object|Selected)";
static const char pcb_acth_Flip[] = "Flip a subcircuit to the opposite side of the board.";
/* DOC: flip.html */
static fgw_error_t pcb_act_Flip(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_coord_t x, y;
	int id;
	void *ptrtmp;

	RND_ACT_CONVARG(1, FGW_KEYWORD, Flip, id = fgw_keyword(&argv[1]));
	RND_ACT_IRES(0);

	rnd_hid_get_coords("Click on Object or Flip Point", &x, &y, 0);

	switch(id) {
		case F_Object:
			if ((pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_SUBC, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_OBJ_VOID) {
				pcb_subc_t *subc = (pcb_subc_t *)ptrtmp;
				pcb_undo_freeze_serial();
				pcb_subc_change_side(subc, 2 * pcb_crosshair.Y - RND_ACT_HIDLIB->size_y);
				pcb_undo_unfreeze_serial();
				pcb_undo_inc_serial();
				pcb_draw();
			}
			break;
		case F_Selected:
		case F_SelectedElements:
			pcb_undo_freeze_serial();
			pcb_selected_subc_change_side();
			pcb_undo_unfreeze_serial();
			pcb_undo_inc_serial();
			pcb_draw();
			break;
		default:
			RND_ACT_FAIL(Flip);
	}
	return 0;
}

static const char pcb_acts_MoveObject[] = "pcb_move_obj(X,Y,[units])";
static const char pcb_acth_MoveObject[] = "Moves the object under the crosshair.";
/* DOC: moveobject.html */
static fgw_error_t pcb_act_MoveObject(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *units = NULL, *saved;
	fgw_coords_t *nx, *ny;
	rnd_coord_t x, y;
	void *ptr1, *ptr2, *ptr3;
	int type;

	RND_ACT_MAY_CONVARG(3, FGW_STR, MoveObject, units = argv[3].val.str);
	fgw_str2coord_unit_set(saved, units);
	RND_ACT_CONVARG(1, FGW_COORDS, MoveObject, nx = fgw_coords(&argv[1]));
	RND_ACT_CONVARG(2, FGW_COORDS, MoveObject, ny = fgw_coords(&argv[2]));
	fgw_str2coord_unit_restore(saved);


	rnd_hid_get_coords("Select an Object", &x, &y, 0);

	type = pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_MOVE_TYPES, &ptr1, &ptr2, &ptr3);
	if (type == PCB_OBJ_VOID) {
		rnd_message(RND_MSG_ERROR, "Nothing found under crosshair\n");
		return 1;
	}

	if (nx->absolute[0])
		nx->c[0] -= pcb_crosshair.X;
	if (ny->absolute[0])
		ny->c[0] -= pcb_crosshair.Y;

	rnd_event(RND_ACT_HIDLIB, PCB_EVENT_RUBBER_RESET, NULL);
	if (conf_core.editor.rubber_band_mode)
		rnd_event(RND_ACT_HIDLIB, PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", type, ptr1, ptr2, ptr3);
	if (type == PCB_OBJ_SUBC)
		rnd_event(RND_ACT_HIDLIB, PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", type, ptr1, ptr2, ptr3);
	pcb_move_obj_and_rubberband(type, ptr1, ptr2, ptr3, nx->c[0], ny->c[0]);
	pcb_board_set_changed_flag(PCB_ACT_BOARD, rnd_true);

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_MoveToCurrentLayer[] = "MoveToCurrentLayer(Object|SelectedObjects)";
static const char pcb_acth_MoveToCurrentLayer[] = "Moves objects to the current layer.";
/* DOC: movetocurrentlayer.html */
static fgw_error_t pcb_act_MoveToCurrentLayer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int id;

	RND_ACT_CONVARG(1, FGW_KEYWORD, MoveToCurrentLayer, id = fgw_keyword(&argv[1]));
	RND_ACT_IRES(0);

	switch(id) {
		case F_Object:
			{
				rnd_coord_t x, y;
				int type;
				void *ptr1, *ptr2, *ptr3;

				rnd_hid_get_coords("Select an Object", &x, &y, 0);
				if ((type = pcb_search_screen(x, y, PCB_MOVETOLAYER_TYPES | PCB_LOOSE_SUBC(pcb), &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
					pcb_layer_t *target = PCB_CURRLAYER(pcb);
					pcb_any_obj_t *o = ptr2;
					pcb_layer_t *ly;

					/* if object is part of a subc (must be a floater!), target layer
					   shall be within the subc too else we would move out the object from
					   under the subc to under the board as an unwanted side effect */
					if ((o->parent_type == PCB_PARENT_LAYER) && (o->parent.layer->parent.data->parent_type == PCB_PARENT_SUBC)) {
						pcb_subc_t *subc= o->parent.layer->parent.data->parent.subc;
						pcb_layer_type_t lyt = pcb_layer_flags_(PCB_CURRLAYER(pcb));
						int old_len = subc->data->LayerN;
						target = pcb_subc_get_layer(subc, lyt, PCB_CURRLAYER(pcb)->comb, 1, PCB_CURRLAYER(pcb)->name, 0);
						if (target == NULL) {
							rnd_message(RND_MSG_ERROR, "Failed to find or allocate the matching subc layer\n");
							break;
						}
						if (old_len != subc->data->LayerN)
							pcb_subc_rebind(pcb, subc); /* had to alloc a new layer */
					}
					ly = ptr1;
					if (((pcb_any_obj_t *)ptr2)->parent_type == PCB_PARENT_LAYER)
						ly = ((pcb_any_obj_t *)ptr2)->parent.layer; /* might be a bound layer, and ptr1 is a resolved one; we don't want undo to put the object back on the resolved layer instead of the original bound layer */
					if (pcb_move_obj_to_layer(type, ly, ptr2, ptr3, target, rnd_false))
						pcb_board_set_changed_flag(pcb, rnd_true);
				}
				break;
			}

		case F_SelectedObjects:
		case F_Selected:
			if (pcb_move_selected_objs_to_layer(PCB_CURRLAYER(pcb)))
				pcb_board_set_changed_flag(pcb, rnd_true);
			break;
	}
	return 0;
}

static const char pcb_acts_ElementList[] = "ElementList(Start|Done|Need,<refdes>,<footprint>,<value>)";
static const char pcb_acth_ElementList[] = "Adds the given element if it doesn't already exist.";
/* DOC: elementlist.html */
static int number_of_footprints_not_found;

static int parse_layout_attribute_units(pcb_board_t *pcb, const char *name, int def)
{
	const char *as = pcb_attrib_get(pcb, name);
	if (!as)
		return def;
	return rnd_get_value(as, NULL, NULL, NULL);
}

static int subc_differs(pcb_subc_t *sc, const char *expect_name)
{
	const char *got_name = pcb_attribute_get(&sc->Attributes, "footprint");
	if ((expect_name != NULL) && (*expect_name == '\0'))
		expect_name = NULL;
	if ((got_name != NULL) && (*got_name == '\0'))
		got_name = NULL;
	if ((got_name == NULL) && (expect_name == NULL))
		return 0;
	if ((got_name == NULL) || (expect_name == NULL))
		return 1;
	return strcmp(got_name, expect_name);
}

typedef struct {
	enum { PLC_DISPERSE, PLC_FRAME, PLC_FIT } plc_method;
	enum { PLC_AT, PLC_MARK, PLC_CENTER } location;
	rnd_coord_t lx, ly, dd;
	pcb_board_t *pcb;

	/* cursor for some placement methods */
	rnd_coord_t c, cx, cy;
	int side;

	/* removal */
	enum { PLC_SELECT, PLC_REMOVE, PLC_LIST } rem_method;
	pcb_view_list_t *remlst;
} placer_t;

static void plc_init(pcb_board_t *pcb, placer_t *plc)
{
	const char *conf_plc_met = conf_core.import.footprint_placement.method;
	const char *conf_rem_met = conf_core.import.footprint_removal.method;

	/* fallback: compatible with the original defaults */
	plc->pcb = pcb;
	plc->plc_method = PLC_DISPERSE;
	plc->rem_method = PLC_SELECT;
	plc->location = PLC_AT;
	plc->lx = pcb->hidlib.size_x / 2;
	plc->ly = pcb->hidlib.size_y / 2;
	plc->dd = MIN(pcb->hidlib.size_x, pcb->hidlib.size_y) / 10;
	plc->c = plc->cx = plc->cy = 0;
	plc->remlst = NULL;

	if ((conf_plc_met != NULL) && (*conf_plc_met != '\0')) {
		const char *s;

		plc->lx = conf_core.import.footprint_placement.x;
		plc->ly = conf_core.import.footprint_placement.y;
		plc->dd = conf_core.import.footprint_placement.disperse;

		if (rnd_strcasecmp(conf_plc_met, "disperse") == 0) plc->plc_method = PLC_DISPERSE;
		else if (rnd_strcasecmp(conf_plc_met, "frame") == 0) plc->plc_method = PLC_FRAME;
		else if (rnd_strcasecmp(conf_plc_met, "fit") == 0) plc->plc_method = PLC_FIT;
		else rnd_message(RND_MSG_ERROR, "Invalid import/footprint_placement/method '%s', falling back to disperse\n", conf_plc_met);

		s = conf_core.import.footprint_placement.location;
		if ((s == NULL) || (*s == '\0')) plc->location = PLC_AT;
		else if (rnd_strcasecmp(s, "mark") == 0) plc->location = PLC_MARK;
		else if (rnd_strcasecmp(s, "center") == 0) plc->location = PLC_CENTER;
		else rnd_message(RND_MSG_ERROR, "Invalid import/footprint_placement/location '%s', falling back to coordinates\n", s);

	}
	else {
		/* use the old import attributes */
		plc->lx = parse_layout_attribute_units(pcb, "import::newX", plc->lx);
		plc->ly = parse_layout_attribute_units(pcb, "import::newY", plc->ly);
		plc->dd = parse_layout_attribute_units(pcb, "import::disperse", plc->dd);
	}

	if ((conf_rem_met != NULL) && (*conf_rem_met != '\0')) {
		if (rnd_strcasecmp(conf_rem_met, "select") == 0) plc->rem_method = PLC_SELECT;
		else if (rnd_strcasecmp(conf_rem_met, "remove") == 0) plc->rem_method = PLC_REMOVE;
		else if (rnd_strcasecmp(conf_rem_met, "list") == 0) plc->rem_method = PLC_LIST;
		else rnd_message(RND_MSG_ERROR, "Invalid import/footprint_removal/method '%s', falling back to select\n", conf_plc_met);
	}

	switch(plc->location) {
		case PLC_AT: break;
		case PLC_MARK:
			if (pcb_marked.status) {
				plc->lx = pcb_marked.X;
				plc->ly = pcb_marked.Y;
				break;
			}
		case PLC_CENTER:
			plc->lx = pcb->hidlib.size_x / 2;
			plc->ly = pcb->hidlib.size_y / 2;
			break;
	}

	if (plc->rem_method == PLC_LIST)
		plc->remlst = calloc(sizeof(pcb_view_list_t), 1);
}

static void plc_place(placer_t *plc, rnd_coord_t *ox,  rnd_coord_t *oy)
{
	rnd_coord_t px = plc->lx, py = plc->ly;
	rnd_box_t bbx;

	switch(plc->plc_method) {
		case PLC_FIT:
			/* not yet implemented */
		case PLC_DISPERSE:
			if (plc->dd > 0) {
				px += rnd_rand() % (plc->dd * 2) - plc->dd;
				py += rnd_rand() % (plc->dd * 2) - plc->dd;
			}
			break;
		case PLC_FRAME:
			pcb_data_bbox(&bbx, PCB_PASTEBUFFER->Data, 0);
			{
				rnd_coord_t bx = bbx.X2 - bbx.X1, by = bbx.Y2 - bbx.Y1;
				rnd_coord_t xo = PCB_PASTEBUFFER->X/2, yo = PCB_PASTEBUFFER->Y/2;
				rnd_coord_t max = (plc->side % 2) ? plc->pcb->hidlib.size_y : plc->pcb->hidlib.size_x;
				rnd_coord_t mdim = (plc->side % 2) ? by : bx;
				rnd_coord_t adim = (plc->side % 2) ? bx : by;

				plc->c += (double)mdim * 0.6;
				switch(plc->side) {
					case 0: px = xo + plc->c; py = yo - (double)(adim) * 0.6; break; /* top */
					case 1: py = yo + plc->c; px = xo + plc->pcb->hidlib.size_x + (double)(adim) * 0.6; break; /* right */
					case 2: px = xo + plc->pcb->hidlib.size_x - plc->c; py = yo + plc->pcb->hidlib.size_y + (double)(adim) * 0.6; break; /* bottom, from right to left */
					case 3: py = yo + plc->pcb->hidlib.size_y - plc->c; px = xo - (double)(adim) * 0.6; break; /* left, from bottom to top */
				}

				plc->c += (double)mdim * 0.6;
				if (plc->c > max) {
					plc->side++;
					if (plc->side > 3)
						plc->side = 0;
					plc->c = 0;
				}
			}
			break;
	}

	*ox = px;
	*oy = py;
}

static void plc_remove(placer_t *plc, pcb_subc_t *sc)
{
	switch(plc->rem_method) {
		case PLC_SELECT:
			PCB_FLAG_SET(PCB_FLAG_SELECTED, sc);
			break;
		case PLC_REMOVE:
			pcb_remove_object(sc->type, sc->parent.data, sc, sc);
			break;
		case PLC_LIST:
			{
				pcb_view_t *v = pcb_view_new(&plc->pcb->hidlib, "removal", RND_UNKNOWN(sc->refdes), "This part is not present on the new netlist");
				pcb_view_append_obj(v, 0, (pcb_any_obj_t *)sc);
				pcb_view_set_bbox_by_objs(plc->pcb->Data, v);
				pcb_view_list_append(plc->remlst, v);
			}
			break;
	}
}

static void plc_end(placer_t *plc)
{
	if (plc->rem_method == PLC_LIST) {
		fgw_arg_t res;
		fgw_arg_t args[4];
		args[1].type = FGW_STR; args[1].val.str = "import_removal";
		args[2].type = FGW_STR; args[2].val.str = "import part removal";
		fgw_ptr_reg(&rnd_fgw, &args[3], PCB_PTR_DOMAIN_VIEWLIST, FGW_PTR | FGW_STRUCT, plc->remlst);
		rnd_actionv_bin(&plc->pcb->hidlib, "viewlist", &res, 4, args);
		plc->remlst = NULL;
	}
	if ((number_of_footprints_not_found > 0) && (!rnd_conf.rc.quiet))
		rnd_message(RND_MSG_ERROR, "Footprint import: not all requested footprints were found.\nSee the message log above for details\n");
}


static fgw_error_t pcb_act_ElementList(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int op;
	pcb_subc_t *sc;
	const char *refdes, *value, *footprint;
	fgw_arg_t rs, args[4];
	int fx, fy, fs;
	static placer_t plc;

	RND_ACT_CONVARG(1, FGW_KEYWORD, ElementList, op = fgw_keyword(&argv[1]));

#ifdef DEBUG
	printf("Entered pcb_act_ElementList, executing function %s\n", function);
#endif

	if (op == F_Start) {
		PCB_SUBC_LOOP(pcb->Data);
		{
			PCB_FLAG_CLEAR(PCB_FLAG_FOUND, subc);
		}
		PCB_END_LOOP;
		number_of_footprints_not_found = 0;
		plc_init(pcb, &plc);
		return 0;
	}

	if (op == F_Done) {
		PCB_SUBC_LOOP(pcb->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_FOUND, subc)) {
				PCB_FLAG_CLEAR(PCB_FLAG_FOUND, subc);
			}
			else if (!RND_EMPTY_STRING_P(subc->refdes) && (!PCB_FLAG_TEST(PCB_FLAG_NONETLIST, subc))) {
				/* Unnamed elements should remain untouched */
				plc_remove(&plc, subc);
			}
		}
		PCB_END_LOOP;
		plc_end(&plc);
		return 0;
	}

	if (op != F_Need)
		RND_ACT_FAIL(ElementList);

	RND_ACT_CONVARG(2, FGW_STR, ElementList, refdes = argv[2].val.str);
	RND_ACT_CONVARG(3, FGW_STR, ElementList, footprint = argv[3].val.str);
	RND_ACT_CONVARG(4, FGW_STR, ElementList, value = argv[4].val.str);

	args[0].type = FGW_FUNC;
	args[0].val.argv0.func = NULL;
	args[0].val.argv0.user_call_ctx = RND_ACT_HIDLIB;
	args[1] = argv[3];
	args[2] = argv[2];
	args[3] = argv[4];
	argc = 4;

	/* turn of flip to avoid mirror/rotat confusion */
	fx = rnd_conf.editor.view.flip_x;
	fy = rnd_conf.editor.view.flip_y;
	fs = conf_core.editor.show_solder_side;
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, 0);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, 0);
	rnd_conf_force_set_bool(conf_core.editor.show_solder_side, 0);

#ifdef DEBUG
	printf("  ... footprint = %s\n", footprint);
	printf("  ... refdes = %s\n", refdes);
	printf("  ... value = %s\n", value);
#endif

	sc = pcb_subc_by_refdes(pcb->Data, refdes);

	if (sc == NULL) {
		rnd_coord_t px, py;

#ifdef DEBUG
		printf("  ... Footprint not on board, need to add it.\n");
#endif
		/* Not on board, need to add it. */
		if (RND_ACT_CALL_C(RND_ACT_HIDLIB, pcb_act_LoadFootprint, &rs, argc, args) != 0) {
			number_of_footprints_not_found++;
			RND_ACT_IRES(1);
			return 0;
		}

		plc_place(&plc, &px, &py);

		/* Place components onto center of board. */
		pcb_crosshair.Y = py; /* flipping side depends on the crosshair unfortunately */
		if (pcb_buffer_copy_to_layout(pcb, px, py, rnd_false))
			pcb_board_set_changed_flag(pcb, rnd_true);
	}
	else if (sc && subc_differs(sc, footprint)) {
#ifdef DEBUG
		printf("  ... Footprint on board, but different from footprint loaded.\n");
#endif
		int orig_on_top, paste_ok = 0;
		rnd_coord_t orig_cx, orig_cy;
		double orig_rot;

		/* Different footprint, we need to swap them out.  */
		if (RND_ACT_CALL_C(RND_ACT_HIDLIB, pcb_act_LoadFootprint, &rs, argc, args) != 0) {
			number_of_footprints_not_found++;
			RND_ACT_IRES(1);
			return 0;
		}

		{
			orig_rot = 0.0;
			orig_cx = 0;
			orig_cy = 0;
			orig_on_top = 0;
			pcb_subc_get_rotation(sc, &orig_rot);
			pcb_subc_get_origin(sc, &orig_cx, &orig_cy);
			pcb_subc_get_side(sc, &orig_on_top);
			orig_on_top = !orig_on_top;
		}

		{
			/* replace with subc */
			pcb_subc_t *psc;
			psc = pcb_subclist_first(&(PCB_PASTEBUFFER->Data->subc));
			if (psc != NULL) {
				rnd_coord_t pcx = 0, pcy = 0;
				pcb_subc_get_origin(psc, &pcx, &pcy);
				if (!orig_on_top)
					pcb_subc_change_side(psc, pcy * 2 - RND_ACT_HIDLIB->size_y);
				if (orig_rot != 0) {
					double cosa, sina;
					cosa = cos(orig_rot / RND_RAD_TO_DEG);
					sina = sin(orig_rot / RND_RAD_TO_DEG);
					pcb_subc_rotate(psc, pcx, pcy, cosa, sina, orig_rot);
				}

/* Not needed anymore: pcb_buffer_copy_to_layout solves this
				pcb_opctx_t op;
				op.move.pcb = PCB;
				op.move.dx = orig_cx;
				op.move.dy = orig_cy;
				op.move.dst_layer = NULL;
				op.move.more_to_come = rnd_true;
				pcb_subcop_move(&op, psc);
*/
				paste_ok = 1;
			}
		}

		if (paste_ok) {
			if (sc != NULL)
				pcb_subc_remove(sc);
			if (pcb_buffer_copy_to_layout(pcb, orig_cx, orig_cy, rnd_false))
				pcb_board_set_changed_flag(pcb,rnd_true);
		}
	}

	/* Now reload footprint */
	sc = pcb_subc_by_refdes(pcb->Data, refdes);
	if (sc != NULL) {
/*		pcb_attribute_put(&sc->Attributes, "refdes", refdes);*/
		pcb_attribute_put(&sc->Attributes, "value", value);
		PCB_FLAG_SET(PCB_FLAG_FOUND, sc);
	}

#ifdef DEBUG
	printf(" ... Leaving pcb_act_ElementList.\n");
#endif

	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, fx);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, fy);
	rnd_conf_force_set_bool(conf_core.editor.show_solder_side, fs);

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ElementSetAttr[] = "ElementSetAttr(refdes,name[,value])";
static const char pcb_acth_ElementSetAttr[] = "Sets or clears an element-specific attribute.";
/* DOC: elementsetattr */
static fgw_error_t pcb_act_ElementSetAttr(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	pcb_subc_t *sc;
	const char *refdes, *name, *value;

	RND_ACT_CONVARG(1, FGW_STR, ElementList, refdes = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, ElementList, name = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, ElementList, value = argv[3].val.str);

	sc = pcb_subc_by_refdes(pcb->Data, refdes);
	if (sc == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't find subcircuit with refdes '%s'\n", refdes);
		RND_ACT_IRES(1);
		return 0;
	}

	if (value != NULL)
		pcb_attribute_put(&sc->Attributes, name, value);
	else
		pcb_attribute_remove(&sc->Attributes, name);

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_RipUp[] = "RipUp(All|Selected|Element)";
static const char pcb_acth_RipUp[] = "Ripup auto-routed tracks";
/* DOC: ripup.html */
static fgw_error_t pcb_act_RipUp(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int op;
	rnd_bool changed = rnd_false;

	RND_ACT_CONVARG(1, FGW_KEYWORD, RipUp, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_All:
			PCB_LINE_ALL_LOOP(pcb->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_AUTO, line) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, line)) {
					pcb_remove_object(PCB_OBJ_LINE, layer, line, line);
					changed = rnd_true;
				}
			}
			PCB_ENDALL_LOOP;
			PCB_ARC_ALL_LOOP(pcb->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_AUTO, arc) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, arc)) {
					pcb_remove_object(PCB_OBJ_ARC, layer, arc, arc);
					changed = rnd_true;
				}
			}
			PCB_ENDALL_LOOP;

			PCB_PADSTACK_LOOP(pcb->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_AUTO, padstack) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, padstack)) {
					pcb_remove_object(PCB_OBJ_PSTK, padstack, padstack, padstack);
					changed = rnd_true;
				}
			}
			PCB_END_LOOP;

			if (changed) {
				pcb_undo_inc_serial();
				pcb_board_set_changed_flag(pcb, rnd_true);
			}
			break;
		case F_Selected:
			PCB_LINE_VISIBLE_LOOP(pcb->Data);
			{
				if (PCB_FLAGS_TEST(PCB_FLAG_AUTO | PCB_FLAG_SELECTED, line)
						&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, line)) {
					pcb_remove_object(PCB_OBJ_LINE, layer, line, line);
					changed = rnd_true;
				}
			}
			PCB_ENDALL_LOOP;
			if (pcb->pstk_on)
			PCB_PADSTACK_LOOP(pcb->Data);
			{
				if (PCB_FLAGS_TEST(PCB_FLAG_AUTO | PCB_FLAG_SELECTED, padstack)
						&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, padstack)) {
					pcb_remove_object(PCB_OBJ_PSTK, padstack, padstack, padstack);
					changed = rnd_true;
				}
			}
			PCB_END_LOOP;
			if (changed) {
				pcb_undo_inc_serial();
				pcb_board_set_changed_flag(pcb, rnd_true);
			}
			break;
	}
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_MoveLayer[] = "MoveLayer(old,new)\nMoveLayer(lid,group,gid)";
static const char pcb_acth_MoveLayer[] = "Moves/Creates/Deletes Layers.";
/* DOC: movelayer.html */
extern rnd_layergrp_id_t pcb_actd_EditGroup_gid;
fgw_error_t pcb_act_MoveLayer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a0, *a1;
	int old_index, new_index;
	pcb_board_t *pcb = PCB_ACT_BOARD;

	RND_ACT_CONVARG(1, FGW_STR, MoveLayer, a0 = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, MoveLayer, a1 = argv[2].val.str);

	if (strcmp(a0, "c") == 0)
		old_index = PCB_CURRLID(pcb);
	else
		old_index = atoi(a0);

	if (strcmp(a1, "c") == 0) {
		new_index = PCB_CURRLID(pcb);
		if (new_index < 0)
			new_index = 0;
	}
	else if (strcmp(a1, "gi") == 0) {
		pcb_layergrp_t *g = pcb_get_layergrp(pcb, pcb_actd_EditGroup_gid);
		if ((g != NULL) && ((g->ltype & PCB_LYT_SUBSTRATE) == 0))
			RND_ACT_IRES(pcb_layer_move(pcb, -1, 0, pcb_actd_EditGroup_gid, 1));
		else
			rnd_message(RND_MSG_ERROR, "Can not add layers in a substrate layer group.\n");
		return 0;
	}
	else if (strcmp(a1, "ga") == 0) {
		pcb_layergrp_t *g = pcb_get_layergrp(pcb, pcb_actd_EditGroup_gid);
		if ((g != NULL) && ((g->ltype & PCB_LYT_SUBSTRATE) == 0))
			RND_ACT_IRES(pcb_layer_move(pcb, -1, 1, pcb_actd_EditGroup_gid, 1));
		else
			rnd_message(RND_MSG_ERROR, "Can not add layers in a substrate layer group.\n");
		return 0;
	}
	else if (strcmp(a1, "group") == 0) {
		long gid;
		RND_ACT_CONVARG(3, FGW_LONG, MoveLayer, gid = argv[3].val.nat_long);
		pcb_layer_move_to_group(pcb, old_index, gid);
		RND_ACT_IRES(0);
		return 0;
	}
	else if (strcmp(a1, "up") == 0) {
		new_index = PCB_CURRLID(pcb) - 1;
		if (new_index < 0) {
			RND_ACT_IRES(1);
			return 0;
		}
	}
	else if (strcmp(a1, "down") == 0) {
		new_index = PCB_CURRLID(pcb) + 1;
		if (new_index >= pcb_max_layer(pcb)) {
			RND_ACT_IRES(1);
			return 0;
		}
	}
	else if (strncmp(a1, "step", 4) == 0) {
		pcb_layer_t *l = PCB_CURRLAYER(pcb);
		pcb_layergrp_t *g = pcb_get_layergrp(pcb, l->meta.real.grp);
		if (g == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid layer group\n");
			return 1;
		}

		RND_ACT_IRES(1);
		switch(a1[4]) {
			case '+': RND_ACT_IRES(pcb_layergrp_step_layer(pcb, g, pcb_layer_id(pcb->Data, l), +1)); break;
			case '-': RND_ACT_IRES(pcb_layergrp_step_layer(pcb, g, pcb_layer_id(pcb->Data, l), -1)); break;
			default: rnd_message(RND_MSG_ERROR, "Invalid step direction\n");
		}
		return 0;
	}

	else
		new_index = atoi(a1);

	if (new_index < 0) {
		if (pcb_layer_flags(pcb, old_index) & PCB_LYT_SILK) {
			pcb_layer_t *l = pcb_get_layer(pcb->Data, old_index);
			pcb_layergrp_t *g = pcb_get_layergrp(pcb, l->meta.real.grp);
			if (g->len == 1) {
				rnd_message(RND_MSG_ERROR, "Removing this layer would result in an empty top or bottom silk group, which is not possible at the moment.\n");
				RND_ACT_IRES(1);
				return 0;
			}
		}
	}

	RND_ACT_IRES(pcb_layer_move(pcb, old_index, new_index, -1, 1));
	return 0;
}

static const char pcb_acts_CreateText[] = "CreateText(layer, fontID, X, Y, direction, scale, text)\n";
static const char pcb_acth_CreateText[] = "Create a new text object";
static fgw_error_t pcb_act_CreateText(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *txt;
	rnd_coord_t x, y;
	pcb_layer_t *ly;
	int fid, dir, scale;
	pcb_text_t *t;

	RND_ACT_CONVARG(1, FGW_LAYER, CreateText, ly = fgw_layer(&argv[1]));
	RND_ACT_CONVARG(2, FGW_INT, CreateText, fid = argv[2].val.nat_int);
	RND_ACT_CONVARG(3, FGW_COORD, CreateText, x = fgw_coord(&argv[3]));
	RND_ACT_CONVARG(4, FGW_COORD, CreateText, y = fgw_coord(&argv[4]));
	RND_ACT_CONVARG(5, FGW_INT, CreateText, dir = argv[5].val.nat_int);
	RND_ACT_CONVARG(6, FGW_INT, CreateText, scale = argv[6].val.nat_int);
	RND_ACT_CONVARG(7, FGW_STR, CreateText, txt = argv[7].val.str);

	if (scale < 1) {
		rnd_message(RND_MSG_ERROR, "Invalid scale (must be larger than zero)\n");
		return 1;
	}

	if ((dir < 0) || (dir > 3)) {
		rnd_message(RND_MSG_ERROR, "Invalid direction (must be 0, 1, 2 or 3)\n");
		return 1;
	}

	t = pcb_text_new(ly, pcb_font(pcb, fid, 1), x, y, dir*90, scale, 0, txt, pcb_no_flags());
	res->type = FGW_LONG;
	res->val.nat_long = (t == NULL ? -1 : t->ID);
	return 0;
}

static const char pcb_acts_subc[] =
	"subc(hash, [board|selected])\n"
	"subc(loose, on|off|toggle|check)\n"
	;
static const char pcb_acth_subc[] = "Various operations on subc";
static fgw_error_t pcb_act_subc(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int op1, op2 = -2;

	RND_ACT_CONVARG(1, FGW_KEYWORD, subc, op1 = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_KEYWORD, subc, op2 = fgw_keyword(&argv[2]));
	RND_ACT_IRES(0);

	switch(op1) {
		case F_Loose:
			switch(op2) {
				case -2:
				case F_Toggle: pcb->loose_subc = !pcb->loose_subc; break;
				case F_On:     pcb->loose_subc = 1; break;
				case F_Off:    pcb->loose_subc = 0; break;
				case F_Check:  RND_ACT_IRES(pcb->loose_subc); return 0;
				default:       RND_ACT_FAIL(subc); return 1;
			}
			/* have to manually trigger the update as it is not a conf item */
			if ((rnd_gui != NULL) && (rnd_gui->update_menu_checkbox != NULL))
				rnd_gui->update_menu_checkbox(rnd_gui, NULL);
			return 0;
		case F_Hash:
			{
				int selected_only = 0;
				gdl_iterator_t it;
				pcb_subc_t *sc;

				switch(op2) {
					case -2: break;
					case F_Selected: selected_only = 1; break;
					default:         RND_ACT_FAIL(subc); return 1;
				}

				polylist_foreach(&pcb->Data->subc, &it, sc) {
					if (selected_only && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, sc))
						continue;
					rnd_message(RND_MSG_INFO, "subc #%ld (%s): %u\n", sc->ID, (sc->refdes == NULL ? "<no refdes>" : sc->refdes), pcb_subc_hash(sc));
				}
			}
			break;
		case F_Eq:
			{
				int selected_only = 0;
				gdl_iterator_t it;
				pcb_subc_t *sc;
				vtp0_t *vt;
				htip_t hash2scs; /* hash to subcircuit vector */
				htip_entry_t *e;
				gds_t str;

				gds_init(&str);
				htip_init(&hash2scs, longhash, longkeyeq);
				switch(op2) {
					case -2: break;
					case F_Selected: selected_only = 1; break;
					default:         RND_ACT_FAIL(subc); return 1;
				}
				polylist_foreach(&pcb->Data->subc, &it, sc) {
					unsigned int hash;
					if (selected_only && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, sc))
						continue;
					hash = pcb_subc_hash(sc);
					vt = htip_get(&hash2scs, hash);
					if (vt == 0) {
						vt = calloc(sizeof(vtp0_t), 1);
						htip_set(&hash2scs, hash, vt);
					}
					vtp0_append(vt, sc);
				}

				/* print the result */
				for (e = htip_first(&hash2scs); e; e = htip_next(&hash2scs, e)) {
					int n;

					vt = e->value;
					str.used = 0;
					rnd_append_printf(&str, "subc eq %u:", e->key);
					for(n = 0; n < vt->used; n++) {
						sc = (pcb_subc_t *)vt->array[n];
						rnd_append_printf(&str, " #%ld(%s):%d", sc->ID, (sc->refdes == NULL ? "<no refdes>" : sc->refdes), pcb_subc_eq(sc, (pcb_subc_t*)vt->array[0]));
					}
					rnd_message(RND_MSG_INFO, "%s\n", str.array);
					vtp0_uninit(vt);
					free(vt);
				}
				gds_uninit(&str);
				htip_uninit(&hash2scs);
			}
			break;
	}

	return 0;
}

static const char pcb_acts_Rotate90[] = "pcb_move_obj(steps)";
static const char pcb_acth_Rotate90[] = "Rotates the object under the crosshair by 90 degree steps.";
/* DOC: rotate90.html */
static fgw_error_t pcb_act_Rotate90(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int steps;
	rnd_coord_t x, y;

	RND_ACT_CONVARG(1, FGW_INT, Rotate90, steps = argv[1].val.nat_int);
	RND_ACT_IRES(0);

	rnd_hid_get_coords("Select an Object", &x, &y, 0);

	if (conf_core.editor.show_solder_side)
		steps = -steps;

	steps = steps % 4;
	if (steps < 0)
		steps = 4+steps;

	pcb_screen_obj_rotate90(PCB_ACT_BOARD, x, y, steps);

	return 0;
}

static const char pcb_acts_GfxMod[] =
	"GfxMod(transparent, [idpath, [#rrggbb]])\n"
	"GfxMod(transparent, [idpath, [x, y]])\n"
	"GfxMod(resize, [idpath, [pdx, pdy1, len]])"
	;
static const char pcb_acth_GfxMod[] = "Modify a gfx object: set transparent pixel on the pixmap or resize by measurement";
/* DOC: gfxmod.html */
static fgw_error_t pcb_act_GfxMod(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	void *ptr1, *ptr2, *ptr3;
	pcb_gfx_t *gfx;
	rnd_coord_t x, y;
	pcb_idpath_t *idp;
	long type;

	RND_ACT_CONVARG(1, FGW_KEYWORD, GfxMod, op = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_IDPATH, GfxMod, idp = fgw_idpath(&argv[2]));

	if (argc <= 2) {
		rnd_hid_get_coords("Click on a gfx", &x, &y, 0);
		type = pcb_search_screen(x, y, PCB_OBJ_GFX, (void **)&ptr1, (void **)&ptr2, (void **)&ptr3);
		if (type != PCB_OBJ_GFX) {
			rnd_message(RND_MSG_ERROR, "No gfx on that location.\n");
			RND_ACT_IRES(-1);
			return 0;
		}
		gfx = ptr2;
	}
	else {
		gfx = (pcb_gfx_t *)pcb_idpath2obj(PCB, idp);
		if ((gfx == NULL) || (gfx->type != PCB_OBJ_GFX))
			return FGW_ERR_ARG_CONV;
	}

	switch(op) {
		case F_Transparent:
			if (argc > 4) {
				long px, py;
				RND_ACT_CONVARG(3, FGW_LONG, GfxMod, px = argv[3].val.nat_long);
				RND_ACT_CONVARG(4, FGW_LONG, GfxMod, py = argv[4].val.nat_long);
				RND_ACT_IRES(gfx_set_transparent_by_coord(gfx, px, py));
				return 0;
			}
			else if (argc > 3) {
				rnd_color_t clr;
				char *s;
				RND_ACT_CONVARG(3, FGW_STR, GfxMod, s = argv[3].val.str);
				if (rnd_color_load_str(&clr, s) == 0)
					gfx_set_transparent(gfx, clr.r, clr.g, clr.b, 1);
				else
					RND_ACT_FAIL(GfxMod);
			}
			else
				gfx_set_transparent_gui(gfx);
			break;
		case F_Resize:
			if (argc > 5) {
				long pdx, pdy;
				rnd_coord_t len;
				RND_ACT_CONVARG(3, FGW_LONG, GfxMod, pdx = argv[3].val.nat_long);
				RND_ACT_CONVARG(4, FGW_LONG, GfxMod, pdy = argv[4].val.nat_long);
				RND_ACT_CONVARG(5, FGW_COORD, GfxMod, len = fgw_coord(&argv[5]));
				RND_ACT_IRES(gfx_set_resize_by_pixel_dist(gfx, pdx, pdy, len, 1, 1, 1));
				return 0;
			}
			else
				gfx_set_resize_gui(RND_ACT_HIDLIB, gfx, 1, 1);
			break;
		default:
			RND_ACT_FAIL(GfxMod);
	}

	RND_ACT_IRES(0);
	return 0;
}

static rnd_action_t object_action_list[] = {
	{"DisperseElements", pcb_act_DisperseElements, pcb_acth_DisperseElements, pcb_acts_DisperseElements},
	{"Flip", pcb_act_Flip, pcb_acth_Flip, pcb_acts_Flip},
	{"MoveObject", pcb_act_MoveObject, pcb_acth_MoveObject, pcb_acts_MoveObject},
	{"MoveToCurrentLayer", pcb_act_MoveToCurrentLayer, pcb_acth_MoveToCurrentLayer, pcb_acts_MoveToCurrentLayer},
	{"ElementList", pcb_act_ElementList, pcb_acth_ElementList, pcb_acts_ElementList},
	{"ElementSetAttr", pcb_act_ElementSetAttr, pcb_acth_ElementSetAttr, pcb_acts_ElementSetAttr},
	{"RipUp", pcb_act_RipUp, pcb_acth_RipUp, pcb_acts_RipUp},
	{"MoveLayer", pcb_act_MoveLayer, pcb_acth_MoveLayer, pcb_acts_MoveLayer},
	{"subc", pcb_act_subc, pcb_acth_subc, pcb_acts_subc},
	{"CreateText", pcb_act_CreateText, pcb_acth_CreateText, pcb_acts_CreateText},
	{"Rotate90", pcb_act_Rotate90, pcb_acth_Rotate90, pcb_acts_Rotate90},
	{"GfxMod", pcb_act_GfxMod, pcb_acth_GfxMod, pcb_acts_GfxMod}
};

void pcb_object_act_init2(void)
{
	RND_REGISTER_ACTIONS(object_action_list, NULL);
}
