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

#include "board.h"
#include "data.h"
#include "action_helper.h"
#include "error.h"
#include "undo.h"
#include "funchash_core.h"

#include "select.h"
#include "set.h"
#include "draw.h"
#include "remove.h"
#include "copy.h"
#include "hid_attrib.h"
#include "compat_misc.h"
#include "compat_nls.h"

/* --------------------------------------------------------------------------- */
/* Ask the user for a search pattern */
static char *gui_get_pat(pcb_search_method_t * method)
{
	const char *methods[] = { "regexp", "list of names", NULL };
	pcb_hid_attribute_t attrs[2];
#define nattr sizeof(attrs)/sizeof(attrs[0])
	static pcb_hid_attr_val_t results[nattr] = { 0 };

	memset(attrs, 0, sizeof(attrs));

	attrs[0].name = "Pattern";
	attrs[0].help_text = "Name/refdes pattern";
	attrs[0].type = HID_String;
	attrs[0].default_val.str_value = results[0].str_value;

	attrs[1].name = "Method";
	attrs[1].help_text = "method of search: either regular expression or a list of full names separated by |";
	attrs[1].type = HID_Enum;
	attrs[1].enumerations = methods;
	attrs[1].default_val.int_value = results[1].int_value;

	gui->attribute_dialog(attrs, nattr, results, "Find element", "Find element by name");

	*method = results[1].int_value;
	if (results[0].str_value == NULL)
		return NULL;
	return pcb_strdup(results[0].str_value);
#undef nattr
}


/* --------------------------------------------------------------------------- */

static const char select_syntax[] =
	"Select(Object|ToggleObject)\n"
	"Select(All|Block|Connection)\n"
	"Select(ElementByName|ObjectByName|PadByName|PinByName)\n"
	"Select(ElementByName|ObjectByName|PadByName|PinByName, Name)\n"
	"Select(TextByName|ViaByName|NetByName)\n" "Select(TextByName|ViaByName|NetByName, Name)\n" "Select(Convert)";

static const char select_help[] = "Toggles or sets the selection.";

/* %start-doc actions Select

@table @code

@item ElementByName
@item ObjectByName
@item PadByName
@item PinByName
@item TextByName
@item ViaByName
@item NetByName

These all rely on having a regular expression parser built into
@code{pcb}.  If the name is not specified then the user is prompted
for a pattern, and all objects that match the pattern and are of the
type specified are selected.

@item Object
@item ToggleObject
Selects the object under the cursor.

@item Block
Selects all objects in a rectangle indicated by the cursor.

@item All
Selects all objects on the board.

@item Connection
Selects all connections with the ``found'' flag set.

@item Convert
Converts the selected objects to an element.  This uses the highest
numbered paste buffer.

@end table

%end-doc */

static int ActionSelect(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		int type;

		switch (funchash_get(function, NULL)) { /* select objects by their names */
		case F_ElementByName:
			type = PCB_TYPE_ELEMENT;
			goto commonByName;
		case F_ObjectByName:
			type = PCB_TYPEMASK_ALL;
			goto commonByName;
		case F_PadByName:
			type = PCB_TYPE_PAD;
			goto commonByName;
		case F_PinByName:
			type = PCB_TYPE_PIN;
			goto commonByName;
		case F_TextByName:
			type = PCB_TYPE_TEXT;
			goto commonByName;
		case F_ViaByName:
			type = PCB_TYPE_VIA;
			goto commonByName;
		case F_NetByName:
			type = PCB_TYPE_NET;
			goto commonByName;

		commonByName:
			{
				const char *pattern = PCB_ACTION_ARG(1);
				pcb_search_method_t method = SM_REGEX;

				if (pattern || (pattern = gui_get_pat(&method)) != NULL) {
					if (SelectObjectByName(type, pattern, pcb_true, method))
						SetChangedFlag(pcb_true);
					if (PCB_ACTION_ARG(1) == NULL)
						free((char*)pattern);
				}
				break;
			}

			/* select a single object */
		case F_ToggleObject:
		case F_Object:
			if (SelectObject())
				SetChangedFlag(pcb_true);
			break;

			/* all objects in block */
		case F_Block:
			{
				pcb_box_t box;

				box.X1 = MIN(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
				box.Y1 = MIN(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
				box.X2 = MAX(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
				box.Y2 = MAX(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
				notify_crosshair_change(pcb_false);
				pcb_notify_block();
				if (Crosshair.AttachedBox.State == STATE_THIRD && SelectBlock(&box, pcb_true)) {
					SetChangedFlag(pcb_true);
					Crosshair.AttachedBox.State = STATE_FIRST;
				}
				notify_crosshair_change(pcb_true);
				break;
			}

			/* select all visible objects */
		case F_All:
			{
				pcb_box_t box;

				box.X1 = -MAX_COORD;
				box.Y1 = -MAX_COORD;
				box.X2 = MAX_COORD;
				box.Y2 = MAX_COORD;
				if (SelectBlock(&box, pcb_true))
					SetChangedFlag(pcb_true);
				break;
			}

			/* all found connections */
		case F_Connection:
			if (SelectConnection(pcb_true)) {
				Draw();
				IncrementUndoSerialNumber();
				SetChangedFlag(pcb_true);
			}
			break;

		case F_Convert:
			{
				pcb_coord_t x, y;
				Note.Buffer = conf_core.editor.buffer_number;
				SetBufferNumber(MAX_BUFFER - 1);
				pcb_buffer_clear(PCB_PASTEBUFFER);
				gui->get_coords(_("Select the Element's Mark Location"), &x, &y);
				x = GridFit(x, PCB->Grid, PCB->GridOffsetX);
				y = GridFit(y, PCB->Grid, PCB->GridOffsetY);
				pcb_buffer_add_selected(PCB_PASTEBUFFER, x, y, pcb_true);
				SaveUndoSerialNumber();
				RemoveSelected();
				ConvertBufferToElement(PCB_PASTEBUFFER);
				RestoreUndoSerialNumber();
				pcb_buffer_copy_to_layout(x, y);
				SetBufferNumber(Note.Buffer);
			}
			break;

		default:
			PCB_AFAIL(select);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char unselect_syntax[] =
	"Unselect(All|Block|Connection)\n"
	"Unselect(ElementByName|ObjectByName|PadByName|PinByName)\n"
	"Unselect(ElementByName|ObjectByName|PadByName|PinByName, Name)\n"
	"Unselect(TextByName|ViaByName)\n" "Unselect(TextByName|ViaByName, Name)\n";

static const char unselect_help[] = "Unselects the object at the pointer location or the specified objects.";

/* %start-doc actions Unselect

@table @code

@item All
Unselect all objects.

@item Block
Unselect all objects in a rectangle given by the cursor.

@item Connection
Unselect all connections with the ``found'' flag set.

@item ElementByName
@item ObjectByName
@item PadByName
@item PinByName
@item TextByName
@item ViaByName

These all rely on having a regular expression parser built into
@code{pcb}.  If the name is not specified then the user is prompted
for a pattern, and all objects that match the pattern and are of the
type specified are unselected.


@end table

%end-doc */

static int ActionUnselect(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		int type;
		switch (funchash_get(function, NULL)) {
			/* select objects by their names */
		case F_ElementByName:
			type = PCB_TYPE_ELEMENT;
			goto commonByName;
		case F_ObjectByName:
			type = PCB_TYPEMASK_ALL;
			goto commonByName;
		case F_PadByName:
			type = PCB_TYPE_PAD;
			goto commonByName;
		case F_PinByName:
			type = PCB_TYPE_PIN;
			goto commonByName;
		case F_TextByName:
			type = PCB_TYPE_TEXT;
			goto commonByName;
		case F_ViaByName:
			type = PCB_TYPE_VIA;
			goto commonByName;
		case F_NetByName:
			type = PCB_TYPE_NET;
			goto commonByName;

		commonByName:
			{
				const char *pattern = PCB_ACTION_ARG(1);
				pcb_search_method_t method = SM_REGEX;

				if (pattern || (pattern = gui_get_pat(&method)) != NULL) {
					if (SelectObjectByName(type, pattern, pcb_false, method))
						SetChangedFlag(pcb_true);
					if (PCB_ACTION_ARG(1) == NULL)
						free((char*)pattern);
				}
				break;
			}

			/* all objects in block */
		case F_Block:
			{
				pcb_box_t box;

				box.X1 = MIN(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
				box.Y1 = MIN(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
				box.X2 = MAX(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
				box.Y2 = MAX(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
				notify_crosshair_change(pcb_false);
				pcb_notify_block();
				if (Crosshair.AttachedBox.State == STATE_THIRD && SelectBlock(&box, pcb_false)) {
					SetChangedFlag(pcb_true);
					Crosshair.AttachedBox.State = STATE_FIRST;
				}
				notify_crosshair_change(pcb_true);
				break;
			}

			/* unselect all visible objects */
		case F_All:
			{
				pcb_box_t box;

				box.X1 = -MAX_COORD;
				box.Y1 = -MAX_COORD;
				box.X2 = MAX_COORD;
				box.Y2 = MAX_COORD;
				if (SelectBlock(&box, pcb_false))
					SetChangedFlag(pcb_true);
				break;
			}

			/* all found connections */
		case F_Connection:
			if (SelectConnection(pcb_false)) {
				Draw();
				IncrementUndoSerialNumber();
				SetChangedFlag(pcb_true);
			}
			break;

		default:
			PCB_AFAIL(unselect);
			break;

		}
	}
	return 0;
}

pcb_hid_action_t select_action_list[] = {
	{"Select", 0, ActionSelect,
	 select_help, select_syntax}
	,
	{"Unselect", 0, ActionUnselect,
	 unselect_help, unselect_syntax}
};

REGISTER_ACTIONS(select_action_list, NULL)
