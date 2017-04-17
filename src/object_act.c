/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "conf_core.h"

#include "data.h"
#include "board.h"
#include "action_helper.h"
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

/* --------------------------------------------------------------------------- */

static pcb_element_t *element_cache = NULL;

static pcb_element_t *find_element_by_refdes(const char *refdes)
{
	if (element_cache && PCB_ELEM_NAME_REFDES(element_cache)
			&& strcmp(PCB_ELEM_NAME_REFDES(element_cache), refdes) == 0)
		return element_cache;

	PCB_ELEMENT_LOOP(PCB->Data);
	{
		if (PCB_ELEM_NAME_REFDES(element)
				&& strcmp(PCB_ELEM_NAME_REFDES(element), refdes) == 0) {
			element_cache = element;
			return element_cache;
		}
	}
	PCB_END_LOOP;
	return NULL;
}

static pcb_attribute_t *lookup_attr(pcb_attribute_list_t *list, const char *name)
{
	int i;
	for (i = 0; i < list->Number; i++)
		if (strcmp(list->List[i].name, name) == 0)
			return &list->List[i];
	return NULL;
}

static void delete_attr(pcb_attribute_list_t *list, pcb_attribute_t * attr)
{
	int idx = attr - list->List;
	if (idx < 0 || idx >= list->Number)
		return;
	if (list->Number - idx > 1)
		memmove(attr, attr + 1, (list->Number - idx - 1) * sizeof(pcb_attribute_t));
	list->Number--;
}

/* ------------------------------------------------------------ */

static const char pcb_acts_Attributes[] = "Attributes(Layout|Layer|Element)\n" "Attributes(Layer,layername)";

static const char pcb_acth_Attributes[] =
	"Let the user edit the attributes of the layout, current or given\n" "layer, or selected element.";

/* %start-doc actions Attributes

This just pops up a dialog letting the user edit the attributes of the
pcb, an element, or a layer.

%end-doc */


static int pcb_act_Attributes(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *layername = PCB_ACTION_ARG(1);
	char *buf;

	if (!function)
		PCB_ACT_FAIL(Attributes);

	if (!pcb_gui->edit_attributes) {
		pcb_message(PCB_MSG_ERROR, _("This GUI doesn't support Attribute Editing\n"));
		return 1;
	}

	switch (pcb_funchash_get(function, NULL)) {
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
					if (strcmp(PCB->Data->Layer[i].Name, layername) == 0) {
						layer = &(PCB->Data->Layer[i]);
						break;
					}
				if (layer == NULL) {
					pcb_message(PCB_MSG_ERROR, _("No layer named %s\n"), layername);
					return 1;
				}
			}
			buf = (char *) malloc(strlen(layer->Name) + strlen("Layer X Attributes"));
			sprintf(buf, "Layer %s Attributes", layer->Name);
			pcb_gui->edit_attributes(buf, &(layer->Attributes));
			free(buf);
			return 0;
		}

	case F_Element:
		{
			int n_found = 0;
			pcb_element_t *e = NULL;
			PCB_ELEMENT_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, element)) {
					e = element;
					n_found++;
				}
			}
			PCB_END_LOOP;
			if (n_found > 1) {
				pcb_message(PCB_MSG_ERROR, _("Too many elements selected\n"));
				return 1;
			}
			if (n_found == 0) {
				void *ptrtmp;
				pcb_gui->get_coords(_("Click on an element"), &x, &y);
				if ((pcb_search_screen(x, y, PCB_TYPE_ELEMENT, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_TYPE_NONE)
					e = (pcb_element_t *) ptrtmp;
				else {
					pcb_message(PCB_MSG_ERROR, _("No element found there\n"));
					return 1;
				}
			}

			if (PCB_ELEM_NAME_REFDES(e)) {
				buf = (char *) malloc(strlen(PCB_ELEM_NAME_REFDES(e)) + strlen("Element X Attributes"));
				sprintf(buf, "Element %s Attributes", PCB_ELEM_NAME_REFDES(e));
			}
			else {
				buf = pcb_strdup("Unnamed Element Attributes");
			}
			pcb_gui->edit_attributes(buf, &(e->Attributes));
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

static const char pcb_acth_DisperseElements[] = "Disperses elements.";

/* %start-doc actions DisperseElements

Normally this is used when starting a board, by selecting all elements
and then dispersing them.  This scatters the elements around the board
so that you can pick individual ones, rather than have all the
elements at the same 0,0 coordinate and thus impossible to choose
from.

%end-doc */

#define GAP PCB_MIL_TO_COORD(100)

static int pcb_act_DisperseElements(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
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


	PCB_ELEMENT_LOOP(PCB->Data);
	{
		/*
		 * If we want to disperse selected elements, maybe we need smarter
		 * code here to avoid putting components on top of others which
		 * are not selected.  For now, I'm assuming that this is typically
		 * going to be used either with a brand new design or a scratch
		 * design holding some new components
		 */
		if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, element) && (all || PCB_FLAG_TEST(PCB_FLAG_SELECTED, element))) {

			/* figure out how much to move the element */
			dx = minx - element->BoundingBox.X1;

			/* snap to the grid */
			dx -= (element->MarkX + dx) % PCB->Grid;

			/*
			 * and add one grid size so we make sure we always space by GAP or
			 * more
			 */
			dx += PCB->Grid;

			/* Figure out if this row has room.  If not, start a new row */
			if (GAP + element->BoundingBox.X2 + dx > PCB->MaxWidth) {
				miny = maxy + GAP;
				minx = GAP;
			}

			/* figure out how much to move the element */
			dx = minx - element->BoundingBox.X1;
			dy = miny - element->BoundingBox.Y1;

			/* snap to the grid */
			dx -= (element->MarkX + dx) % PCB->Grid;
			dx += PCB->Grid;
			dy -= (element->MarkY + dy) % PCB->Grid;
			dy += PCB->Grid;

			/* move the element */
			pcb_element_move(PCB->Data, element, dx, dy);

			/* and add to the undo list so we can undo this operation */
			pcb_undo_add_obj_to_move(PCB_TYPE_ELEMENT, NULL, NULL, element, dx, dy);

			/* keep track of how tall this row is */
			minx += element->BoundingBox.X2 - element->BoundingBox.X1 + GAP;
			if (maxy < element->BoundingBox.Y2) {
				maxy = element->BoundingBox.Y2;
			}
		}

	}
	PCB_END_LOOP;

	/* done with our action so increment the undo # */
	pcb_undo_inc_serial();

	pcb_redraw();
	pcb_board_set_changed_flag(pcb_true);

	return 0;
}

#undef GAP

/* -------------------------------------------------------------------------- */

static const char pcb_acts_Flip[] = "Flip(Object|Selected|SelectedElements)";

static const char pcb_acth_Flip[] = "Flip an element to the opposite side of the board.";

/* %start-doc actions Flip

Note that the location of the element will be symmetric about the
cursor location; i.e. if the part you are pointing at will still be at
the same spot once the element is on the other side.  When flipping
multiple elements, this retains their positions relative to each
other, not their absolute positions on the board.

%end-doc */

static int pcb_act_Flip(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	pcb_element_t *element;
	void *ptrtmp;
	int err = 0;

	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			if ((pcb_search_screen(x, y, PCB_TYPE_ELEMENT, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_TYPE_NONE) {
				element = (pcb_element_t *) ptrtmp;
				pcb_undo_save_serial();
				pcb_element_change_side(element, 2 * pcb_crosshair.Y - PCB->MaxHeight);
				pcb_undo_inc_serial();
				pcb_draw();
			}
			break;
		case F_Selected:
		case F_SelectedElements:
			pcb_undo_save_serial();
			pcb_selected_element_change_side();
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

static int pcb_act_MoveObject(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *x_str = PCB_ACTION_ARG(0);
	const char *y_str = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	pcb_coord_t nx, ny;
	pcb_bool absolute1, absolute2;
	void *ptr1, *ptr2, *ptr3;
	int type;

	ny = pcb_get_value(y_str, units, &absolute1, NULL);
	nx = pcb_get_value(x_str, units, &absolute2, NULL);

	type = pcb_search_screen(x, y, PCB_MOVE_TYPES, &ptr1, &ptr2, &ptr3);
	if (type == PCB_TYPE_NONE) {
		pcb_message(PCB_MSG_ERROR, _("Nothing found under crosshair\n"));
		return 1;
	}
	if (absolute1)
		nx -= x;
	if (absolute2)
		ny -= y;
	pcb_event(PCB_EVENT_RUBBER_RESET, NULL);
	if (conf_core.editor.rubber_band_mode)
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", type, ptr1, ptr2, ptr3);
	if (type == PCB_TYPE_ELEMENT)
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", type, ptr1, ptr2, ptr3);
	pcb_move_obj_and_rubberband(type, ptr1, ptr2, ptr3, nx, ny);
	pcb_board_set_changed_flag(pcb_true);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_MoveToCurrentLayer[] = "MoveToCurrentLayer(Object|SelectedObjects)";

static const char pcb_acth_MoveToCurrentLayer[] = "Moves objects to the current layer.";

/* %start-doc actions MoveToCurrentLayer

Note that moving an element from a component layer to a solder layer,
or from solder to component, won't automatically flip it.  Use the
@code{Flip()} action to do that.

%end-doc */

static int pcb_act_MoveToCurrentLayer(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				pcb_gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, PCB_MOVETOLAYER_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
					if (pcb_move_obj_to_layer(type, ptr1, ptr2, ptr3, CURRENT, pcb_false))
						pcb_board_set_changed_flag(pcb_true);
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
}

/* ---------------------------------------------------------------- */
static const char pcb_acts_ElementList[] = "ElementList(Start|Done|Need,<refdes>,<footprint>,<value>)";

static const char pcb_acth_ElementList[] = "Adds the given element if it doesn't already exist.";

/* %start-doc actions elementlist

@table @code

@item Start
Indicates the start of an element list; call this before any Need
actions.

@item Need
Searches the board for an element with a matching refdes.

If found, the value and footprint are updated.

If not found, a new element is created with the given footprint and value.

@item Done
Compares the list of elements needed since the most recent
@code{start} with the list of elements actually on the board.  Any
elements that weren't listed are selected, so that the user may delete
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

static int pcb_act_ElementList(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_element_t *e = NULL;
	const char *refdes, *value, *footprint;
	const char *args[3];
	const char *function = argv[0];
	char *old;

#ifdef DEBUG
	printf("Entered pcb_act_ElementList, executing function %s\n", function);
#endif

	if (pcb_strcasecmp(function, "start") == 0) {
		PCB_ELEMENT_LOOP(PCB->Data);
		{
			PCB_FLAG_CLEAR(PCB_FLAG_FOUND, element);
		}
		PCB_END_LOOP;
		element_cache = NULL;
		number_of_footprints_not_found = 0;
		return 0;
	}

	if (pcb_strcasecmp(function, "done") == 0) {
		PCB_ELEMENT_LOOP(PCB->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_FOUND, element)) {
				PCB_FLAG_CLEAR(PCB_FLAG_FOUND, element);
			}
			else if (!PCB_EMPTY_STRING_P(PCB_ELEM_NAME_REFDES(element))) {
				/* Unnamed elements should remain untouched */
				PCB_FLAG_SET(PCB_FLAG_SELECTED, element);
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

	args[0] = footprint;
	args[1] = refdes;
	args[2] = value;

#ifdef DEBUG
	printf("  ... footprint = %s\n", footprint);
	printf("  ... refdes = %s\n", refdes);
	printf("  ... value = %s\n", value);
#endif

	e = find_element_by_refdes(refdes);

	if (!e) {
		pcb_coord_t nx, ny, d;

#ifdef DEBUG
		printf("  ... Footprint not on board, need to add it.\n");
#endif
		/* Not on board, need to add it. */
		if (pcb_act_LoadFootprint(argc, args, x, y)) {
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
		if (pcb_buffer_copy_to_layout(PCB, nx, ny))
			pcb_board_set_changed_flag(pcb_true);
	}

	else if (e && strcmp(PCB_ELEM_NAME_DESCRIPTION(e), footprint) != 0) {
#ifdef DEBUG
		printf("  ... Footprint on board, but different from footprint loaded.\n");
#endif
		int er, pr, i;
		pcb_coord_t mx, my;
		pcb_element_t *pe;

		/* Different footprint, we need to swap them out.  */
		if (pcb_act_LoadFootprint(argc, args, x, y)) {
			number_of_footprints_not_found++;
			return 1;
		}

		er = pcb_element_get_orientation(e);
		pe = elementlist_first(&(PCB_PASTEBUFFER->Data->Element));
		if (!PCB_FRONT(e))
			pcb_element_mirror(PCB_PASTEBUFFER->Data, pe, pe->MarkY * 2 - PCB->MaxHeight);
		pr = pcb_element_get_orientation(pe);

		mx = e->MarkX;
		my = e->MarkY;

		if (er != pr)
			pcb_element_rotate90(PCB_PASTEBUFFER->Data, pe, pe->MarkX, pe->MarkY, (er - pr + 4) % 4);

		for (i = 0; i < PCB_MAX_ELEMENTNAMES; i++) {
			pe->Name[i].X = e->Name[i].X - mx + pe->MarkX;
			pe->Name[i].Y = e->Name[i].Y - my + pe->MarkY;
			pe->Name[i].Direction = e->Name[i].Direction;
			pe->Name[i].Scale = e->Name[i].Scale;
		}

		pcb_element_remove(e);

		if (pcb_buffer_copy_to_layout(PCB, mx, my))
			pcb_board_set_changed_flag(pcb_true);
	}

	/* Now reload footprint */
	element_cache = NULL;
	e = find_element_by_refdes(refdes);

	old = pcb_element_text_change(PCB, PCB->Data, e, PCB_ELEMNAME_IDX_REFDES, pcb_strdup(refdes));
	if (old)
		free(old);
	old = pcb_element_text_change(PCB, PCB->Data, e, PCB_ELEMNAME_IDX_VALUE, pcb_strdup(value));
	if (old)
		free(old);

	PCB_FLAG_SET(PCB_FLAG_FOUND, e);

#ifdef DEBUG
	printf(" ... Leaving pcb_act_ElementList.\n");
#endif

	return 0;
}

/* ---------------------------------------------------------------- */
static const char pcb_acts_ElementSetAttr[] = "ElementSetAttr(refdes,name[,value])";

static const char pcb_acth_ElementSetAttr[] = "Sets or clears an element-specific attribute.";

/* %start-doc actions elementsetattr

If a value is specified, the named attribute is added (if not already
present) or changed (if it is) to the given value.  If the value is
not specified, the given attribute is removed if present.

%end-doc */

static int pcb_act_ElementSetAttr(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_element_t *e = NULL;
	const char *refdes, *name, *value;
	pcb_attribute_t *attr;

	if (argc < 2) {
		PCB_ACT_FAIL(ElementSetAttr);
	}

	refdes = argv[0];
	name = argv[1];
	value = PCB_ACTION_ARG(2);

	PCB_ELEMENT_LOOP(PCB->Data);
	{
		if (PCB_NSTRCMP(refdes, PCB_ELEM_NAME_REFDES(element)) == 0) {
			e = element;
			break;
		}
	}
	PCB_END_LOOP;

	if (!e) {
		pcb_message(PCB_MSG_ERROR, _("Cannot change attribute of %s - element not found\n"), refdes);
		return 1;
	}

	attr = lookup_attr(&e->Attributes, name);

	if (attr && value) {
		free(attr->value);
		attr->value = pcb_strdup(value);
	}
	if (attr && !value) {
		delete_attr(&e->Attributes, attr);
	}
	if (!attr && value) {
		pcb_attribute_put(&e->Attributes, name, value, 0);
	}

	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_RipUp[] = "RipUp(All|Selected|Element)";

static const char pcb_acth_RipUp[] = "Ripup auto-routed tracks, or convert an element to parts.";

/* %start-doc actions RipUp

@table @code

@item All
Removes all lines and vias which were created by the autorouter.

@item Selected
Removes all selected lines and vias which were created by the
autorouter.

@item Element
Converts the element under the cursor to parts (vias and lines).  Note
that this uses the highest numbered paste buffer.

@end table

%end-doc */

static int pcb_act_RipUp(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	pcb_bool changed = pcb_false;

	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_All:
			PCB_LINE_ALL_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_AUTO, line) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, line)) {
					pcb_remove_object(PCB_TYPE_LINE, layer, line, line);
					changed = pcb_true;
				}
			}
			PCB_ENDALL_LOOP;
			PCB_ARC_ALL_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_AUTO, arc) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, arc)) {
					pcb_remove_object(PCB_TYPE_ARC, layer, arc, arc);
					changed = pcb_true;
				}
			}
			PCB_ENDALL_LOOP;
			PCB_VIA_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_AUTO, via) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, via)) {
					pcb_remove_object(PCB_TYPE_VIA, via, via, via);
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
					pcb_remove_object(PCB_TYPE_LINE, layer, line, line);
					changed = pcb_true;
				}
			}
			PCB_ENDALL_LOOP;
			if (PCB->ViaOn)
				PCB_VIA_LOOP(PCB->Data);
			{
				if (PCB_FLAGS_TEST(PCB_FLAG_AUTO | PCB_FLAG_SELECTED, via)
						&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, via)) {
					pcb_remove_object(PCB_TYPE_VIA, via, via, via);
					changed = pcb_true;
				}
			}
			PCB_END_LOOP;
			if (changed) {
				pcb_undo_inc_serial();
				pcb_board_set_changed_flag(pcb_true);
			}
			break;
		case F_Element:
			{
				void *ptr1, *ptr2, *ptr3;

				if (pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_TYPE_ELEMENT, &ptr1, &ptr2, &ptr3) != PCB_TYPE_NONE) {
					Note.Buffer = conf_core.editor.buffer_number;
					pcb_buffer_set_number(PCB_MAX_BUFFER - 1);
					pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
					pcb_copy_obj_to_buffer(PCB, PCB_PASTEBUFFER->Data, PCB->Data, PCB_TYPE_ELEMENT, ptr1, ptr2, ptr3);
					pcb_element_smash_buffer(PCB_PASTEBUFFER);
					PCB_PASTEBUFFER->X = 0;
					PCB_PASTEBUFFER->Y = 0;
					pcb_undo_save_serial();
					pcb_erase_obj(PCB_TYPE_ELEMENT, ptr1, ptr1);
					pcb_undo_move_obj_to_remove(PCB_TYPE_ELEMENT, ptr1, ptr2, ptr3);
					pcb_undo_restore_serial();
					pcb_buffer_copy_to_layout(PCB, 0, 0);
					pcb_buffer_set_number(Note.Buffer);
					pcb_board_set_changed_flag(pcb_true);
				}
			}
			break;
		}
	}
	return 0;
}

/* ---------------------------------------------------------------------------  */

static const char pcb_acts_MinMaskGap[] = "MinMaskGap(delta)\n" "MinMaskGap(Selected, delta)";

static const char pcb_acth_MinMaskGap[] = "Ensures the mask is a minimum distance from pins and pads.";

/* %start-doc actions MinMaskGap

Checks all specified pins and/or pads, and increases the mask if
needed to ensure a minimum distance between the pin or pad edge and
the mask edge.

%end-doc */

static int pcb_act_MinMaskGap(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
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
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		PCB_PIN_LOOP(element);
		{
			if (!PCB_FLAGS_TEST(flags, pin))
				continue;
			if (pin->Mask < pin->Thickness + value) {
				pcb_chg_obj_mask_size(PCB_TYPE_PIN, element, pin, 0, pin->Thickness + value, 1);
				pcb_undo_restore_serial();
			}
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			if (!PCB_FLAGS_TEST(flags, pad))
				continue;
			if (pad->Mask < pad->Thickness + value) {
				pcb_chg_obj_mask_size(PCB_TYPE_PAD, element, pad, 0, pad->Thickness + value, 1);
				pcb_undo_restore_serial();
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
	PCB_VIA_LOOP(PCB->Data);
	{
		if (!PCB_FLAGS_TEST(flags, via))
			continue;
		if (via->Mask && via->Mask < via->Thickness + value) {
			pcb_chg_obj_mask_size(PCB_TYPE_VIA, via, 0, 0, via->Thickness + value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_END_LOOP;
	pcb_undo_restore_serial();
	pcb_undo_inc_serial();
	return 0;
}

/* ---------------------------------------------------------------------------  */

static const char pcb_acts_MinClearGap[] = "MinClearGap(delta)\n" "MinClearGap(Selected, delta)";

static const char pcb_acth_MinClearGap[] = "Ensures that polygons are a minimum distance from objects.";

/* %start-doc actions MinClearGap

Checks all specified objects, and increases the polygon clearance if
needed to ensure a minimum distance between their edges and the
polygon edges.

%end-doc */

static int pcb_act_MinClearGap(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
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
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		PCB_PIN_LOOP(element);
		{
			if (!PCB_FLAGS_TEST(flags, pin))
				continue;
			if (pin->Clearance < value) {
				pcb_chg_obj_clear_size(PCB_TYPE_PIN, element, pin, 0, value, 1);
				pcb_undo_restore_serial();
			}
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			if (!PCB_FLAGS_TEST(flags, pad))
				continue;
			if (pad->Clearance < value) {
				pcb_chg_obj_clear_size(PCB_TYPE_PAD, element, pad, 0, value, 1);
				pcb_undo_restore_serial();
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
	PCB_VIA_LOOP(PCB->Data);
	{
		if (!PCB_FLAGS_TEST(flags, via))
			continue;
		if (via->Clearance < value) {
			pcb_chg_obj_clear_size(PCB_TYPE_VIA, via, 0, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(PCB->Data);
	{
		if (!PCB_FLAGS_TEST(flags, line))
			continue;
		if (line->Clearance < value) {
			pcb_chg_obj_clear_size(PCB_TYPE_LINE, layer, line, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(PCB->Data);
	{
		if (!PCB_FLAGS_TEST(flags, arc))
			continue;
		if (arc->Clearance < value) {
			pcb_chg_obj_clear_size(PCB_TYPE_ARC, layer, arc, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_ENDALL_LOOP;
	pcb_undo_restore_serial();
	pcb_undo_inc_serial();
	return 0;
}

/* ---------------------------------------------------------------------------  */
int pcb_act_ListRotations(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		printf("%d %s\n", pcb_element_get_orientation(element), PCB_ELEM_NAME_REFDES(element));
	}
	PCB_END_LOOP;

	return 0;
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

@item c
Creates a new layer.

@end table

@end table

%end-doc */
int pcb_act_MoveLayer(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int old_index, new_index;
	int new_top = -1;

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
	else if (strcmp(argv[1], "up") == 0) {
		new_index = INDEXOFCURRENT - 1;
		if (new_index < 0)
			return 1;
		new_top = new_index;
	}
	else if (strcmp(argv[1], "down") == 0) {
		new_index = INDEXOFCURRENT + 1;
		if (new_index >= pcb_max_layer)
			return 1;
		new_top = new_index;
	}
	else
		new_index = atoi(argv[1]);

	if (new_index < 0) {
		if (pcb_layer_flags(PCB, old_index) & PCB_LYT_SILK) {
			pcb_message(PCB_MSG_ERROR, "Can not remove silk layers\n");
			return 1;
		}
	}

	if (pcb_layer_move(old_index, new_index))
		return 1;

	if (new_index == -1) {
		new_top = old_index;
		if (new_top >= pcb_max_layer)
			new_top--;
		new_index = new_top;
	}
	if (old_index == -1)
		new_top = new_index;

	if (new_top != -1)
		pcb_layervis_change_group_vis(new_index, 1, 1);

	return 0;
}

static pcb_layer_t *pick_layer(const char *user_text)
{
	char *end;
	pcb_layer_id_t id;
	if (*user_text == '#') {
		id = strtol(user_text+1, &end, 10);
		if (*end == '\0')
			return pcb_get_layer(id);
	}
	return NULL;
}

static const char pcb_acts_CreateText[] = "CreateText(layer, fontID, X, Y, direction, scale, text)\n";
static const char pcb_acth_CreateText[] = "Create a new text object";
static int pcb_act_CreateText(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
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
}


pcb_hid_action_t object_action_list[] = {
	{"Attributes", 0, pcb_act_Attributes,
	 pcb_acth_Attributes, pcb_acts_Attributes}
	,
	{"DisperseElements", 0, pcb_act_DisperseElements,
	 pcb_acth_DisperseElements, pcb_acts_DisperseElements}
	,
	{"Flip", N_("Click on Object or Flip Point"), pcb_act_Flip,
	 pcb_acth_Flip, pcb_acts_Flip}
	,
	{"ListRotations", 0, pcb_act_ListRotations,
	 0, 0}
	,
	{"MoveObject", N_("Select an Object"), pcb_act_MoveObject,
	 pcb_acth_MoveObject, pcb_acts_MoveObject}
	,
	{"MoveToCurrentLayer", 0, pcb_act_MoveToCurrentLayer,
	 pcb_acth_MoveToCurrentLayer, pcb_acts_MoveToCurrentLayer}
	,
	{"ElementList", 0, pcb_act_ElementList,
	 pcb_acth_ElementList, pcb_acts_ElementList}
	,
	{"ElementSetAttr", 0, pcb_act_ElementSetAttr,
	 pcb_acth_ElementSetAttr, pcb_acts_ElementSetAttr}
	,
	{"RipUp", 0, pcb_act_RipUp,
	 pcb_acth_RipUp, pcb_acts_RipUp}
	,
	{"MinMaskGap", 0, pcb_act_MinMaskGap,
	 pcb_acth_MinMaskGap, pcb_acts_MinMaskGap}
	,
	{"MinClearGap", 0, pcb_act_MinClearGap,
	 pcb_acth_MinClearGap, pcb_acts_MinClearGap}
	,
	{"MoveLayer", 0, pcb_act_MoveLayer,
	 movelayer_help, movelayer_syntax}
	,
	{"CreateText", 0, pcb_act_CreateText,
	 pcb_acth_CreateText, pcb_acts_CreateText}
};

PCB_REGISTER_ACTIONS(object_action_list, NULL)
