/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
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

#include "data.h"
#include "board.h"
#include "action_helper.h"
#include "tool.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "event.h"
#include "funchash_core.h"

#include "search.h"
#include "draw.h"
#include "copy.h"
#include "remove.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "layer_vis.h"
#include "operation.h"
#include "obj_pstk.h"
#include "macro.h"
#include "rotate.h"
#include "actions.h"

static const char pcb_acts_Attributes[] = "Attributes(Layout|Layer|Element|Subc)\n" "Attributes(Layer,layername)";
static const char pcb_acth_Attributes[] =
	"Let the user edit the attributes of the layout, current or given\n"
	"layer, or selected subcircuit.";

/* %start-doc actions Attributes

This just pops up a dialog letting the user edit the attributes of the
pcb, a subcircuit, or a layer.

%end-doc */

static fgw_error_t pcb_act_Attributes(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int id;
	const char *layername;
	char *buf;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, Attributes, id = argv[1].val.nat_keyword);
	PCB_ACT_MAY_CONVARG(1, FGW_STR, Attributes, layername = argv[1].val.str);
	PCB_ACT_IRES(0);

	if (!pcb_gui->edit_attributes) {
		pcb_message(PCB_MSG_ERROR, "This GUI doesn't support Attribute Editing\n");
		return FGW_ERR_UNKNOWN;
	}

	switch(id) {
	case F_Layout:
		{
			pcb_gui->edit_attributes("Layout Attributes", &(PCB->Attributes));
			return 0;
		}

	case F_Layer:
		{
			pcb_layer_t *layer = CURRENT;
			if (layername) {
				int i;
				layer = NULL;
				for (i = 0; i < pcb_max_layer; i++)
					if (strcmp(PCB->Data->Layer[i].name, layername) == 0) {
						layer = &(PCB->Data->Layer[i]);
						break;
					}
				if (layer == NULL) {
					pcb_message(PCB_MSG_ERROR, "No layer named %s\n", layername);
					return 1;
				}
			}
			pcb_layer_edit_attrib(layer);
			return 0;
		}

	case F_Element:
	case F_Subc:
		{
			int n_found = 0;
			pcb_subc_t *s = NULL;
			PCB_SUBC_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc)) {
					s = subc;
					n_found++;
				}
			}
			PCB_END_LOOP;
			if (n_found > 1) {
				pcb_message(PCB_MSG_ERROR, "Too many subcircuits selected\n");
				return 1;
			}
			if (n_found == 0) {
				pcb_coord_t x, y;
				void *ptrtmp;
				pcb_hid_get_coords("Click on a subcircuit", &x, &y);
				if ((pcb_search_screen(x, y, PCB_OBJ_SUBC, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_OBJ_VOID)
					s = (pcb_subc_t *)ptrtmp;
				else {
					pcb_message(PCB_MSG_ERROR, "No subcircuit found there\n");
					PCB_ACT_IRES(-1);
					return 0;
				}
			}

			if (s->refdes != NULL)
				buf = pcb_strdup_printf("Subcircuit %s Attributes", s->refdes);
			else
				buf = pcb_strdup("Unnamed Subcircuit's Attributes");

			pcb_gui->edit_attributes(buf, &(s->Attributes));
			free(buf);
			break;
		}

	default:
		PCB_ACT_FAIL(Attributes);
	}

	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_DisperseElements[] = "DisperseElements(All|Selected)";

static const char pcb_acth_DisperseElements[] = "Disperses subcircuits.";

/* %start-doc actions DisperseElements

Normally this is used when starting a board, by selecting all subcircuits
and then dispersing them.  This scatters the subcircuits around the board
so that you can pick individual ones, rather than have all the
subcircuits at the same 0,0 coordinate and thus impossible to choose
from.

%end-doc */

#define GAP PCB_MIL_TO_COORD(100)

static void disperse_obj(pcb_board_t *pcb, pcb_any_obj_t *obj, pcb_coord_t ox, pcb_coord_t oy, pcb_coord_t *dx, pcb_coord_t *dy, pcb_coord_t *minx, pcb_coord_t *miny, pcb_coord_t *maxy)
{
	pcb_coord_t newx2, newy2;

	/* If we want to disperse selected objects, maybe we need smarter
	   code here to avoid putting components on top of others which
	   are not selected.  For now, I'm assuming that this is typically
	   going to be used either with a brand new design or a scratch
	   design holding some new components */

	/* figure out how much to move the object */
	*dx = *minx - obj->BoundingBox.X1;

	/* snap to the grid */
	*dx -= (ox + *dx) % pcb->Grid;

	/* and add one grid size so we make sure we always space by GAP or more */
	*dx += pcb->Grid;

	/* Figure out if this row has room.  If not, start a new row */
	if (GAP + obj->BoundingBox.X2 + *dx > pcb->MaxWidth) {
		*miny = *maxy + GAP;
		*minx = GAP;
	}

	/* figure out how much to move the object */
	*dx = *minx - obj->BoundingBox.X1;
	*dy = *miny - obj->BoundingBox.Y1;

	/* snap to the grid */
	*dx -= (ox + *dx) % pcb->Grid;
	*dx += pcb->Grid;
	*dy -= (oy + *dy) % pcb->Grid;
	*dy += pcb->Grid;

	/* new X2 and Y2 coords with snapping considered */
	newx2 = obj->BoundingBox.X2 + *dx;
	newy2 = obj->BoundingBox.Y2 + *dy;

	/* keep track of how tall this row is */
	*minx = newx2 + GAP;
	if (*maxy < newy2) {
		*maxy = newy2;
		if (*maxy > PCB->MaxHeight - GAP) {
			*maxy = GAP;
			pcb_message(PCB_MSG_WARNING, "The board is too small for hosting all subcircuits,\ndiesperse restarted from the top.\nExpect overlapping subcircuits\n");
		}
	}
}

static fgw_error_t pcb_act_DisperseElements(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	pcb_coord_t minx = GAP, miny = GAP, maxy = GAP, dx, dy;
	int all = 0, bad = 0;

	if (!function || !*function) {
		bad = 1;
	}
	else {
		switch (pcb_funchash_get(function, NULL)) {
		case F_All:
			all = 1;
			break;

		case F_Selected:
			all = 0;
			break;

		default:
			bad = 1;
		}
	}

	if (bad) {
		PCB_ACT_FAIL(DisperseElements);
	}

	pcb_draw_inhibit_inc();
	PCB_SUBC_LOOP(PCB->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, subc) && (all || PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc))) {
			pcb_coord_t ox, oy;
			if (pcb_subc_get_origin(subc, &ox, &oy) != 0) {
				ox = (subc->BoundingBox.X1 + subc->BoundingBox.X2)/2;
				oy = (subc->BoundingBox.Y1 + subc->BoundingBox.Y2)/2;
			}
			disperse_obj(PCB, (pcb_any_obj_t *)subc, ox, oy, &dx, &dy, &minx, &miny, &maxy);
			pcb_move_obj(PCB_OBJ_SUBC, subc, subc, subc, dx, dy);
		}
	}
	PCB_END_LOOP;
	pcb_draw_inhibit_dec();

	/* done with our action so increment the undo # */
	pcb_undo_inc_serial();

	pcb_redraw();
	pcb_board_set_changed_flag(pcb_true);

	return 0;
	PCB_OLD_ACT_END;
}

#undef GAP

/* -------------------------------------------------------------------------- */

static const char pcb_acts_Flip[] = "Flip(Object|Selected|SelectedElements)";

static const char pcb_acth_Flip[] = "Flip a subcircuit to the opposite side of the board.";

/* %start-doc actions Flip

Note that the location of the subcircuit will be symmetric about the
cursor location; i.e. if the part you are pointing at will still be at
the same spot once the subcircuit is on the other side.  When flipping
multiple subcircuits, this retains their positions relative to each
other, not their absolute positions on the board.

%end-doc */

static fgw_error_t pcb_act_Flip(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_coord_t x, y;
	const char *function = PCB_ACTION_ARG(0);
	void *ptrtmp;
	int err = 0;

	pcb_hid_get_coords("Click on Object or Flip Point", &x, &y);

	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			if ((pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_SUBC, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_OBJ_VOID) {
				pcb_subc_t *subc = (pcb_subc_t *)ptrtmp;
				pcb_undo_save_serial();
				pcb_subc_change_side(&subc, 2 * pcb_crosshair.Y - PCB->MaxHeight);
				pcb_undo_inc_serial();
				pcb_draw();
			}
			break;
		case F_Selected:
		case F_SelectedElements:
			pcb_undo_save_serial();
			pcb_selected_subc_change_side();
			pcb_undo_inc_serial();
			pcb_draw();
			break;
		default:
			err = 1;
			break;
		}
		if (!err)
			return 0;
	}

	PCB_ACT_FAIL(Flip);
	PCB_OLD_ACT_END;
}
/* --------------------------------------------------------------------------- */

static const char pcb_acts_MoveObject[] = "pcb_move_obj(X,Y,dim)";

static const char pcb_acth_MoveObject[] = "Moves the object under the crosshair.";

/* %start-doc actions MoveObject

The @code{X} and @code{Y} are treated like @code{delta} is for many
other objects.  For each, if it's prefixed by @code{+} or @code{-},
then that amount is relative.  Otherwise, it's absolute.  Units can be
@code{mil} or @code{mm}; if unspecified, units are PCB's internal
units, currently 1/100 mil.

%end-doc */

static fgw_error_t pcb_act_MoveObject(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *x_str = PCB_ACTION_ARG(0);
	const char *y_str = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	pcb_coord_t nx, ny, x, y;
	pcb_bool absolute1, absolute2;
	void *ptr1, *ptr2, *ptr3;
	int type;

	pcb_hid_get_coords("Select an Object", &x, &y);

	ny = pcb_get_value(y_str, units, &absolute1, NULL);
	nx = pcb_get_value(x_str, units, &absolute2, NULL);

	type = pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_MOVE_TYPES, &ptr1, &ptr2, &ptr3);
	if (type == PCB_OBJ_VOID) {
		pcb_message(PCB_MSG_ERROR, _("Nothing found under crosshair\n"));
		return 1;
	}
	if (absolute1)
		nx -= pcb_crosshair.X;
	if (absolute2)
		ny -= pcb_crosshair.Y;
	pcb_event(PCB_EVENT_RUBBER_RESET, NULL);
	if (conf_core.editor.rubber_band_mode)
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", type, ptr1, ptr2, ptr3);
	if (type == PCB_OBJ_SUBC)
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", type, ptr1, ptr2, ptr3);
	pcb_move_obj_and_rubberband(type, ptr1, ptr2, ptr3, nx, ny);
	pcb_board_set_changed_flag(pcb_true);
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_MoveToCurrentLayer[] = "MoveToCurrentLayer(Object|SelectedObjects)";

static const char pcb_acth_MoveToCurrentLayer[] = "Moves objects to the current layer.";

/* %start-doc actions MoveToCurrentLayer

Note that moving an subcircuit from a component layer to a solder layer,
or from solder to component, won't automatically flip it.  Use the
@code{Flip()} action to do that.

%end-doc */

static fgw_error_t pcb_act_MoveToCurrentLayer(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			{
				pcb_coord_t x, y;
				int type;
				void *ptr1, *ptr2, *ptr3;

				pcb_hid_get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, PCB_MOVETOLAYER_TYPES | PCB_LOOSE_SUBC, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
					pcb_layer_t *target = CURRENT;
					pcb_any_obj_t *o = ptr2;

					/* if object is part of a subc (must be a floater!), target layer
					   shall be within the subc too else we would move out the object from
					   under the subc to under the board as an unwanted side effect */
					if ((o->parent_type == PCB_PARENT_LAYER) && (o->parent.layer->parent.data->parent_type == PCB_PARENT_SUBC)) {
						pcb_subc_t *subc= o->parent.layer->parent.data->parent.subc;
						pcb_layer_type_t lyt = pcb_layer_flags_(CURRENT);
						int old_len = subc->data->LayerN;
						target = pcb_subc_get_layer(subc, lyt, CURRENT->comb, 1, CURRENT->name, 0);
						if (target == NULL) {
							pcb_message(PCB_MSG_ERROR, "Failed to find or allocate the matching subc layer\n");
							break;
						}
						if (old_len != subc->data->LayerN)
							pcb_subc_rebind(PCB, subc); /* had to alloc a new layer */
					}
					if (pcb_move_obj_to_layer(type, ptr1, ptr2, ptr3, target, pcb_false))
						pcb_board_set_changed_flag(pcb_true);
				}
				break;
			}

		case F_SelectedObjects:
		case F_Selected:
			if (pcb_move_selected_objs_to_layer(CURRENT))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

/* ---------------------------------------------------------------- */
static const char pcb_acts_ElementList[] = "ElementList(Start|Done|Need,<refdes>,<footprint>,<value>)";

static const char pcb_acth_ElementList[] = "Adds the given element if it doesn't already exist.";

/* %start-doc actions elementlist

@table @code

@item Start
Indicates the start of an subcircuit list; call this before any Need
actions.

@item Need
Searches the board for an subcircuit with a matching refdes.

If found, the value and footprint are updated.

If not found, a new subcircuit is created with the given footprint and value.

@item Done
Compares the list of subcircuits needed since the most recent
@code{start} with the list of subcircuits actually on the board.  Any
subcircuits that weren't listed are selected, so that the user may delete
them.

@end table

%end-doc */

static int number_of_footprints_not_found;

static int parse_layout_attribute_units(const char *name, int def)
{
	const char *as = pcb_attrib_get(PCB, name);
	if (!as)
		return def;
	return pcb_get_value(as, NULL, NULL, NULL);
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

static fgw_error_t pcb_act_ElementList(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_subc_t *sc;
	const char *refdes, *value, *footprint;
	fgw_arg_t res, args[4];
	const char *function = argv[0];
	int fx, fy, fs;

#ifdef DEBUG
	printf("Entered pcb_act_ElementList, executing function %s\n", function);
#endif

	if (pcb_strcasecmp(function, "start") == 0) {
		PCB_SUBC_LOOP(PCB->Data);
		{
			PCB_FLAG_CLEAR(PCB_FLAG_FOUND, subc);
		}
		PCB_END_LOOP;
		number_of_footprints_not_found = 0;
		return 0;
	}

	if (pcb_strcasecmp(function, "done") == 0) {
		PCB_SUBC_LOOP(PCB->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_FOUND, subc)) {
				PCB_FLAG_CLEAR(PCB_FLAG_FOUND, subc);
			}
			else if (!PCB_EMPTY_STRING_P(subc->refdes)) {
				/* Unnamed elements should remain untouched */
				PCB_FLAG_SET(PCB_FLAG_SELECTED, subc);
			}
		}
		PCB_END_LOOP;
		if (number_of_footprints_not_found > 0)
			pcb_gui->confirm_dialog("Not all requested footprints were found.\n" "See the message log for details", "Ok", NULL);
		return 0;
	}

	if (pcb_strcasecmp(function, "need") != 0)
		PCB_ACT_FAIL(ElementList);

	if (argc != 4)
		PCB_ACT_FAIL(ElementList);

	argc--;
	argv++;

	refdes = PCB_ACTION_ARG(0);
	footprint = PCB_ACTION_ARG(1);
	value = PCB_ACTION_ARG(2);

	args[0].type = FGW_FUNC;
	args[0].val.func = NULL;
	args[1].type = FGW_STR;
	args[1].val.str = (char *)footprint;
	args[2].type = FGW_STR;
	args[2].val.str = (char *)refdes;
	args[3].type = FGW_STR;
	args[3].val.str = (char *)value;
	argc = 4;

	/* turn of flip to avoid mirror/rotat confusion */
	fx = conf_core.editor.view.flip_x;
	fy = conf_core.editor.view.flip_y;
	fs = conf_core.editor.show_solder_side;
	conf_force_set_bool(conf_core.editor.view.flip_x, 0);
	conf_force_set_bool(conf_core.editor.view.flip_y, 0);
	conf_force_set_bool(conf_core.editor.show_solder_side, 0);

#ifdef DEBUG
	printf("  ... footprint = %s\n", footprint);
	printf("  ... refdes = %s\n", refdes);
	printf("  ... value = %s\n", value);
#endif

	sc = pcb_subc_by_refdes(PCB->Data, refdes);

	if (sc == NULL) {
		pcb_coord_t nx, ny, d;

#ifdef DEBUG
		printf("  ... Footprint not on board, need to add it.\n");
#endif
		/* Not on board, need to add it. */
		if (PCB_ACT_CALL_C(pcb_act_LoadFootprint, &res, argc, args) != 0) {
			number_of_footprints_not_found++;
			return 1;
		}

		nx = PCB->MaxWidth / 2;
		ny = PCB->MaxHeight / 2;
		d = MIN(PCB->MaxWidth, PCB->MaxHeight) / 10;

		nx = parse_layout_attribute_units("import::newX", nx);
		ny = parse_layout_attribute_units("import::newY", ny);
		d = parse_layout_attribute_units("import::disperse", d);

		if (d > 0) {
			nx += pcb_rand() % (d * 2) - d;
			ny += pcb_rand() % (d * 2) - d;
		}

		if (nx < 0)
			nx = 0;
		if (nx >= PCB->MaxWidth)
			nx = PCB->MaxWidth - 1;
		if (ny < 0)
			ny = 0;
		if (ny >= PCB->MaxHeight)
			ny = PCB->MaxHeight - 1;

		/* Place components onto center of board. */
		pcb_crosshair.Y = ny; /* flipping side depends on the crosshair unfortunately */
		if (pcb_buffer_copy_to_layout(PCB, nx, ny))
			pcb_board_set_changed_flag(pcb_true);
	}
	else if (sc && subc_differs(sc, footprint)) {
#ifdef DEBUG
		printf("  ... Footprint on board, but different from footprint loaded.\n");
#endif
		int orig_on_top, paste_ok = 0;
		pcb_coord_t orig_cx, orig_cy;
		double orig_rot;

		/* Different footprint, we need to swap them out.  */
		if (PCB_ACT_CALL_C(pcb_act_LoadFootprint, &res, argc, args) != 0) {
			number_of_footprints_not_found++;
			return 1;
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
				pcb_coord_t pcx = 0, pcy = 0;
				pcb_subc_get_origin(psc, &pcx, &pcy);
				if (!orig_on_top)
					pcb_subc_change_side(&psc, pcy * 2 - PCB->MaxHeight);
				if (orig_rot != 0) {
					double cosa, sina;
					cosa = cos(orig_rot / PCB_RAD_TO_DEG);
					sina = sin(orig_rot / PCB_RAD_TO_DEG);
					pcb_subc_rotate(psc, pcx, pcy, cosa, sina, orig_rot);
				}

/* Not needed anymore: pcb_buffer_copy_to_layout solves this
				pcb_opctx_t op;
				op.move.pcb = PCB;
				op.move.dx = orig_cx;
				op.move.dy = orig_cy;
				op.move.dst_layer = NULL;
				op.move.more_to_come = pcb_true;
				pcb_subcop_move(&op, psc);
*/
				paste_ok = 1;
			}
		}

		if (paste_ok) {
			if (sc != NULL)
				pcb_subc_remove(sc);
			if (pcb_buffer_copy_to_layout(PCB, orig_cx, orig_cy))
				pcb_board_set_changed_flag(pcb_true);
		}
	}

	/* Now reload footprint */
	sc = pcb_subc_by_refdes(PCB->Data, refdes);
	if (sc != NULL) {
/*		pcb_attribute_put(&sc->Attributes, "refdes", refdes);*/
		pcb_attribute_put(&sc->Attributes, "value", value);
		PCB_FLAG_SET(PCB_FLAG_FOUND, sc);
	}

#ifdef DEBUG
	printf(" ... Leaving pcb_act_ElementList.\n");
#endif

	conf_force_set_bool(conf_core.editor.view.flip_x, fx);
	conf_force_set_bool(conf_core.editor.view.flip_y, fy);
	conf_force_set_bool(conf_core.editor.show_solder_side, fs);

	return 0;
	PCB_OLD_ACT_END;
}

/* ---------------------------------------------------------------- */
static const char pcb_acts_ElementSetAttr[] = "ElementSetAttr(refdes,name[,value])";

static const char pcb_acth_ElementSetAttr[] = "Sets or clears an element-specific attribute.";

/* %start-doc actions elementsetattr

If a value is specified, the named attribute is added (if not already
present) or changed (if it is) to the given value.  If the value is
not specified, the given attribute is removed if present.

%end-doc */

static fgw_error_t pcb_act_ElementSetAttr(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_subc_t *sc;
	const char *refdes, *name, *value;

	if (argc < 2) {
		PCB_ACT_FAIL(ElementSetAttr);
	}

	refdes = argv[0];
	name = argv[1];
	value = PCB_ACTION_ARG(2);


	sc = pcb_subc_by_refdes(PCB->Data, refdes);
	if (sc == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't find subcircuit with refdes '%s'\n", refdes);
		return 0;
	}

	if (value != NULL)
		pcb_attribute_put(&sc->Attributes, name, value);
	else
		pcb_attribute_remove(&sc->Attributes, name);
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_RipUp[] = "RipUp(All|Selected|Element)";

static const char pcb_acth_RipUp[] = "Ripup auto-routed tracks";

/* %start-doc actions RipUp

@table @code

@item All
Removes all lines and vias which were created by the autorouter.

@item Selected
Removes all selected lines and vias which were created by the
autorouter.

@end table

%end-doc */

static fgw_error_t pcb_act_RipUp(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	pcb_bool changed = pcb_false;

	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_All:
			PCB_LINE_ALL_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_AUTO, line) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, line)) {
					pcb_remove_object(PCB_OBJ_LINE, layer, line, line);
					changed = pcb_true;
				}
			}
			PCB_ENDALL_LOOP;
			PCB_ARC_ALL_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_AUTO, arc) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, arc)) {
					pcb_remove_object(PCB_OBJ_ARC, layer, arc, arc);
					changed = pcb_true;
				}
			}
			PCB_ENDALL_LOOP;

			PCB_PADSTACK_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_AUTO, padstack) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, padstack)) {
					pcb_remove_object(PCB_OBJ_PSTK, padstack, padstack, padstack);
					changed = pcb_true;
				}
			}
			PCB_END_LOOP;

			if (changed) {
				pcb_undo_inc_serial();
				pcb_board_set_changed_flag(pcb_true);
			}
			break;
		case F_Selected:
			PCB_LINE_VISIBLE_LOOP(PCB->Data);
			{
				if (PCB_FLAGS_TEST(PCB_FLAG_AUTO | PCB_FLAG_SELECTED, line)
						&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, line)) {
					pcb_remove_object(PCB_OBJ_LINE, layer, line, line);
					changed = pcb_true;
				}
			}
			PCB_ENDALL_LOOP;
			if (PCB->pstk_on)
			PCB_PADSTACK_LOOP(PCB->Data);
			{
				if (PCB_FLAGS_TEST(PCB_FLAG_AUTO | PCB_FLAG_SELECTED, padstack)
						&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, padstack)) {
					pcb_remove_object(PCB_OBJ_PSTK, padstack, padstack, padstack);
					changed = pcb_true;
				}
			}
			PCB_END_LOOP;
			if (changed) {
				pcb_undo_inc_serial();
				pcb_board_set_changed_flag(pcb_true);
			}
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

/* ---------------------------------------------------------------------------  */

static const char pcb_acts_MinMaskGap[] = "MinMaskGap(delta)\n" "MinMaskGap(Selected, delta)";
static const char pcb_acth_MinMaskGap[] = "Ensures the mask is a minimum distance from pins and pads. Not supported anymore.";
static fgw_error_t pcb_act_MinMaskGap(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_ERROR, "Feature not supported; use padstackedit()\n");
	return 1;
	PCB_OLD_ACT_END;
}

/* ---------------------------------------------------------------------------  */

static const char pcb_acts_MinClearGap[] = "MinClearGap(delta)\n" "MinClearGap(Selected, delta)";

static const char pcb_acth_MinClearGap[] = "Ensures that polygons are a minimum distance from objects.";

/* %start-doc actions MinClearGap

Checks all specified objects, and increases the polygon clearance if
needed to ensure a minimum distance between their edges and the
polygon edges.

%end-doc */

static void minclr(pcb_data_t *data, pcb_coord_t value, int flags)
{
	PCB_SUBC_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, subc))
			continue;
		minclr(subc->data, value, 0);
	}
	PCB_END_LOOP;

	PCB_PADSTACK_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, padstack))
			continue;
		if (padstack->Clearance < value) {
			pcb_chg_obj_clear_size(PCB_OBJ_PSTK, padstack, 0, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_END_LOOP;

	PCB_LINE_ALL_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, line))
			continue;
		if ((line->Clearance != 0) && (line->Clearance < value)) {
			pcb_chg_obj_clear_size(PCB_OBJ_LINE, layer, line, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, arc))
			continue;
		if ((arc->Clearance != 0) && (arc->Clearance < value)) {
			pcb_chg_obj_clear_size(PCB_OBJ_ARC, layer, arc, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, polygon))
			continue;
		if ((polygon->Clearance != 0) && (polygon->Clearance < value)) {
			pcb_chg_obj_clear_size(PCB_OBJ_POLY, layer, polygon, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_ENDALL_LOOP;
}

static fgw_error_t pcb_act_MinClearGap(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *delta = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	pcb_bool absolute;
	pcb_coord_t value;
	int flags;

	if (!function)
		return 1;
	if (pcb_strcasecmp(function, "Selected") == 0)
		flags = PCB_FLAG_SELECTED;
	else {
		units = delta;
		delta = function;
		flags = 0;
	}
	value = 2 * pcb_get_value(delta, units, &absolute, NULL);

	pcb_undo_save_serial();
	minclr(PCB->Data, value, flags);
	pcb_undo_restore_serial();
	pcb_undo_inc_serial();
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char movelayer_syntax[] = "MoveLayer(old,new)";

static const char movelayer_help[] = "Moves/Creates/Deletes Layers.";

/* %start-doc actions MoveLayer

Moves a layer, creates a new layer, or deletes a layer.

@table @code

@item old
The is the layer number to act upon.  Allowed values are:
@table @code

@item c
Currently selected layer.

@item -1
Create a new layer.

@item number
An existing layer number.

@end table

@item new
Specifies where to move the layer to.  Allowed values are:
@table @code
@item -1
Deletes the layer.

@item up
Moves the layer up.

@item down
Moves the layer down.

@item step+
Moves the layer towards the end of its group's list.

@item step-
Moves the layer towards the beginning of its group's list.

@item c
Creates a new layer.

@end table

@end table

%end-doc */
extern pcb_layergrp_id_t pcb_actd_EditGroup_gid;
fgw_error_t pcb_act_MoveLayer(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	int old_index, new_index;

	if (argc != 2) {
		pcb_message(PCB_MSG_ERROR, "Usage; MoveLayer(old,new)");
		return 1;
	}

	if (strcmp(argv[0], "c") == 0)
		old_index = INDEXOFCURRENT;
	else
		old_index = atoi(argv[0]);

	if (strcmp(argv[1], "c") == 0) {
		new_index = INDEXOFCURRENT;
		if (new_index < 0)
			new_index = 0;
	}
	else if (strcmp(argv[1], "gi") == 0) {
		return pcb_layer_move(PCB, -1, 0, pcb_actd_EditGroup_gid);
	}
	else if (strcmp(argv[1], "ga") == 0) {
		return pcb_layer_move(PCB, -1, 1, pcb_actd_EditGroup_gid);
	}
	else if (strcmp(argv[1], "up") == 0) {
		new_index = INDEXOFCURRENT - 1;
		if (new_index < 0)
			return 1;
	}
	else if (strcmp(argv[1], "down") == 0) {
		new_index = INDEXOFCURRENT + 1;
		if (new_index >= pcb_max_layer)
			return 1;
	}
	else if (strncmp(argv[1], "step", 4) == 0) {
		pcb_layer_t *l = CURRENT;
		pcb_layergrp_t *g = pcb_get_layergrp(PCB, l->meta.real.grp);
		if (g == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid layer group\n");
			return 1;
		}
		switch(argv[1][4]) {
			case '+': return pcb_layergrp_step_layer(PCB, g, pcb_layer_id(PCB->Data, l), +1); break;
			case '-': return pcb_layergrp_step_layer(PCB, g, pcb_layer_id(PCB->Data, l), -1); break;
		}
		pcb_message(PCB_MSG_ERROR, "Invalid step direction\n");
		return 1;
	}

	else
		new_index = atoi(argv[1]);

	if (new_index < 0) {
		if (pcb_layer_flags(PCB, old_index) & PCB_LYT_SILK) {
			pcb_layer_t *l = pcb_get_layer(PCB->Data, old_index);
			pcb_layergrp_t *g = pcb_get_layergrp(PCB, l->meta.real.grp);
			if (g->len == 1) {
				pcb_message(PCB_MSG_ERROR, "Removing this layer would result in an empty top or bottom silk group, which is not possible at the moment.\n");
				return 1;
			}
		}
	}

	if (pcb_layer_move(PCB, old_index, new_index, -1))
		return 1;

	return 0;
	PCB_OLD_ACT_END;
}

static pcb_layer_t *pick_layer(const char *user_text)
{
	char *end;
	pcb_layer_id_t id;
	if (*user_text == '#') {
		id = strtol(user_text+1, &end, 10);
		if (*end == '\0')
			return pcb_get_layer(PCB->Data, id);
	}
	return NULL;
}

static const char pcb_acts_CreateText[] = "CreateText(layer, fontID, X, Y, direction, scale, text)\n";
static const char pcb_acth_CreateText[] = "Create a new text object";
static fgw_error_t pcb_act_CreateText(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_coord_t x, y;
	pcb_layer_t *ly;
	int fid = 0, dir = 0, scale = 0;
	pcb_bool succ;

	if (argc != 7)
		PCB_ACT_FAIL(CreateText);

	ly = pick_layer(argv[0]);
	if (ly == NULL) {
		pcb_message(PCB_MSG_ERROR, "Unknown layer %s", argv[0]);
		return 1;
	}

	fid = atoi(argv[1]);
	x = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mm", &succ);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "Invalid X coord %s", argv[2]);
		return 1;
	}
	y = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mm", &succ);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "Invalid Y coord %s", argv[3]);
		return 1;
	}
	dir = atoi(argv[4]);
	scale = atoi(argv[5]);
	if (scale < 1) {
		pcb_message(PCB_MSG_ERROR, "Invalid scale coord %s", argv[5]);
		return 1;
	}

	pcb_text_new(ly, pcb_font(PCB, fid, 1), x, y, dir, scale, argv[6], pcb_no_flags());

	return 0;
	PCB_OLD_ACT_END;
}

static const char pcb_acts_subc[] =
	"subc(hash, [board|selected])\n"
	"subc(loose, on|off|toggle|check)\n"
	;
static const char pcb_acth_subc[] = "Various operations on subc";
static fgw_error_t pcb_act_subc(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	if (argc == 0)
		PCB_ACT_FAIL(subc);
	switch (pcb_funchash_get(argv[0], NULL)) {
		case F_Loose:
			if ((argc < 2) || (strcmp(argv[1], "toggle") == 0))
				PCB->loose_subc = !PCB->loose_subc;
			else if (strcmp(argv[1], "on") == 0)
				PCB->loose_subc = 1;
			else if (strcmp(argv[1], "off") == 0)
				PCB->loose_subc = 0;
			else if (strcmp(argv[1], "check") == 0) {
				ores->type = FGW_INT;
				ores->val.nat_int = PCB->loose_subc;
				return 0;
			}
			else {
				PCB_ACT_FAIL(subc);
				return 1;
			}
			/* have to manually trigger the update as it is not a conf item */
			if ((pcb_gui != NULL) && (pcb_gui->update_menu_checkbox != NULL))
				pcb_gui->update_menu_checkbox(NULL);
			return 0;
		case F_Hash:
			{
				int selected_only = 0;
				gdl_iterator_t it;
				pcb_subc_t *sc;

				if (argc < 1) {
				
				}
				else if (strcmp(argv[1], "selected") == 0)
					selected_only = 1;

				polylist_foreach(&PCB->Data->subc, &it, sc) {
					if (selected_only && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, sc))
						continue;
					pcb_message(PCB_MSG_INFO, "subc #%ld (%s): %u\n", sc->ID, (sc->refdes == NULL ? "<no refdes>" : sc->refdes), pcb_subc_hash(sc));
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
				if (argc < 1) {
				
				}
				else if (strcmp(argv[1], "selected") == 0)
					selected_only = 1;

				polylist_foreach(&PCB->Data->subc, &it, sc) {
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
					pcb_append_printf(&str, "subc eq %u:", e->key);
					for(n = 0; n < vt->used; n++) {
						sc = (pcb_subc_t *)vt->array[n];
						pcb_append_printf(&str, " #%ld(%s):%d", sc->ID, (sc->refdes == NULL ? "<no refdes>" : sc->refdes), pcb_subc_eq(sc, (pcb_subc_t*)vt->array[0]));
					}
					pcb_message(PCB_MSG_INFO, "%s\n", str.array);
					vtp0_uninit(vt);
					free(vt);
				}
				gds_uninit(&str);
				htip_uninit(&hash2scs);
			}
			break;
	}
	return 0;
	PCB_OLD_ACT_END;
}

static const char pcb_acts_Rotate90[] = "pcb_move_obj(steps)";
static const char pcb_acth_Rotate90[] = "Rotates the object under the crosshair by 90 degree steps.";

/* %start-doc actions Rotate90

Rotates the object under the mouse pointer by 90 degree @code{steps}.

%end-doc */

static fgw_error_t pcb_act_Rotate90(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *ssteps = PCB_ACTION_ARG(0);
	int steps = atoi(ssteps);
	pcb_coord_t x, y;

	pcb_hid_get_coords("Select an Object", &x, &y);

	if (conf_core.editor.show_solder_side)
		steps = -steps;

	steps = steps % 4;
	if (steps < 0)
		steps = 4+steps;

	pcb_screen_obj_rotate90(x, y, steps);

	return 0;
	PCB_OLD_ACT_END;
}

pcb_action_t object_action_list[] = {
	{"Attributes", pcb_act_Attributes, pcb_acth_Attributes, pcb_acts_Attributes},
	{"DisperseElements", pcb_act_DisperseElements, pcb_acth_DisperseElements, pcb_acts_DisperseElements},
	{"Flip", pcb_act_Flip, pcb_acth_Flip, pcb_acts_Flip},
	{"MoveObject", pcb_act_MoveObject, pcb_acth_MoveObject, pcb_acts_MoveObject},
	{"MoveToCurrentLayer", pcb_act_MoveToCurrentLayer, pcb_acth_MoveToCurrentLayer, pcb_acts_MoveToCurrentLayer},
	{"ElementList", pcb_act_ElementList, pcb_acth_ElementList, pcb_acts_ElementList},
	{"ElementSetAttr", pcb_act_ElementSetAttr, pcb_acth_ElementSetAttr, pcb_acts_ElementSetAttr},
	{"RipUp", pcb_act_RipUp, pcb_acth_RipUp, pcb_acts_RipUp},
	{"MinMaskGap", pcb_act_MinMaskGap, pcb_acth_MinMaskGap, pcb_acts_MinMaskGap},
	{"MinClearGap", pcb_act_MinClearGap, pcb_acth_MinClearGap, pcb_acts_MinClearGap},
	{"MoveLayer", pcb_act_MoveLayer, movelayer_help, movelayer_syntax},
	{"subc", pcb_act_subc, pcb_acth_subc, pcb_acts_subc},
	{"CreateText", pcb_act_CreateText, pcb_acth_CreateText, pcb_acts_CreateText},
	{"Rotate90", pcb_act_Rotate90, pcb_acth_Rotate90, pcb_acts_Rotate90}
};

PCB_REGISTER_ACTIONS(object_action_list, NULL)
