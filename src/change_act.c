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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
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
#include "route_style.h"

static void ChangeFlag(const char *, const char *, int, const char *);
static fgw_error_t pcb_act_ChangeSize(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);
static fgw_error_t pcb_act_Change2ndSize(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeClearSize[] =
	"ChangeClearSize(Object, delta|style)\n"
	"ChangeClearSize(SelectedPins|SelectedPads|SelectedVias, delta|style)\n"
	"ChangeClearSize(SelectedLines|SelectedArcs, delta|style)\n"
	"ChangeClearSize(Selected|SelectedObjects, delta|style)";
static const char pcb_acth_ChangeClearSize[] = "Changes the clearance size of objects.";
/* DOC: changeclearsize.html */
static fgw_error_t pcb_act_ChangeClearSize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function;
	const char *delta = NULL;
	const char *units = NULL;
	pcb_bool absolute;
	pcb_coord_t value;
	int type = PCB_OBJ_VOID;
	void *ptr1, *ptr2, *ptr3;
	pcb_coord_t x, y;
	int got_coords = 0;

	PCB_ACT_CONVARG(1, FGW_STR, ChangeClearSize, function = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, ChangeClearSize, delta = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, ChangeClearSize, units = argv[3].val.str);

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			pcb_hid_get_coords("Select an Object", &x, &y, 0);
			got_coords = 1;
			type = pcb_search_screen(x, y, PCB_CHANGECLEARSIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		if (strcmp(delta, "style") == 0) {
			if (!got_coords) {
				pcb_hid_get_coords("Select an Object", &x, &y, 0);
				got_coords = 1;
			}

			if ((type == PCB_OBJ_VOID) || (type == PCB_OBJ_POLY))	/* workaround: pcb_search_screen(PCB_CHANGECLEARSIZE_TYPES) wouldn't return elements */
				type = pcb_search_screen(x, y, PCB_CHANGE2NDSIZE_TYPES, &ptr1, &ptr2, &ptr3);
			if (pcb_get_style_size(funcid, &value, type, 2) != 0) {
				PCB_ACT_IRES(-1);
				return 0;
			}
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
	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeFlag[] =
	"ChangeFlag(Object|Selected|SelectedObjects, flag, value)\n"
	"ChangeFlag(SelectedLines|SelectedPins|SelectedVias, flag, value)\n"
	"ChangeFlag(SelectedPads|SelectedTexts|SelectedNames, flag, value)\n"
	"ChangeFlag(SelectedElements, flag, value)\n" "flag = thermal | join\n" "value = 0 | 1";
static const char pcb_acth_ChangeFlag[] = "Sets or clears flags on objects.";
/* DOC: changeflag.html */
static fgw_error_t pcb_act_ChangeFlag(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function;
	const char *flag;
	int value;

	PCB_ACT_CONVARG(1, FGW_STR, ChangeFlag, function = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, ChangeFlag, flag = argv[2].val.str);
	PCB_ACT_CONVARG(2, FGW_INT, ChangeFlag, value = argv[3].val.nat_int);

	if (value != 0 && value != 1)
		PCB_ACT_FAIL(ChangeFlag);

	ChangeFlag(function, flag, value, "ChangeFlag");
	PCB_ACT_IRES(0);
	return 0;
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
		pcb_message(PCB_MSG_ERROR, "%s():  Flag \"%s\" is not valid\n", cmd_name, flag_name);
		return;
	}

	switch (pcb_funchash_get(what, NULL)) {
	case F_Object:
		{
			int type;
			void *ptr1, *ptr2, *ptr3;
			pcb_coord_t x, y;

			pcb_hid_get_coords("Click on object to change", &x, &y, 0);

			if ((type = pcb_search_screen(x, y, PCB_CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID)
				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2))
					pcb_message(PCB_MSG_WARNING, "Sorry, the object is locked\n");
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

static const char pcb_acts_ChangeSizes[] =
	"ChangeSizes(Object, delta|style)\n"
	"ChangeSizes(SelectedObjects|Selected, delta|style)\n"
	"ChangeSizes(SelectedLines|SelectedPins|SelectedVias, delta|style)\n"
	"ChangeSizes(SelectedPads|SelectedTexts|SelectedNames, delta|style)\n" "ChangeSizes(SelectedElements, delta|style)";
static const char pcb_acth_ChangeSizes[] = "Changes all sizes of objects.";
/* DOC: changesizes.html */
static fgw_error_t pcb_act_ChangeSizes(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	fgw_error_t a, b, c;
	pcb_undo_save_serial();
	a = pcb_act_ChangeSize(res, argc, argv);
	pcb_undo_restore_serial();
	b = pcb_act_Change2ndSize(res, argc, argv);
	pcb_undo_restore_serial();
	c = pcb_act_ChangeClearSize(res, argc, argv);
	pcb_undo_restore_serial();
	pcb_undo_inc_serial();
	PCB_ACT_IRES(!(!a || !b || !c));
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeSize[] =
	"ChangeSize(Object, delta|style)\n"
	"ChangeSize(SelectedObjects|Selected, delta|style)\n"
	"ChangeSize(SelectedLines|SelectedPins|SelectedVias, delta|style)\n"
	"ChangeSize(SelectedPads|SelectedTexts|SelectedNames, delta|style)\n" "ChangeSize(SelectedElements, delta|style)";
static const char pcb_acth_ChangeSize[] = "Changes the size of objects.";
/* DOC: changesize.html */
static fgw_error_t pcb_act_ChangeSize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function;
	const char *delta;
	const char *units = NULL;
	pcb_bool absolute;								/* indicates if absolute size is given */
	pcb_coord_t value;
	int type = PCB_OBJ_VOID, tostyle = 0;
	void *ptr1, *ptr2, *ptr3;

	PCB_ACT_CONVARG(1, FGW_STR, ChangeSize, function = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, ChangeSize, delta = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, ChangeSize, units = argv[3].val.str);


	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			pcb_coord_t x, y;
			pcb_hid_get_coords("Click on object to change size of", &x, &y, 0);
			type = pcb_search_screen(x, y, PCB_CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		if (strcmp(delta, "style") == 0) {
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
						pcb_message(PCB_MSG_WARNING, "Sorry, the object is locked\n");
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

	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Change2ndSize[] =
	"ChangeDrillSize(Object, delta|style)\n" "ChangeDrillSize(SelectedPins|SelectedVias|Selected|SelectedObjects, delta|style)";

static const char pcb_acth_Change2ndSize[] = "Changes the drilling hole size of objects.";

static fgw_error_t pcb_act_Change2ndSize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function;
	const char *delta;
	const char *units = NULL;
	int type = PCB_OBJ_VOID;
	void *ptr1, *ptr2, *ptr3;
	pcb_bool absolute;
	pcb_coord_t value;

	PCB_ACT_CONVARG(1, FGW_STR, Change2ndSize, function = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, Change2ndSize, delta = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, Change2ndSize, units = argv[3].val.str);

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			pcb_coord_t x, y;
			pcb_hid_get_coords("Select an Object", &x, &y, 0);
			type = pcb_search_screen(x, y, PCB_CHANGE2NDSIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		if (strcmp(delta, "style") == 0) {
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
	PCB_ACT_IRES(0);
	return 0;
}

/* ---------------------------------------------------------------------------  */

static const char pcb_acts_ChangePinName[] = "ChangePinName(Refdes,PinNumber,PinName)";
static const char pcb_acth_ChangePinName[] = "Sets the name of a specific pin on a specific subcircuit.";
/* DOC: changepinname.html */
static fgw_error_t pcb_act_ChangePinName(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_cardinal_t changed = 0;
	const char *refdes, *pinnum, *pinname;

	PCB_ACT_CONVARG(1, FGW_STR, ChangePinName, refdes = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, ChangePinName, pinnum = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_STR, ChangePinName, pinname = argv[3].val.str);

	PCB_SUBC_LOOP(PCB->Data);
	{
		if ((subc->refdes != NULL) && (PCB_NSTRCMP(refdes, subc->refdes) == 0)) {
			pcb_any_obj_t *o;
			pcb_data_it_t it;

			for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
				if ((o->term != NULL) && (PCB_NSTRCMP(pinnum, o->term) == 0)) {
TODO(": make this undoable")
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

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ChangeName[] = "ChangeName(Object)\n" "ChangeName(Refdes)\n" "ChangeName(Layout|Layer)";
static const char pcb_acth_ChangeName[] = "Sets the name, text string, terminal ID or refdes of objects.";
/* DOC: changename.html */
static fgw_error_t pcb_act_ChangeName(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	char *name;
	pcb_objtype_t type;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, ChangeName, op = fgw_keyword(&argv[1]));

	switch(op) {
		/* change the refdes of a subcircuit */
		case F_Subc:
		{
			pcb_coord_t x, y;
			pcb_hid_get_coords("Select a subcircuit", &x, &y, 0);
			type = PCB_OBJ_SUBC;
			goto do_chg_name;
		}
			/* change the name of an object */
		case F_Object:
		case F_Refdes:
			{
				pcb_coord_t x, y;
				void *ptr1, *ptr2, *ptr3;
				pcb_hid_get_coords("Select an Object", &x, &y, 0);
				type = op == F_Refdes ? PCB_OBJ_SUBC : PCB_CHANGENAME_TYPES;
				do_chg_name:;
				if ((type = pcb_search_screen(x, y, type, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
					pcb_undo_save_serial();
					if (pcb_chg_obj_name_query(ptr2)) {
						pcb_redraw();
						pcb_board_set_changed_flag(pcb_true);
						pcb_actionl("DeleteRats", "AllRats", NULL);
					}
				}
				break;
			}

			/* change the layout's name */
		case F_Layout:
			name = pcb_hid_prompt_for("Enter the layout name:", PCB_EMPTY(PCB->Name), "Layout name");
			/* NB: ChangeLayoutName takes ownership of the passed memory */
			if (name && pcb_board_change_name(name))
				pcb_board_set_changed_flag(pcb_true);
			break;

			/* change the name of the active layer */
		case F_Layer:
			name = pcb_hid_prompt_for("Enter the layer name:", PCB_EMPTY(CURRENT->name), "Layer name");
			/* NB: pcb_layer_rename_ takes ownership of the passed memory */
			if (name && (pcb_layer_rename_(CURRENT, name) == 0))
				pcb_board_set_changed_flag(pcb_true);
			else
				free(name);
			break;

		default:
			return FGW_ERR_ARG_CONV;
	}

	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeJoin[] = "ChangeJoin(ToggleObject|SelectedLines|SelectedArcs|Selected)";

static const char pcb_acth_ChangeJoin[] = "Changes the join (clearance through polygons) of objects.";

/* DOC: changejoin.html */

static fgw_error_t pcb_act_ChangeJoin(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, ChangeJoin, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_ToggleObject:
		case F_Object:
			{
				pcb_coord_t x, y;
				int type;
				void *ptr1, *ptr2, *ptr3;

				pcb_hid_get_coords("Select an Object", &x, &y, 0);
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

		default:
			return FGW_ERR_ARG_CONV;
	}

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ChangeNonetlist[] =
	"ChangeNonetlist(ToggleObject)\n" "ChangeNonetlist(SelectedElements)\n" "ChangeNonetlist(Selected|SelectedObjects)";
static const char pcb_acth_ChangeNonetlist[] = "Changes the nonetlist flag of subcircuits.";
static fgw_error_t pcb_act_ChangeNonetlist(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, ChangeJoin, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_ToggleObject:
		case F_Object:
		case F_Element:
			{
				pcb_coord_t x, y;
				int type;
				void *ptr1, *ptr2, *ptr3;
				pcb_hid_get_coords("Select an Element", &x, &y, 0);

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

		default:
			return FGW_ERR_ARG_CONV;
	}

	PCB_ACT_IRES(0);
	return 0;
}


static const char pcb_acts_SetThermal[] = "SetThermal(Object|SelectedPins|SelectedVias|Selected, Style)";
static const char pcb_acth_SetThermal[] =
	"Set the thermal (on the current layer) of padstacks to the given style.\n"
	"Style = 0 means no thermal.\n"
	"Style = 1 has diagonal fingers with sharp edges.\n"
	"Style = 2 has horizontal and vertical fingers with sharp edges.\n"
	"Style = 3 is a solid connection to the plane.\n"
	"Style = 4 has diagonal fingers with rounded edges.\n"
	"Style = 5 has horizontal and vertical fingers with rounded edges.\n";
/* DOC: setthermal.html */
static fgw_error_t pcb_act_SetThermal(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function;
	const char *style;
	void *ptr1, *ptr2, *ptr3;
	int type, kind;
	int err = 0;
	pcb_coord_t gx, gy;

	PCB_ACT_CONVARG(1, FGW_STR, SetThermal, function = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, SetThermal, style = argv[2].val.str);

	if (function && *function && style && *style) {
		pcb_bool absolute;

		kind = pcb_get_value_ex(style, NULL, &absolute, NULL, NULL, NULL);
		if (absolute && (kind <= 5))
			switch (pcb_funchash_get(function, NULL)) {
			case F_Object:
				pcb_hid_get_coords("Click on object for SetThermal", &gx, &gy, 0);
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
		if (!err) {
			PCB_ACT_IRES(0);
			return 0;
		}
	}

	PCB_ACT_FAIL(SetThermal);
}


/* --------------------------------------------------------------------------- */

static const char pcb_acts_SetFlag[] =
	"SetFlag(Object|Selected|SelectedObjects, flag)\n"
	"SetFlag(SelectedLines|SelectedPins|SelectedVias, flag)\n"
	"SetFlag(SelectedPads|SelectedTexts|SelectedNames, flag)\n"
	"SetFlag(SelectedElements, flag)\n" "flag = thermal | join";
static const char pcb_acth_SetFlag[] = "Sets flags on objects.";
/* DOC: setflag.html */
static fgw_error_t pcb_act_SetFlag(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function;
	const char *flag;

	PCB_ACT_CONVARG(1, FGW_STR, SetFlag, function = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, SetFlag, flag = argv[2].val.str);

	ChangeFlag(function, flag, 1, "SetFlag");

	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ClrFlag[] =
	"ClrFlag(Object|Selected|SelectedObjects, flag)\n"
	"ClrFlag(SelectedLines|SelectedPins|SelectedVias, flag)\n"
	"ClrFlag(SelectedPads|SelectedTexts|SelectedNames, flag)\n"
	"ClrFlag(SelectedElements, flag)\n" "flag = thermal | join";
static const char pcb_acth_ClrFlag[] = "Clears flags on objects.";
/* DOC: clrflag.html */
static fgw_error_t pcb_act_ClrFlag(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function;
	const char *flag;

	PCB_ACT_CONVARG(1, FGW_STR, SetFlag, function = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, SetFlag, flag = argv[2].val.str);

	ChangeFlag(function, flag, 0, "ClrFlag");

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_SetValue[] = "SetValue(Grid|Line|LineSize|Text|TextScale, delta)";
static const char pcb_acth_SetValue[] = "Change various board-wide values and sizes.";
/* DOC: setvalue.html */
static fgw_error_t pcb_act_SetValue(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int fnc_id;
	const char *val;
	const char *units = NULL;
	pcb_bool absolute;								/* flag for 'absolute' value */
	double value;
	int err = 0;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, SetValue, fnc_id = fgw_keyword(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_STR, SetValue, val = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, SetValue, units = argv[3].val.str);

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

	switch(fnc_id) {
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
	if (!err) {
		PCB_ACT_IRES(0);
		return 0;
	}

	PCB_ACT_FAIL(SetValue);
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeAngle[] =
	"ChangeAngle(Object, start|delta|both, delta)\n"
	"ChangeAngle(SelectedObjects|Selected, start|delta|both, delta)\n"
	"ChangeAngle(SelectedArcs, start|delta|both, delta)\n";
static const char pcb_acth_ChangeAngle[] = "Changes the start angle, delta angle or both angles of an arc.";
static fgw_error_t pcb_act_ChangeAngle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *prim;
	const char *delta;
	pcb_bool absolute;								/* indicates if absolute size is given */
	double value;
	int funcid, type = PCB_OBJ_VOID, which;
	void *ptr1, *ptr2, *ptr3;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, ChangeAngle, funcid = fgw_keyword(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_STR, ChangeAngle, prim = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_STR, ChangeAngle, delta = argv[3].val.str);


	if (pcb_strcasecmp(prim, "start") == 0) which = 0;
	else if (pcb_strcasecmp(prim, "delta") == 0) which = 1;
	else if (pcb_strcasecmp(prim, "both") == 0) which = 2;
	else {
		pcb_message(PCB_MSG_ERROR, "Second argument of ChangeAngle must be start, delta or both\n");
		return -1;
	}

	if (funcid == F_Object) {
		pcb_coord_t x, y;
		pcb_hid_get_coords("Click on object to change angle of", &x, &y, 0);
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
	
	switch(funcid) {
		case F_Object:
			{
				if (type != PCB_OBJ_VOID) {
					if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2))
						pcb_message(PCB_MSG_WARNING, "Sorry, the object is locked\n");
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

	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_ChangeRadius[] =
	"ChangeRadius(Object, width|x|height|y|both, delta)\n"
	"ChangeRadius(SelectedObjects|Selected, width|x|height|y|both, delta)\n"
	"ChangeRadius(SelectedArcs, width|x|height|y|both, delta)\n";
static const char pcb_acth_ChangeRadius[] = "Changes the width or height (radius) of an arc.";
static fgw_error_t pcb_act_ChangeRadius(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *prim;
	const char *delta;
	const char *units;
	pcb_bool absolute;								/* indicates if absolute size is given */
	double value;
	int funcid, type = PCB_OBJ_VOID, which;
	void *ptr1, *ptr2, *ptr3;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, ChangeRadius, funcid = fgw_keyword(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_STR, ChangeRadius, prim = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_STR, ChangeRadius, delta = argv[3].val.str);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, ChangeRadius, units = argv[4].val.str);

	if ((pcb_strcasecmp(prim, "width") == 0) || (pcb_strcasecmp(prim, "x") == 0)) which = 0;
	else if ((pcb_strcasecmp(prim, "height") == 0) || (pcb_strcasecmp(prim, "y") == 0)) which = 1;
	else if (pcb_strcasecmp(prim, "both") == 0) which = 2;
	else {
		pcb_message(PCB_MSG_ERROR, "Second argument of ChangeRadius must be width, x, height, y or both\n");
		return -1;
	}

	if (funcid == F_Object) {
		pcb_coord_t x, y;
		pcb_hid_get_coords("Click on object to change radius of", &x, &y, 0);
		type = pcb_search_screen(x, y, PCB_CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3);
	}

	value = pcb_get_value(delta, units, &absolute, NULL);

	switch(funcid) {
		case F_Object:
			{
				if (type != PCB_OBJ_VOID) {
					if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2))
						pcb_message(PCB_MSG_WARNING, "Sorry, the object is locked\n");
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

	PCB_ACT_IRES(0);
	return 0;
}


/* --------------------------------------------------------------------------- */

pcb_action_t change_action_list[] = {
	{"ChangeAngle", pcb_act_ChangeAngle, pcb_acth_ChangeAngle, pcb_acts_ChangeAngle},
	{"ChangeClearSize", pcb_act_ChangeClearSize, pcb_acth_ChangeClearSize, pcb_acts_ChangeClearSize},
	{"ChangeDrillSize", pcb_act_Change2ndSize, pcb_acth_Change2ndSize, pcb_acts_Change2ndSize},
	{"ChangeJoin", pcb_act_ChangeJoin, pcb_acth_ChangeJoin, pcb_acts_ChangeJoin},
	{"ChangeName", pcb_act_ChangeName, pcb_acth_ChangeName, pcb_acts_ChangeName},
	{"ChangePinName", pcb_act_ChangePinName, pcb_acth_ChangePinName, pcb_acts_ChangePinName},
	{"ChangeRadius", pcb_act_ChangeRadius, pcb_acth_ChangeRadius, pcb_acts_ChangeRadius},
	{"ChangeSize", pcb_act_ChangeSize, pcb_acth_ChangeSize, pcb_acts_ChangeSize},
	{"ChangeSizes", pcb_act_ChangeSizes, pcb_acth_ChangeSizes, pcb_acts_ChangeSizes},
	{"ChangeNonetlist", pcb_act_ChangeNonetlist, pcb_acth_ChangeNonetlist, pcb_acts_ChangeNonetlist},
	{"ChangeFlag", pcb_act_ChangeFlag, pcb_acth_ChangeFlag, pcb_acts_ChangeFlag},
	{"SetThermal", pcb_act_SetThermal, pcb_acth_SetThermal, pcb_acts_SetThermal},
	{"SetValue", pcb_act_SetValue, pcb_acth_SetValue, pcb_acts_SetValue},
	{"SetFlag", pcb_act_SetFlag, pcb_acth_SetFlag, pcb_acts_SetFlag},
	{"ClrFlag", pcb_act_ClrFlag, pcb_acth_ClrFlag, pcb_acts_ClrFlag}
};

PCB_REGISTER_ACTIONS(change_action_list, NULL)
