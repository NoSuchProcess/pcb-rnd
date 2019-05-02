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
#include "hidlib_conf.h"

#include "data.h"
#include "board.h"
#include "tool.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "event.h"
#include "funchash_core.h"

#include "search.h"
#include "draw.h"
#include "move.h"
#include "remove.h"
#include "compat_misc.h"
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
/* DOC: attributes.html */
static fgw_error_t pcb_act_Attributes(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int id;
	const char *layername;
	char *buf;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, Attributes, id = fgw_keyword(&argv[1]));
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
				pcb_hid_get_coords("Click on a subcircuit", &x, &y, 0);
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

static const char pcb_acts_DisperseElements[] = "DisperseElements(All|Selected)";
static const char pcb_acth_DisperseElements[] = "Disperses subcircuits.";

#define GAP PCB_MIL_TO_COORD(100)

/* DOC: disperseelements.html */

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
		if (*maxy > PCB->hidlib.size_y - GAP) {
			*maxy = GAP;
			pcb_message(PCB_MSG_WARNING, "The board is too small for hosting all subcircuits,\ndiesperse restarted from the top.\nExpect overlapping subcircuits\n");
		}
	}
}

static fgw_error_t pcb_act_DisperseElements(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_coord_t minx = GAP, miny = GAP, maxy = GAP, dx, dy;
	int all = 0, id;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, DisperseElements, id = fgw_keyword(&argv[1]));
	PCB_ACT_IRES(0);

	switch(id) {
		case F_All:      all = 1; break;
		case F_Selected: all = 0; break;
		default:         PCB_ACT_FAIL(DisperseElements);
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

	pcb_hid_redraw();
	pcb_board_set_changed_flag(pcb_true);

	return 0;
}

#undef GAP

static const char pcb_acts_Flip[] = "Flip(Object|Selected)";
static const char pcb_acth_Flip[] = "Flip a subcircuit to the opposite side of the board.";
/* DOC: flip.html */
static fgw_error_t pcb_act_Flip(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_coord_t x, y;
	int id;
	void *ptrtmp;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, Flip, id = fgw_keyword(&argv[1]));
	PCB_ACT_IRES(0);

	pcb_hid_get_coords("Click on Object or Flip Point", &x, &y, 0);

	switch(id) {
		case F_Object:
			if ((pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_SUBC, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_OBJ_VOID) {
				pcb_subc_t *subc = (pcb_subc_t *)ptrtmp;
				pcb_undo_freeze_serial();
				pcb_subc_change_side(subc, 2 * pcb_crosshair.Y - PCB->hidlib.size_y);
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
			PCB_ACT_FAIL(Flip);
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
	pcb_coord_t x, y;
	void *ptr1, *ptr2, *ptr3;
	int type;

	PCB_ACT_MAY_CONVARG(3, FGW_STR, MoveObject, units = argv[3].val.str);
	fgw_str2coord_unit_set(saved, units);
	PCB_ACT_CONVARG(1, FGW_COORDS, MoveObject, nx = fgw_coords(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_COORDS, MoveObject, ny = fgw_coords(&argv[2]));
	fgw_str2coord_unit_restore(saved);


	pcb_hid_get_coords("Select an Object", &x, &y, 0);

	type = pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_MOVE_TYPES, &ptr1, &ptr2, &ptr3);
	if (type == PCB_OBJ_VOID) {
		pcb_message(PCB_MSG_ERROR, "Nothing found under crosshair\n");
		return 1;
	}

	if (nx->absolute[0])
		nx->c[0] -= pcb_crosshair.X;
	if (ny->absolute[0])
		ny->c[0] -= pcb_crosshair.Y;

	pcb_event(PCB_EVENT_RUBBER_RESET, NULL);
	if (conf_core.editor.rubber_band_mode)
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", type, ptr1, ptr2, ptr3);
	if (type == PCB_OBJ_SUBC)
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", type, ptr1, ptr2, ptr3);
	pcb_move_obj_and_rubberband(type, ptr1, ptr2, ptr3, nx->c[0], ny->c[0]);
	pcb_board_set_changed_flag(pcb_true);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_MoveToCurrentLayer[] = "MoveToCurrentLayer(Object|SelectedObjects)";
static const char pcb_acth_MoveToCurrentLayer[] = "Moves objects to the current layer.";
/* DOC: movetocurrentlayer.html */
static fgw_error_t pcb_act_MoveToCurrentLayer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int id;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, MoveToCurrentLayer, id = fgw_keyword(&argv[1]));
	PCB_ACT_IRES(0);

	switch(id) {
		case F_Object:
			{
				pcb_coord_t x, y;
				int type;
				void *ptr1, *ptr2, *ptr3;

				pcb_hid_get_coords("Select an Object", &x, &y, 0);
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
	return 0;
}

static const char pcb_acts_ElementList[] = "ElementList(Start|Done|Need,<refdes>,<footprint>,<value>)";
static const char pcb_acth_ElementList[] = "Adds the given element if it doesn't already exist.";
/* DOC: elementlist.html */
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

static fgw_error_t pcb_act_ElementList(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	pcb_subc_t *sc;
	const char *refdes, *value, *footprint;
	fgw_arg_t rs, args[4];
	int fx, fy, fs;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, ElementList, op = fgw_keyword(&argv[1]));

#ifdef DEBUG
	printf("Entered pcb_act_ElementList, executing function %s\n", function);
#endif

	if (op == F_Start) {
		PCB_SUBC_LOOP(PCB->Data);
		{
			PCB_FLAG_CLEAR(PCB_FLAG_FOUND, subc);
		}
		PCB_END_LOOP;
		number_of_footprints_not_found = 0;
		return 0;
	}

	if (op == F_Done) {
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
		if ((number_of_footprints_not_found > 0) && (!pcbhl_conf.rc.quiet))
			pcb_message(PCB_MSG_ERROR, "Footprint import: not all requested footprints were found.\nSee the message log above for details\n");
		return 0;
	}

	if (op != F_Need)
		PCB_ACT_FAIL(ElementList);

	PCB_ACT_CONVARG(2, FGW_STR, ElementList, refdes = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_STR, ElementList, footprint = argv[3].val.str);
	PCB_ACT_CONVARG(4, FGW_STR, ElementList, value = argv[4].val.str);

	args[0].type = FGW_FUNC;
	args[0].val.func = NULL;
	args[1] = argv[3];
	args[2] = argv[2];
	args[3] = argv[4];
	argc = 4;

	/* turn of flip to avoid mirror/rotat confusion */
	fx = pcbhl_conf.editor.view.flip_x;
	fy = pcbhl_conf.editor.view.flip_y;
	fs = conf_core.editor.show_solder_side;
	conf_force_set_bool(pcbhl_conf.editor.view.flip_x, 0);
	conf_force_set_bool(pcbhl_conf.editor.view.flip_y, 0);
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
		if (PCB_ACT_CALL_C(pcb_act_LoadFootprint, &rs, argc, args) != 0) {
			number_of_footprints_not_found++;
			PCB_ACT_IRES(1);
			return 0;
		}

		nx = PCB->hidlib.size_x / 2;
		ny = PCB->hidlib.size_y / 2;
		d = MIN(PCB->hidlib.size_x, PCB->hidlib.size_y) / 10;

		nx = parse_layout_attribute_units("import::newX", nx);
		ny = parse_layout_attribute_units("import::newY", ny);
		d = parse_layout_attribute_units("import::disperse", d);

		if (d > 0) {
			nx += pcb_rand() % (d * 2) - d;
			ny += pcb_rand() % (d * 2) - d;
		}

		if (nx < 0)
			nx = 0;
		if (nx >= PCB->hidlib.size_x)
			nx = PCB->hidlib.size_x - 1;
		if (ny < 0)
			ny = 0;
		if (ny >= PCB->hidlib.size_y)
			ny = PCB->hidlib.size_y - 1;

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
		if (PCB_ACT_CALL_C(pcb_act_LoadFootprint, &rs, argc, args) != 0) {
			number_of_footprints_not_found++;
			PCB_ACT_IRES(1);
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
				pcb_coord_t pcx = 0, pcy = 0;
				pcb_subc_get_origin(psc, &pcx, &pcy);
				if (!orig_on_top)
					pcb_subc_change_side(psc, pcy * 2 - PCB->hidlib.size_y);
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

	conf_force_set_bool(pcbhl_conf.editor.view.flip_x, fx);
	conf_force_set_bool(pcbhl_conf.editor.view.flip_y, fy);
	conf_force_set_bool(conf_core.editor.show_solder_side, fs);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ElementSetAttr[] = "ElementSetAttr(refdes,name[,value])";
static const char pcb_acth_ElementSetAttr[] = "Sets or clears an element-specific attribute.";
/* DOC: elementsetattr */
static fgw_error_t pcb_act_ElementSetAttr(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_subc_t *sc;
	const char *refdes, *name, *value;

	PCB_ACT_CONVARG(1, FGW_STR, ElementList, refdes = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, ElementList, name = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, ElementList, value = argv[3].val.str);

	sc = pcb_subc_by_refdes(PCB->Data, refdes);
	if (sc == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't find subcircuit with refdes '%s'\n", refdes);
		PCB_ACT_IRES(1);
		return 0;
	}

	if (value != NULL)
		pcb_attribute_put(&sc->Attributes, name, value);
	else
		pcb_attribute_remove(&sc->Attributes, name);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_RipUp[] = "RipUp(All|Selected|Element)";
static const char pcb_acth_RipUp[] = "Ripup auto-routed tracks";
/* DOC: ripup.html */
static fgw_error_t pcb_act_RipUp(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	pcb_bool changed = pcb_false;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, RipUp, op = fgw_keyword(&argv[1]));

	switch(op) {
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
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_MinClearGap[] = "MinClearGap(delta)\n" "MinClearGap(Selected, delta)";
static const char pcb_acth_MinClearGap[] = "Ensures that polygons are a minimum distance from objects.";
/* DOC: mincleargap.html */
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

static fgw_error_t pcb_act_MinClearGap(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function;
	const char *delta = NULL;
	const char *units = NULL;
	pcb_bool absolute;
	pcb_coord_t value;
	int flags;

	PCB_ACT_CONVARG(1, FGW_STR, MinClearGap, function = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, MinClearGap, delta = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, MinClearGap, delta = argv[3].val.str);

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
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_MoveLayer[] = "MoveLayer(old,new)\nMoveLayer(lid,group,gid)";
static const char pcb_acth_MoveLayer[] = "Moves/Creates/Deletes Layers.";
/* DOC: movelayer.html */
extern pcb_layergrp_id_t pcb_actd_EditGroup_gid;
fgw_error_t pcb_act_MoveLayer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a0, *a1;
	int old_index, new_index;

	PCB_ACT_CONVARG(1, FGW_STR, MoveLayer, a0 = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, MoveLayer, a1 = argv[2].val.str);

	if (strcmp(a0, "c") == 0)
		old_index = INDEXOFCURRENT;
	else
		old_index = atoi(a0);

	if (strcmp(a1, "c") == 0) {
		new_index = INDEXOFCURRENT;
		if (new_index < 0)
			new_index = 0;
	}
	else if (strcmp(a1, "gi") == 0) {
		PCB_ACT_IRES(pcb_layer_move(PCB, -1, 0, pcb_actd_EditGroup_gid));
		return 0;
	}
	else if (strcmp(a1, "ga") == 0) {
		PCB_ACT_IRES(pcb_layer_move(PCB, -1, 1, pcb_actd_EditGroup_gid));
		return 0;
	}
	else if (strcmp(a1, "group") == 0) {
		long gid;
		PCB_ACT_CONVARG(3, FGW_LONG, MoveLayer, gid = argv[3].val.nat_long);
		pcb_layer_move_to_group(PCB, old_index, gid);
		PCB_ACT_IRES(0);
		return 0;
	}
	else if (strcmp(a1, "up") == 0) {
		new_index = INDEXOFCURRENT - 1;
		if (new_index < 0) {
			PCB_ACT_IRES(1);
			return 0;
		}
	}
	else if (strcmp(a1, "down") == 0) {
		new_index = INDEXOFCURRENT + 1;
		if (new_index >= pcb_max_layer) {
			PCB_ACT_IRES(1);
			return 0;
		}
	}
	else if (strncmp(a1, "step", 4) == 0) {
		pcb_layer_t *l = CURRENT;
		pcb_layergrp_t *g = pcb_get_layergrp(PCB, l->meta.real.grp);
		if (g == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid layer group\n");
			return 1;
		}
		switch(a1[4]) {
			case '+': return pcb_layergrp_step_layer(PCB, g, pcb_layer_id(PCB->Data, l), +1); break;
			case '-': return pcb_layergrp_step_layer(PCB, g, pcb_layer_id(PCB->Data, l), -1); break;
		}
		pcb_message(PCB_MSG_ERROR, "Invalid step direction\n");
		PCB_ACT_IRES(1);
		return 0;
	}

	else
		new_index = atoi(a1);

	if (new_index < 0) {
		if (pcb_layer_flags(PCB, old_index) & PCB_LYT_SILK) {
			pcb_layer_t *l = pcb_get_layer(PCB->Data, old_index);
			pcb_layergrp_t *g = pcb_get_layergrp(PCB, l->meta.real.grp);
			if (g->len == 1) {
				pcb_message(PCB_MSG_ERROR, "Removing this layer would result in an empty top or bottom silk group, which is not possible at the moment.\n");
				PCB_ACT_IRES(1);
				return 0;
			}
		}
	}

	PCB_ACT_IRES(pcb_layer_move(PCB, old_index, new_index, -1));
	return 0;
}

static const char pcb_acts_CreateText[] = "CreateText(layer, fontID, X, Y, direction, scale, text)\n";
static const char pcb_acth_CreateText[] = "Create a new text object";
static fgw_error_t pcb_act_CreateText(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *txt;
	pcb_coord_t x, y;
	pcb_layer_t *ly;
	int fid, dir, scale;
	pcb_text_t *t;

	PCB_ACT_CONVARG(1, FGW_LAYER, CreateText, ly = fgw_layer(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_INT, CreateText, fid = argv[2].val.nat_int);
	PCB_ACT_CONVARG(3, FGW_COORD, CreateText, x = fgw_coord(&argv[3]));
	PCB_ACT_CONVARG(4, FGW_COORD, CreateText, y = fgw_coord(&argv[4]));
	PCB_ACT_CONVARG(5, FGW_INT, CreateText, dir = argv[5].val.nat_int);
	PCB_ACT_CONVARG(6, FGW_INT, CreateText, scale = argv[6].val.nat_int);
	PCB_ACT_CONVARG(7, FGW_STR, CreateText, txt = argv[7].val.str);

	if (scale < 1) {
		pcb_message(PCB_MSG_ERROR, "Invalid scale (must be larger than zero)\n");
		return 1;
	}

	if ((dir < 0) || (dir > 3)) {
		pcb_message(PCB_MSG_ERROR, "Invalid direction (must be 0, 1, 2 or 3)\n");
		return 1;
	}

	t = pcb_text_new(ly, pcb_font(PCB, fid, 1), x, y, dir, scale, 0, txt, pcb_no_flags());
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
	int op1, op2 = -2;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, subc, op1 = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_KEYWORD, subc, op2 = fgw_keyword(&argv[2]));
	PCB_ACT_IRES(0);

	switch(op1) {
		case F_Loose:
			switch(op2) {
				case -2:
				case F_Toggle: PCB->loose_subc = !PCB->loose_subc; break;
				case F_On:     PCB->loose_subc = 1; break;
				case F_Off:    PCB->loose_subc = 0; break;
				case F_Check:  PCB_ACT_IRES(PCB->loose_subc); return 0;
				default:       PCB_ACT_FAIL(subc); return 1;
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

				switch(op2) {
					case -2: break;
					case F_Selected: selected_only = 1; break;
					default:         PCB_ACT_FAIL(subc); return 1;
				}

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
				switch(op2) {
					case -2: break;
					case F_Selected: selected_only = 1; break;
					default:         PCB_ACT_FAIL(subc); return 1;
				}
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
}

static const char pcb_acts_Rotate90[] = "pcb_move_obj(steps)";
static const char pcb_acth_Rotate90[] = "Rotates the object under the crosshair by 90 degree steps.";
/* DOC: rotate90.html */
static fgw_error_t pcb_act_Rotate90(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int steps;
	pcb_coord_t x, y;

	PCB_ACT_CONVARG(1, FGW_INT, Rotate90, steps = argv[1].val.nat_int);
	PCB_ACT_IRES(0);

	pcb_hid_get_coords("Select an Object", &x, &y, 0);

	if (conf_core.editor.show_solder_side)
		steps = -steps;

	steps = steps % 4;
	if (steps < 0)
		steps = 4+steps;

	pcb_screen_obj_rotate90(x, y, steps);

	return 0;
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
	{"MinClearGap", pcb_act_MinClearGap, pcb_acth_MinClearGap, pcb_acts_MinClearGap},
	{"MoveLayer", pcb_act_MoveLayer, pcb_acth_MoveLayer, pcb_acts_MoveLayer},
	{"subc", pcb_act_subc, pcb_acth_subc, pcb_acts_subc},
	{"CreateText", pcb_act_CreateText, pcb_acth_CreateText, pcb_acts_CreateText},
	{"Rotate90", pcb_act_Rotate90, pcb_acth_Rotate90, pcb_acts_Rotate90}
};

PCB_REGISTER_ACTIONS(object_action_list, NULL)
