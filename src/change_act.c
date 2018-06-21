/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

/* action routines for output window
 */

#include "config.h"
#include "conf_core.h"

#include "data.h"
#include "funchash_core.h"

#include "board.h"
#include "action_helper.h"
#include "actions.h"
#include "change.h"
#include "draw.h"
#include "search.h"
#include "undo.h"
#include "event.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_rat_draw.h"
#include "data_it.h"
#include "macro.h"
#include "grid.h"

static void ChangeFlag(const char *, const char *, int, const char *);
static int pcb_act_ChangeSize(int oargc, const char **oargv);
static int pcb_act_Change2ndSize(int oargc, const char **oargv);

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeClearSize[] =
	"ChangeClearSize(Object, delta|style)\n"
	"ChangeClearSize(SelectedPins|SelectedPads|SelectedVias, delta|style)\n"
	"ChangeClearSize(SelectedLines|SelectedArcs, delta|style)\n" "ChangeClearSize(Selected|SelectedObjects, delta|style)";

static const char pcb_acth_ChangeClearSize[] = "Changes the clearance size of objects.";

/* %start-doc actions ChangeClearSize

If the solder mask is currently showing, this action changes the
solder mask clearance.  If the mask is not showing, this action
changes the polygon clearance.

%end-doc */

static int pcb_act_ChangeClearSize(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *delta = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	pcb_bool absolute;
	pcb_coord_t value;
	int type = PCB_OBJ_VOID;
	void *ptr1, *ptr2, *ptr3;
	pcb_coord_t x, y;
	int got_coords = 0;

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			pcb_hid_get_coords(_("Select an Object"), &x, &y);
			got_coords = 1;
			type = pcb_search_screen(x, y, PCB_CHANGECLEARSIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		if (strcmp(argv[1], "style") == 0) {
			if (!got_coords) {
				pcb_hid_get_coords(_("Select an Object"), &x, &y);
				got_coords = 1;
			}

			if ((type == PCB_OBJ_VOID) || (type == PCB_OBJ_POLY))	/* workaround: pcb_search_screen(PCB_CHANGECLEARSIZE_TYPES) wouldn't return elements */
				type = pcb_search_screen(x, y, PCB_CHANGE2NDSIZE_TYPES, &ptr1, &ptr2, &ptr3);
			if (pcb_get_style_size(funcid, &value, type, 2) != 0)
				return 1;
			absolute = 1;
			value *= 2;
		}
		else
			value = 2 * pcb_get_value(delta, units, &absolute, NULL);
		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			{
				if (type != PCB_OBJ_VOID)
					if (pcb_chg_obj_clear_size(type, ptr1, ptr2, ptr3, value, absolute))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}
		case F_SelectedVias:
		case F_SelectedPads:
		case F_SelectedPins:
			if (pcb_chg_selected_clear_size(PCB_OBJ_PSTK, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_SelectedLines:
			if (pcb_chg_selected_clear_size(PCB_OBJ_LINE, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_SelectedArcs:
			if (pcb_chg_selected_clear_size(PCB_OBJ_ARC, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_clear_size(PCB_CHANGECLEARSIZE_TYPES, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeFlag[] =
	"ChangeFlag(Object|Selected|SelectedObjects, flag, value)\n"
	"ChangeFlag(SelectedLines|SelectedPins|SelectedVias, flag, value)\n"
	"ChangeFlag(SelectedPads|SelectedTexts|SelectedNames, flag, value)\n"
	"ChangeFlag(SelectedElements, flag, value)\n" "flag = thermal | join\n" "value = 0 | 1";

static const char pcb_acth_ChangeFlag[] = "Sets or clears flags on objects.";

/* %start-doc actions ChangeFlag

Toggles the given flag on the indicated object(s).  The flag may be
one of the flags listed above (thermal, join).  The
value may be the number 0 or 1.  If the value is 0, the flag is
cleared.  If the value is 1, the flag is set.

%end-doc */

static int pcb_act_ChangeFlag(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *flag = PCB_ACTION_ARG(1);
	int value = argc > 2 ? atoi(argv[2]) : -1;
	if (value != 0 && value != 1)
		PCB_ACT_FAIL(ChangeFlag);

	ChangeFlag(function, flag, value, "ChangeFlag");
	return 0;
	PCB_OLD_ACT_END;
}


static void ChangeFlag(const char *what, const char *flag_name, int value,
											 const char *cmd_name)
{
	pcb_bool(*set_object) (int, void *, void *, void *);
	pcb_bool(*set_selected) (int);

	if (PCB_NSTRCMP(flag_name, "join") == 0) {
		/* Note: these are backwards, because the flag is "clear" but
		   the command is "join".  */
		set_object = value ? pcb_clr_obj_join : pcb_set_obj_join;
		set_selected = value ? pcb_clr_selected_join : pcb_set_selected_join;
	}
	else {
		pcb_message(PCB_MSG_ERROR, _("%s():  Flag \"%s\" is not valid\n"), cmd_name, flag_name);
		return;
	}

	switch (pcb_funchash_get(what, NULL)) {
	case F_Object:
		{
			int type;
			void *ptr1, *ptr2, *ptr3;
			pcb_coord_t x, y;

			pcb_hid_get_coords("Click on object to change", &x, &y);

			if ((type = pcb_search_screen(x, y, PCB_CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID)
				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2))
					pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
			if (set_object(type, ptr1, ptr2, ptr3))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}

	case F_SelectedVias:
	case F_SelectedPins:
	case F_SelectedPads:
		if (set_selected(PCB_OBJ_PSTK))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_SelectedLines:
		if (set_selected(PCB_OBJ_LINE))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_SelectedTexts:
		if (set_selected(PCB_OBJ_TEXT))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_SelectedNames:
	case F_SelectedElements:
		pcb_message(PCB_MSG_ERROR, "Feature not supported\n");
		break;

	case F_Selected:
	case F_SelectedObjects:
		if (set_selected(PCB_CHANGESIZE_TYPES))
			pcb_board_set_changed_flag(pcb_true);
		break;
	}
}

/* --------------------------------------------------------------------------- */
static const char changehold_syntax[] = "ChangeHole(ToggleObject|Object|SelectedVias|Selected)";
static const char changehold_help[] = "Changes the hole flag of objects. Not supported anymore; use the propery editor.";
static int pcb_act_ChangeHole(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_ERROR, "Feature not supported with padstacks.\n");
	return 1;
	PCB_OLD_ACT_END;
}
static const char pcb_acts_ChangePaste[] = "ChangePaste(ToggleObject|Object|SelectedPads|Selected)";
static const char pcb_acth_ChangePaste[] = "Changes the no paste flag of objects. Not supported anymore.";
static int pcb_act_ChangePaste(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_ERROR, "Feature not supported with padstacks.\n");
	return 1;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeSizes[] =
	"ChangeSizes(Object, delta|style)\n"
	"ChangeSizes(SelectedObjects|Selected, delta|style)\n"
	"ChangeSizes(SelectedLines|SelectedPins|SelectedVias, delta|style)\n"
	"ChangeSizes(SelectedPads|SelectedTexts|SelectedNames, delta|style)\n" "ChangeSizes(SelectedElements, delta|style)";

static const char pcb_acth_ChangeSizes[] = "Changes all sizes of objects.";

/* %start-doc actions ChangeSize

Call pcb_act_ChangeSize, ActionChangeDrillSize and pcb_act_ChangeClearSize
with the same arguments. If any of them did not fail, return success.
%end-doc */

static int pcb_act_ChangeSizes(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	int a, b, c;
	pcb_undo_save_serial();
	a = pcb_act_ChangeSize(argc, argv);
	pcb_undo_restore_serial();
	b = pcb_act_Change2ndSize(argc, argv);
	pcb_undo_restore_serial();
	c = pcb_act_ChangeClearSize(argc, argv);
	pcb_undo_restore_serial();
	pcb_undo_inc_serial();
	return !(!a || !b || !c);
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeSize[] =
	"ChangeSize(Object, delta|style)\n"
	"ChangeSize(SelectedObjects|Selected, delta|style)\n"
	"ChangeSize(SelectedLines|SelectedPins|SelectedVias, delta|style)\n"
	"ChangeSize(SelectedPads|SelectedTexts|SelectedNames, delta|style)\n" "ChangeSize(SelectedElements, delta|style)";

static const char pcb_acth_ChangeSize[] = "Changes the size of objects.";

/* %start-doc actions ChangeSize

For lines and arcs, this changes the width.  For padstacks, this
changes the shape size (but does not touch the hole diameter). For
texts, this changes the scaling factor.  For subcircuits, this
changes the width of the silk layer lines and arcs for this element.

%end-doc */

static int pcb_act_ChangeSize(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *delta = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	pcb_bool absolute;								/* indicates if absolute size is given */
	pcb_coord_t value;
	int type = PCB_OBJ_VOID, tostyle = 0;
	void *ptr1, *ptr2, *ptr3;


	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			pcb_coord_t x, y;
			pcb_hid_get_coords("Click on object to change size of", &x, &y);
			type = pcb_search_screen(x, y, PCB_CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		if (strcmp(argv[1], "style") == 0) {
			if (pcb_get_style_size(funcid, &value, type, 0) != 0)
				return 1;
			absolute = 1;
			tostyle = 1;
		}
		else
			value = pcb_get_value(delta, units, &absolute, NULL);
		switch (funcid) {
		case F_Object:
			{
				if (type != PCB_OBJ_VOID)
					if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2))
						pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
				if (tostyle) {
					if (pcb_chg_obj_1st_size(type, ptr1, ptr2, ptr3, value, absolute))
						pcb_board_set_changed_flag(pcb_true);
				}
				else {
					if (pcb_chg_obj_size(type, ptr1, ptr2, ptr3, value, absolute))
						pcb_board_set_changed_flag(pcb_true);
				}
				break;
			}
		case F_SelectedVias:
		case F_SelectedPins:
		case F_SelectedPads:
			if (pcb_chg_selected_size(PCB_OBJ_PSTK, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedArcs:
			if (pcb_chg_selected_size(PCB_OBJ_ARC, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedLines:
			if (pcb_chg_selected_size(PCB_OBJ_LINE, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedTexts:
			if (pcb_chg_selected_size(PCB_OBJ_TEXT, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedNames:
		case F_SelectedElements:
			pcb_message(PCB_MSG_ERROR, "Feature not supported.\n");
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_size(PCB_CHANGESIZE_TYPES, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char changedrillsize_syntax[] =
	"ChangeDrillSize(Object, delta|style)\n" "ChangeDrillSize(SelectedPins|SelectedVias|Selected|SelectedObjects, delta|style)";

static const char changedrillsize_help[] = "Changes the drilling hole size of objects.";

/* %start-doc actions ChangeDrillSize

%end-doc */

static int pcb_act_Change2ndSize(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *delta = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	int type = PCB_OBJ_VOID;
	void *ptr1, *ptr2, *ptr3;

	pcb_bool absolute;
	pcb_coord_t value;

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			pcb_coord_t x, y;
			pcb_hid_get_coords(_("Select an Object"), &x, &y);
			type = pcb_search_screen(x, y, PCB_CHANGE2NDSIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		if (strcmp(argv[1], "style") == 0) {
			if (pcb_get_style_size(funcid, &value, type, 1) != 0)
				return 1;
			absolute = 1;
		}
		else
			value = pcb_get_value(delta, units, &absolute, NULL);

		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			{

				if (type != PCB_OBJ_VOID)
					if (pcb_chg_obj_2nd_size(type, ptr1, ptr2, ptr3, value, absolute, pcb_true))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}

		case F_SelectedPadstacks:
		case F_SelectedVias:
		case F_SelectedPins:
			if (pcb_chg_selected_2nd_size(PCB_OBJ_PSTK, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_2nd_size(PCB_OBJ_CLASS_PIN, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

/* ---------------------------------------------------------------------------  */

static const char pcb_acts_ChangePinName[] = "ChangePinName(Refdes,PinNumber,PinName)";

static const char pcb_acth_ChangePinName[] = "Sets the name of a specific pin on a specific subcircuit.";

/* %start-doc actions ChangePinName

This can be especially useful for annotating pin names from a
schematic to the layout without requiring knowledge of the pcb file
format.

@example
ChangePinName(U3, 7, VCC)
@end example

%end-doc */

static int pcb_act_ChangePinName(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_cardinal_t changed = 0;
	const char *refdes, *pinnum, *pinname;

	if (argc != 3) {
		PCB_ACT_FAIL(ChangePinName);
	}

	refdes = argv[0];
	pinnum = argv[1];
	pinname = argv[2];

	PCB_SUBC_LOOP(PCB->Data);
	{
		if ((subc->refdes != NULL) && (PCB_NSTRCMP(refdes, subc->refdes) == 0)) {
			pcb_any_obj_t *o;
			pcb_data_it_t it;

			for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
				if ((o->term != NULL) && (PCB_NSTRCMP(pinnum, o->term) == 0)) {
#warning TODO: make this undoable
					pcb_attribute_put(&o->Attributes, "name", pinname);
					pcb_board_set_changed_flag(pcb_true);
					changed++;
				}
			}
		}
	}
	PCB_END_LOOP;

	/*
	 * done with our action so increment the undo # if we actually
	 * changed anything
	 */
	if (changed) {
		if (defer_updates)
			defer_needs_update = 1;
		else {
			/* pcb_undo_inc_serial(); */
			pcb_gui->invalidate_all();
		}
	}

	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeName[] = "ChangeName(Object)\n" "ChangeName(Layout|Layer)";

static const char pcb_acth_ChangeName[] = "Sets the name (or pin number) of objects.";

/* %start-doc actions ChangeName

@table @code

@item Object
Changes the name of the element under the cursor.

@item Layout
Changes the name of the layout.  This is printed on the fab drawings.

@item Layer
Changes the name of the currently active layer.

@end table

%end-doc */

int pcb_act_ChangeName(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	char *name;
	int pinnum;
	pcb_objtype_t type;

	if (function) {
		switch (pcb_funchash_get(function, NULL)) {

		/* change the refdes of a subcircuit */
		case F_Subc:
		{
			pcb_coord_t x, y;
			pcb_hid_get_coords("Select a subcircuit", &x, &y);
			type = PCB_OBJ_SUBC;
			goto do_chg_name;
		}
			/* change the name of an object */
		case F_Object:
			{
				pcb_coord_t x, y;
				void *ptr1, *ptr2, *ptr3;
				pcb_hid_get_coords(_("Select an Object"), &x, &y);
				type = PCB_CHANGENAME_TYPES;
				do_chg_name:;
				if ((type = pcb_search_screen(x, y, type, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
					pcb_undo_save_serial();
					pinnum = 0;
					if (pcb_chg_obj_name_query(ptr2)) {
						pcb_redraw();
						pcb_board_set_changed_flag(pcb_true);
						pcb_event(PCB_EVENT_RUBBER_RENAME, "ipppi", type, ptr1, ptr2, ptr3, pinnum);
					}
				}
				break;
			}

			/* change the layout's name */
		case F_Layout:
			name = pcb_gui->prompt_for(_("Enter the layout name:"), PCB_EMPTY(PCB->Name));
			/* NB: ChangeLayoutName takes ownership of the passed memory */
			if (name && pcb_board_change_name(name))
				pcb_board_set_changed_flag(pcb_true);
			break;

			/* change the name of the active layer */
		case F_Layer:
			name = pcb_gui->prompt_for(_("Enter the layer name:"), PCB_EMPTY(CURRENT->name));
			/* NB: pcb_layer_rename_ takes ownership of the passed memory */
			if (name && (pcb_layer_rename_(CURRENT, name) == 0))
				pcb_board_set_changed_flag(pcb_true);
			else
				free(name);
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeJoin[] = "ChangeJoin(ToggleObject|SelectedLines|SelectedArcs|Selected)";

static const char pcb_acth_ChangeJoin[] = "Changes the join (clearance through polygons) of objects.";

/* %start-doc actions ChangeJoin

The join flag determines whether a line or arc, drawn to intersect a
polygon, electrically connects to the polygon or not.  When joined,
the line/arc is simply drawn over the polygon, making an electrical
connection.  When not joined, a gap is drawn between the line and the
polygon, insulating them from each other.

%end-doc */

static int pcb_act_ChangeJoin(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				pcb_coord_t x, y;
				int type;
				void *ptr1, *ptr2, *ptr3;

				pcb_hid_get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, PCB_CHANGEJOIN_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID)
					if (pcb_chg_obj_join(type, ptr1, ptr2, ptr3))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}

		case F_SelectedLines:
			if (pcb_chg_selected_join(PCB_OBJ_LINE))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedArcs:
			if (pcb_chg_selected_join(PCB_OBJ_ARC))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_join(PCB_CHANGEJOIN_TYPES))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeNonetlist[] =
	"ChangeNonetlist(ToggleObject)\n" "ChangeNonetlist(SelectedElements)\n" "ChangeNonetlist(Selected|SelectedObjects)";

static const char pcb_acth_ChangeNonetlist[] = "Changes the nonetlist flag of subcircuits.";

/* %start-doc actions ChangeNonetlist

Note that @code{Pins} means both pins and pads.

%end-doc */

static int pcb_act_ChangeNonetlist(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
		case F_Element:
			{
				pcb_coord_t x, y;
				int type;
				void *ptr1, *ptr2, *ptr3;
				pcb_hid_get_coords(_("Select an Element"), &x, &y);

				ptr3 = NULL;
				type = pcb_search_screen(x, y, PCB_CHANGENONETLIST_TYPES, &ptr1, &ptr2, &ptr3);
				if (pcb_chg_obj_nonetlist(type, ptr1, ptr2, ptr3))
					pcb_board_set_changed_flag(pcb_true);
				break;
			}
		case F_SelectedElements:
		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_nonetlist(PCB_OBJ_SUBC))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

#warning padstack TODO: remove the next few?
/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeSquare[] = "ChangeSquare(ToggleObject)\n" "ChangeSquare(SelectedElements|SelectedPins)\n" "ChangeSquare(Selected|SelectedObjects)";
static const char pcb_acth_ChangeSquare[] = "Changes the square flag of pins and pads. Not supported anymore.";
static int pcb_act_ChangeSquare(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_ERROR, "Feature not supported with padstacks.\n");
	return 1;
	PCB_OLD_ACT_END;
}
static const char pcb_acts_SetSquare[] = "SetSquare(ToggleObject|SelectedElements|SelectedPins)";
static const char pcb_acth_SetSquare[] = "sets the square-flag of objects. Not supported anymore.";
static int pcb_act_SetSquare(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_ERROR, "Feature not supported with padstacks.\n");
	return 1;
	PCB_OLD_ACT_END;
}

static const char pcb_acts_ClearSquare[] = "ClearSquare(ToggleObject|SelectedElements|SelectedPins)";
static const char pcb_acth_ClearSquare[] = "Clears the square-flag of pins and pads. Not supported anymore.";
static int pcb_act_ClearSquare(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_ERROR, "Feature not supported with padstacks.\n");
	return 1;
	PCB_OLD_ACT_END;
}
static const char pcb_acts_ChangeOctagon[] = "ChangeOctagon(Object|ToggleObject|SelectedObjects|Selected)\n" "ChangeOctagon(SelectedElements|SelectedPins|SelectedVias)";
static const char pcb_acth_ChangeOctagon[] = "Changes the octagon-flag of pins and vias. Not supported anymore.";
static int pcb_act_ChangeOctagon(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_ERROR, "Feature not supported with padstacks.\n");
	return 1;
	PCB_OLD_ACT_END;
}
static const char pcb_acts_SetOctagon[] = "SetOctagon(Object|ToggleObject|SelectedElements|Selected)";
static const char pcb_acth_SetOctagon[] = "Sets the octagon-flag of objects. Not supported anymore.";
static int pcb_act_SetOctagon(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_ERROR, "Feature not supported with padstacks.\n");
	return 1;
	PCB_OLD_ACT_END;
}
static const char pcb_acts_ClearOctagon[] = "ClearOctagon(ToggleObject|Object|SelectedObjects|Selected)\n" "ClearOctagon(SelectedElements|SelectedPins|SelectedVias)";
static const char pcb_acth_ClearOctagon[] = "Clears the octagon-flag of pins and vias. Not supported anymore.";
static int pcb_act_ClearOctagon(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_ERROR, "Feature not supported with padstacks.\n");
	return 0;
	PCB_OLD_ACT_END;
}

/* -------------------------------------------------------------------------- */

static const char pcb_acts_SetThermal[] = "SetThermal(Object|SelectedPins|SelectedVias|Selected, Style)";

static const char pcb_acth_SetThermal[] =
	"Set the thermal (on the current layer) of padstacks to the given style.\n"
	"Style = 0 means no thermal.\n"
	"Style = 1 has diagonal fingers with sharp edges.\n"
	"Style = 2 has horizontal and vertical fingers with sharp edges.\n"
	"Style = 3 is a solid connection to the plane.\n"
	"Style = 4 has diagonal fingers with rounded edges.\n"
	"Style = 5 has horizontal and vertical fingers with rounded edges.\n";

/* %start-doc actions SetThermal

This changes how/whether padstacks connect to any rectangle or polygon
on the current layer. The first argument can specify one object, or all
selected padstacks.
The second argument specifies the style of connection.
There are 5 possibilities:
0 - no connection,
1 - 45 degree fingers with sharp edges,
2 - horizontal & vertical fingers with sharp edges,
3 - solid connection,
4 - 45 degree fingers with rounded corners,
5 - horizontal & vertical fingers with rounded corners.

Padstacks may have thermals whether or not there is a polygon available
to connect with. However, they will have no effect without the polygon.
%end-doc */

static int pcb_act_SetThermal(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *style = PCB_ACTION_ARG(1);
	void *ptr1, *ptr2, *ptr3;
	int type, kind;
	int err = 0;
	pcb_coord_t gx, gy;

	if (function && *function && style && *style) {
		pcb_bool absolute;

		kind = pcb_get_value_ex(style, NULL, &absolute, NULL, NULL, NULL);
		if (absolute && (kind <= 5))
			switch (pcb_funchash_get(function, NULL)) {
			case F_Object:
				pcb_hid_get_coords("Click on object for SetThermal", &gx, &gy);
				if ((type = pcb_search_screen(gx, gy, PCB_CHANGETHERMAL_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
					pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, kind, INDEXOFCURRENT);
					pcb_undo_inc_serial();
					pcb_draw();
				}
				break;
			case F_SelectedPins:
			case F_SelectedVias:
				pcb_chg_selected_thermals(PCB_OBJ_PSTK, kind, INDEXOFCURRENT);
				break;
			case F_Selected:
			case F_SelectedElements:
				pcb_chg_selected_thermals(PCB_CHANGETHERMAL_TYPES, kind, INDEXOFCURRENT);
				break;
			default:
				err = 1;
				break;
			}
		else
			err = 1;
		if (!err)
			return 0;
	}

	PCB_ACT_FAIL(SetThermal);
	PCB_OLD_ACT_END;
}


/* --------------------------------------------------------------------------- */

static const char pcb_acts_SetFlag[] =
	"SetFlag(Object|Selected|SelectedObjects, flag)\n"
	"SetFlag(SelectedLines|SelectedPins|SelectedVias, flag)\n"
	"SetFlag(SelectedPads|SelectedTexts|SelectedNames, flag)\n"
	"SetFlag(SelectedElements, flag)\n" "flag = thermal | join";

static const char pcb_acth_SetFlag[] = "Sets flags on objects.";

/* %start-doc actions SetFlag

Turns the given flag on, regardless of its previous setting.  See
@code{ChangeFlag}.

@example
SetFlag(SelectedPins,thermal)
@end example

%end-doc */

static int pcb_act_SetFlag(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *flag = PCB_ACTION_ARG(1);
	ChangeFlag(function, flag, 1, "SetFlag");
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ClrFlag[] =
	"ClrFlag(Object|Selected|SelectedObjects, flag)\n"
	"ClrFlag(SelectedLines|SelectedPins|SelectedVias, flag)\n"
	"ClrFlag(SelectedPads|SelectedTexts|SelectedNames, flag)\n"
	"ClrFlag(SelectedElements, flag)\n" "flag = thermal | join";

static const char pcb_acth_ClrFlag[] = "Clears flags on objects.";

/* %start-doc actions ClrFlag

Turns the given flag off, regardless of its previous setting.  See
@code{ChangeFlag}.

@example
ClrFlag(SelectedLines,join)
@end example

%end-doc */

static int pcb_act_ClrFlag(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *flag = PCB_ACTION_ARG(1);
	ChangeFlag(function, flag, 0, "ClrFlag");
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_SetValue[] = "SetValue(Grid|Line|LineSize|Text|TextScale, delta)";

static const char pcb_acth_SetValue[] = "Change various board-wide values and sizes.";

/* %start-doc actions SetValue

@table @code

@item Grid
Sets the grid spacing.

@item Line
@item LineSize
Changes the thickness of new lines.

@item Text
@item TextScale
Changes the size of new text.

@end table

%end-doc */

static int pcb_act_SetValue(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *val = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	pcb_bool absolute;								/* flag for 'absolute' value */
	double value;
	int err = 0;

	if (function && val) {
		int fnc_id = pcb_funchash_get(function, NULL);

		/* special case: can't convert with pcb_get_value() */
		if ((fnc_id == F_Grid) && ((val[0] == '*') || (val[0] == '/'))) {
			double d;
			char *end;

			d = strtod(val+1, &end);
			if ((*end != '\0') || (d <= 0)) {
				pcb_message(PCB_MSG_ERROR, "SetValue: Invalid multiplier/divider for grid set: needs to be a positive number\n");
				return 1;
			}
			pcb_grid_inval();
			if (val[0] == '*')
				pcb_board_set_grid(pcb_round(PCB->Grid * d), pcb_false, 0, 0);
			else
				pcb_board_set_grid(pcb_round(PCB->Grid / d), pcb_false, 0, 0);
		}

		value = pcb_get_value(val, units, &absolute, NULL);

		switch (fnc_id) {

		case F_Grid:
			pcb_grid_inval();
			if (absolute)
				pcb_board_set_grid(value, pcb_false, 0, 0);
			else {
				/* On the way down, short against the minimum
				 * PCB drawing unit */
				if ((value + PCB->Grid) < 1)
					pcb_board_set_grid(1, pcb_false, 0, 0);
				else if (PCB->Grid == 1)
					pcb_board_set_grid(value, pcb_false, 0, 0);
				else
					pcb_board_set_grid(value + PCB->Grid, pcb_false, 0, 0);
			}
			break;

		case F_LineSize:
		case F_Line:
			pcb_board_set_line_width(absolute ? value : value + conf_core.design.line_thickness);
			pcb_event(PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
			break;

		case F_Text:
		case F_TextScale:
			value = pcb_round(value / (double)PCB_FONT_CAPHEIGHT * 100.0);
			pcb_board_set_text_scale(absolute ? value : (value + conf_core.design.text_scale));
			break;
		default:
			err = 1;
			break;
		}
		if (!err)
			return 0;
	}

	PCB_ACT_FAIL(SetValue);
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeAngle[] =
	"ChangeAngle(Object, start|delta|both, delta)\n"
	"ChangeAngle(SelectedObjects|Selected, start|delta|both, delta)\n"
	"ChangeAngle(SelectedArcs, start|delta|both, delta)\n";
static const char pcb_acth_ChangeAngle[] = "Changes the start angle, delta angle or both angles of an arc.";
static int pcb_act_ChangeAngle(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *prim  = PCB_ACTION_ARG(1);
	const char *delta = PCB_ACTION_ARG(2);
	pcb_bool absolute;								/* indicates if absolute size is given */
	double value;
	int type = PCB_OBJ_VOID, which;
	void *ptr1, *ptr2, *ptr3;

	if (pcb_strcasecmp(prim, "start") == 0) which = 0;
	else if (pcb_strcasecmp(prim, "delta") == 0) which = 1;
	else if (pcb_strcasecmp(prim, "both") == 0) which = 2;
	else {
		pcb_message(PCB_MSG_ERROR, "Second argument of ChangeAngle must be start, delta or both\n");
		return -1;
	}

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			pcb_coord_t x, y;
			pcb_hid_get_coords("Click on object to change angle of", &x, &y);
			type = pcb_search_screen(x, y, PCB_CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		{ /* convert angle from string */
			char *end;
			while(isspace(*delta)) delta++;
			value = strtod(delta, &end);
			if (*end != '\0') {
				pcb_message(PCB_MSG_ERROR, "Invalid numeric (in angle)\n");
				return -1;
			}
			absolute = ((*delta != '-') && (*delta != '+'));
		}
		
		switch (funcid) {
		case F_Object:
			{
				if (type != PCB_OBJ_VOID) {
					if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2))
						pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
					else {
						if (pcb_chg_obj_angle(type, ptr1, ptr2, ptr3, which, value, absolute))
							pcb_board_set_changed_flag(pcb_true);
					}
				}
				break;
			}

		case F_SelectedArcs:
			if (pcb_chg_selected_angle(PCB_OBJ_ARC, which, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_angle(PCB_CHANGESIZE_TYPES, which, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeRadius[] =
	"ChangeRadius(Object, width|x|height|y|both, delta)\n"
	"ChangeRadius(SelectedObjects|Selected, width|x|height|y|both, delta)\n"
	"ChangeRadius(SelectedArcs, width|x|height|y|both, delta)\n";
static const char pcb_acth_ChangeRadius[] = "Changes the width or height (radius) of an arc.";
static int pcb_act_ChangeRadius(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	const char *prim  = PCB_ACTION_ARG(1);
	const char *delta = PCB_ACTION_ARG(2);
	const char *units = PCB_ACTION_ARG(3);
	pcb_bool absolute;								/* indicates if absolute size is given */
	double value;
	int type = PCB_OBJ_VOID, which;
	void *ptr1, *ptr2, *ptr3;

	if ((pcb_strcasecmp(prim, "width") == 0) || (pcb_strcasecmp(prim, "x") == 0)) which = 0;
	else if ((pcb_strcasecmp(prim, "height") == 0) || (pcb_strcasecmp(prim, "y") == 0)) which = 1;
	else if (pcb_strcasecmp(prim, "both") == 0) which = 2;
	else {
		pcb_message(PCB_MSG_ERROR, "Second argument of ChangeRadius must be width, x, height, y or both\n");
		return -1;
	}

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			pcb_coord_t x, y;
			pcb_hid_get_coords("Click on object to change radius of", &x, &y);
			type = pcb_search_screen(x, y, PCB_CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		value = pcb_get_value(delta, units, &absolute, NULL);

		switch (funcid) {
		case F_Object:
			{
				if (type != PCB_OBJ_VOID) {
					if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2))
						pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
					else {
						if (pcb_chg_obj_radius(type, ptr1, ptr2, ptr3, which, value, absolute))
							pcb_board_set_changed_flag(pcb_true);
					}
				}
				break;
			}

		case F_SelectedArcs:
			if (pcb_chg_selected_radius(PCB_OBJ_ARC, which, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_radius(PCB_CHANGESIZE_TYPES, which, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}


/* --------------------------------------------------------------------------- */

pcb_action_t change_action_list[] = {
	{"ChangeAngle", pcb_act_ChangeAngle, pcb_acth_ChangeAngle, pcb_acts_ChangeAngle},
	{"ChangeClearSize", pcb_act_ChangeClearSize, pcb_acth_ChangeClearSize, pcb_acts_ChangeClearSize},
	{"ChangeDrillSize", pcb_act_Change2ndSize, changedrillsize_help, changedrillsize_syntax},
	{"ChangeHole", pcb_act_ChangeHole, changehold_help, changehold_syntax},
	{"ChangeJoin", pcb_act_ChangeJoin, pcb_acth_ChangeJoin, pcb_acts_ChangeJoin},
	{"ChangeName", pcb_act_ChangeName, pcb_acth_ChangeName, pcb_acts_ChangeName},
	{"ChangePaste", pcb_act_ChangePaste, pcb_acth_ChangePaste, pcb_acts_ChangePaste},
	{"ChangePinName", pcb_act_ChangePinName, pcb_acth_ChangePinName, pcb_acts_ChangePinName},
	{"ChangeRadius", pcb_act_ChangeRadius, pcb_acth_ChangeRadius, pcb_acts_ChangeRadius},
	{"ChangeSize", pcb_act_ChangeSize, pcb_acth_ChangeSize, pcb_acts_ChangeSize},
	{"ChangeSizes", pcb_act_ChangeSizes, pcb_acth_ChangeSizes, pcb_acts_ChangeSizes},
	{"ChangeNonetlist", pcb_act_ChangeNonetlist, pcb_acth_ChangeNonetlist, pcb_acts_ChangeNonetlist},
	{"ChangeSquare", pcb_act_ChangeSquare, pcb_acth_ChangeSquare, pcb_acts_ChangeSquare},
	{"ChangeOctagon", pcb_act_ChangeOctagon, pcb_acth_ChangeOctagon, pcb_acts_ChangeOctagon},
	{"ChangeFlag", pcb_act_ChangeFlag, pcb_acth_ChangeFlag, pcb_acts_ChangeFlag},
	{"ClearSquare", pcb_act_ClearSquare, pcb_acth_ClearSquare, pcb_acts_ClearSquare},
	{"ClearOctagon", pcb_act_ClearOctagon, pcb_acth_ClearOctagon, pcb_acts_ClearOctagon},
	{"SetSquare", pcb_act_SetSquare, pcb_acth_SetSquare, pcb_acts_SetSquare},
	{"SetOctagon", pcb_act_SetOctagon, pcb_acth_SetOctagon, pcb_acts_SetOctagon},
	{"SetThermal", pcb_act_SetThermal, pcb_acth_SetThermal, pcb_acts_SetThermal},
	{"SetValue", pcb_act_SetValue, pcb_acth_SetValue, pcb_acts_SetValue},
	{"SetFlag", pcb_act_SetFlag, pcb_acth_SetFlag, pcb_acts_SetFlag},
	{"ClrFlag", pcb_act_ClrFlag, pcb_acth_ClrFlag, pcb_acts_ClrFlag}
};

PCB_REGISTER_ACTIONS(change_action_list, NULL)
