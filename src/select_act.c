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
#include "global.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "funchash_core.h"

#include "search.h"
#include "select.h"
#include "crosshair.h"
#include "set.h"
#include "buffer.h"
#include "draw.h"
#include "remove.h"
#include "copy.h"

/* --------------------------------------------------------------------------- */
/* Ask the user for a search pattern */
static char *gui_get_pat(search_method_t * method)
{
	const char *methods[] = { "regexp", "list of names", NULL };
	HID_Attribute attrs[] = {
		{.name = "Pattern",.help_text = "Name/refdes pattern",.type = HID_String},
		{.name = "Method",.help_text = "method of search: either regular expression or a list of full names separated by |",.type =
		 HID_Enum,.enumerations = methods}
	};
#define nattr sizeof(attrs)/sizeof(attrs[0])
	static HID_Attr_Val results[nattr] = { 0 };

	attrs[0].default_val.str_value = results[0].str_value;
	attrs[1].default_val.int_value = results[1].int_value;

	gui->attribute_dialog(attrs, nattr, results, "Find element", "Find element by name");

	*method = results[1].int_value;
	return strdup(results[0].str_value);
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

static int ActionSelect(int argc, char **argv, Coord x, Coord y)
{
	char *function = ACTION_ARG(0);
	if (function) {
		switch (funchash_get(function, NULL)) {
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
			int type;
			/* select objects by their names */
		case F_ElementByName:
			type = ELEMENT_TYPE;
			goto commonByName;
		case F_ObjectByName:
			type = ALL_TYPES;
			goto commonByName;
		case F_PadByName:
			type = PAD_TYPE;
			goto commonByName;
		case F_PinByName:
			type = PIN_TYPE;
			goto commonByName;
		case F_TextByName:
			type = TEXT_TYPE;
			goto commonByName;
		case F_ViaByName:
			type = VIA_TYPE;
			goto commonByName;
		case F_NetByName:
			type = NET_TYPE;
			goto commonByName;

		commonByName:
			{
				char *pattern = ACTION_ARG(1);
				search_method_t method;

				if (pattern || (pattern = gui_get_pat(&method)) != NULL) {
					if (SelectObjectByName(type, pattern, true, method))
						SetChangedFlag(true);
					if (ACTION_ARG(1) == NULL)
						free(pattern);
				}
				break;
			}
#endif /* defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP) */

			/* select a single object */
		case F_ToggleObject:
		case F_Object:
			if (SelectObject())
				SetChangedFlag(true);
			break;

			/* all objects in block */
		case F_Block:
			{
				BoxType box;

				box.X1 = MIN(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
				box.Y1 = MIN(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
				box.X2 = MAX(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
				box.Y2 = MAX(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
				notify_crosshair_change(false);
				NotifyBlock();
				if (Crosshair.AttachedBox.State == STATE_THIRD && SelectBlock(&box, true)) {
					SetChangedFlag(true);
					Crosshair.AttachedBox.State = STATE_FIRST;
				}
				notify_crosshair_change(true);
				break;
			}

			/* select all visible objects */
		case F_All:
			{
				BoxType box;

				box.X1 = -MAX_COORD;
				box.Y1 = -MAX_COORD;
				box.X2 = MAX_COORD;
				box.Y2 = MAX_COORD;
				if (SelectBlock(&box, true))
					SetChangedFlag(true);
				break;
			}

			/* all found connections */
		case F_Connection:
			if (SelectConnection(true)) {
				Draw();
				IncrementUndoSerialNumber();
				SetChangedFlag(true);
			}
			break;

		case F_Convert:
			{
				Coord x, y;
				Note.Buffer = Settings.BufferNumber;
				SetBufferNumber(MAX_BUFFER - 1);
				ClearBuffer(PASTEBUFFER);
				gui->get_coords(_("Select the Element's Mark Location"), &x, &y);
				x = GridFit(x, PCB->Grid, PCB->GridOffsetX);
				y = GridFit(y, PCB->Grid, PCB->GridOffsetY);
				AddSelectedToBuffer(PASTEBUFFER, x, y, true);
				SaveUndoSerialNumber();
				RemoveSelected();
				ConvertBufferToElement(PASTEBUFFER);
				RestoreUndoSerialNumber();
				CopyPastebufferToLayout(x, y);
				SetBufferNumber(Note.Buffer);
			}
			break;

		default:
			AFAIL(select);
			break;
		}
	}
	return 0;
}

/* FLAG(have_regex,FlagHaveRegex,0) */
int FlagHaveRegex(int parm)
{
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
	return 1;
#else
	return 0;
#endif
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

static int ActionUnselect(int argc, char **argv, Coord x, Coord y)
{
	char *function = ACTION_ARG(0);
	if (function) {
		switch (funchash_get(function, NULL)) {
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
			int type;
			/* select objects by their names */
		case F_ElementByName:
			type = ELEMENT_TYPE;
			goto commonByName;
		case F_ObjectByName:
			type = ALL_TYPES;
			goto commonByName;
		case F_PadByName:
			type = PAD_TYPE;
			goto commonByName;
		case F_PinByName:
			type = PIN_TYPE;
			goto commonByName;
		case F_TextByName:
			type = TEXT_TYPE;
			goto commonByName;
		case F_ViaByName:
			type = VIA_TYPE;
			goto commonByName;
		case F_NetByName:
			type = NET_TYPE;
			goto commonByName;

		commonByName:
			{
				char *pattern = ACTION_ARG(1);
				search_method_t method;

				if (pattern || (pattern = gui_get_pat(&method)) != NULL) {
					if (SelectObjectByName(type, pattern, false, method))
						SetChangedFlag(true);
					if (ACTION_ARG(1) == NULL)
						free(pattern);
				}
				break;
			}
#endif /* defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP) */

			/* all objects in block */
		case F_Block:
			{
				BoxType box;

				box.X1 = MIN(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
				box.Y1 = MIN(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
				box.X2 = MAX(Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point2.X);
				box.Y2 = MAX(Crosshair.AttachedBox.Point1.Y, Crosshair.AttachedBox.Point2.Y);
				notify_crosshair_change(false);
				NotifyBlock();
				if (Crosshair.AttachedBox.State == STATE_THIRD && SelectBlock(&box, false)) {
					SetChangedFlag(true);
					Crosshair.AttachedBox.State = STATE_FIRST;
				}
				notify_crosshair_change(true);
				break;
			}

			/* unselect all visible objects */
		case F_All:
			{
				BoxType box;

				box.X1 = -MAX_COORD;
				box.Y1 = -MAX_COORD;
				box.X2 = MAX_COORD;
				box.Y2 = MAX_COORD;
				if (SelectBlock(&box, false))
					SetChangedFlag(true);
				break;
			}

			/* all found connections */
		case F_Connection:
			if (SelectConnection(false)) {
				Draw();
				IncrementUndoSerialNumber();
				SetChangedFlag(true);
			}
			break;

		default:
			AFAIL(unselect);
			break;

		}
	}
	return 0;
}

HID_Action select_action_list[] = {
	{"Select", 0, ActionSelect,
	 select_help, select_syntax}
	,
	{"Unselect", 0, ActionUnselect,
	 unselect_help, unselect_syntax}
};

REGISTER_ACTIONS(select_action_list, NULL)
