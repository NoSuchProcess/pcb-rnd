/* $Id$ */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "xincludes.h"

#include "compat_misc.h"
#include "global.h"
#include "data.h"
#include "set.h"
#include "misc.h"
#include "mymem.h"
#include "pcb-printf.h"

#include "hid.h"
#include "lesstif.h"
#include "hid_flags.h"
#include "stdarg.h"
#include "misc_util.h"


RCSID("$Id$");

/* There are three places where styles are kept:

   First, the "active" style is in conf_core.design.line_thickness et al.

   Second, there are NUM_STYLES styles in PCB->RouteStyle[].

   (not anymore: Third, there are NUM_STYLES styles in conf_core.design.RouteStyle[])

   Selecting a style copies its values to the active style.  We also
   need a way to modify the active style, copy the active style to
   PCB->RouteStyle[], and copy PCB->RouteStyle[] to
   Settings.RouteStyle[].  Since Lesstif reads the default style from
   .Xdefaults, we can ignore Settings.RouteStyle[] in general, as it's
   only used to initialize new PCB files.

   So, we need to do PCB->RouteStyle <-> active style.
*/

typedef enum {
	SSthick, SSdiam, SShole, SSkeep,
	SSNUM
} StyleValues;

static Widget style_dialog = 0;
static Widget style_values[SSNUM];

/* dynamic arrays for styles */
static int alloced_styles = 0;
static Widget *style_pb;
static Widget *units_pb;
static int *name_hashes;
typedef struct {
	Widget *w;
} StyleButtons;

static StyleButtons *style_button_list = NULL;
static int num_style_buttons = 0; /* number of style_button_list instances (depends on how many times it's placed in the menu) */
/**/

static Widget value_form, value_labels, value_texts, units_form;
static int local_update = 0;
XmString xms_mm, xms_mil;

static const Unit *unit = 0;
static XmString ustr;

static int hash(char *cp)
{
	int h = 0;
	while (*cp) {
		h = h * 13 + *(unsigned char *) cp;
		h ^= (h >> 16);
		cp++;
	}
	return h;
}

static char *value_names[] = {
	"Thickness", "Diameter", "Hole", "Clearance"
};

static int RouteStylesChanged(int argc, char **argv, Coord x, Coord y);

static void update_one_value(int i, Coord v)
{
	char buf[100];

	pcb_snprintf(buf, sizeof(buf), "%m+%.2mS", unit->allow, v);
	XmTextSetString(style_values[i], buf);
	stdarg_n = 0;
	stdarg(XmNlabelString, ustr);
	XtSetValues(units_pb[i], stdarg_args, stdarg_n);
}

static void update_values()
{
	local_update = 1;
	update_one_value(SSthick, conf_core.design.line_thickness);
	update_one_value(SSdiam, conf_core.design.via_thickness);
	update_one_value(SShole, conf_core.design.via_drilling_hole);
	update_one_value(SSkeep, conf_core.design.clearance);
	local_update = 0;
	lesstif_update_status_line();
}

void lesstif_styles_update_values()
{
	if (!style_dialog) {
		lesstif_update_status_line();
		return;
	}
	unit = conf_core.editor.grid_unit;
	ustr = XmStringCreateLocalized((char *) conf_core.editor.grid_unit->suffix);
	update_values();
}

static void update_style_buttons()
{
	int i = hid_get_flag("GetStyle()");
	int j, n;

	for (n = 0; n < num_style_buttons; n++) {
		for (j = 0; j < vtroutestyle_len(&PCB->RouteStyle); j++)
			if (j != i - 1)
				XmToggleButtonSetState(style_button_list[n].w[j], 0, 0);
			else
				XmToggleButtonSetState(style_button_list[n].w[j], 1, 0);
	}
	if (style_dialog) {
		for (j = 0; j < vtroutestyle_len(&PCB->RouteStyle); j++)
			if (j != i - 1)
				XmToggleButtonSetState(style_pb[j], 0, 0);
			else
				XmToggleButtonSetState(style_pb[j], 1, 0);
	}
}

static void style_value_cb(Widget w, int i, void *cbs)
{
	char *s;

	if (local_update)
		return;
	s = XmTextGetString(w);
	GetValueEx(s, NULL, NULL, NULL, unit->suffix, NULL);
	switch (i) {
	case SSthick:
		conf_set_design("design/line_thickness", "%s", s);
		break;
	case SSdiam:
		conf_set_design("design/via_thickness", "%s", s);
		break;
	case SShole:
		conf_set_design("design/via_drilling_hole", "%s", s);;
		break;
	case SSkeep:
		conf_set_design("design/clearance", "%s", s);;
		break;
	}
	update_style_buttons();
}

static void units_cb()
{
	if (unit == get_unit_struct("mm"))
		unit = get_unit_struct("mil");
	else
		unit = get_unit_struct("mm");
	ustr = XmStringCreateLocalized((char *) unit->suffix);
	update_values();
}

static Widget style_value(int i)
{
	Widget w, l;
	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_POSITION);
	stdarg(XmNtopPosition, i);
	stdarg(XmNbottomAttachment, XmATTACH_POSITION);
	stdarg(XmNbottomPosition, i + 1);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNalignment, XmALIGNMENT_END);
	l = XmCreateLabel(value_labels, value_names[i], stdarg_args, stdarg_n);
	XtManageChild(l);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_POSITION);
	stdarg(XmNtopPosition, i);
	stdarg(XmNbottomAttachment, XmATTACH_POSITION);
	stdarg(XmNbottomPosition, i + 1);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNcolumns, 8);
	w = XmCreateTextField(value_texts, value_names[i], stdarg_args, stdarg_n);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) style_value_cb, (XtPointer) (size_t) i);
	XtManageChild(w);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_POSITION);
	stdarg(XmNtopPosition, i);
	stdarg(XmNbottomAttachment, XmATTACH_POSITION);
	stdarg(XmNbottomPosition, i + 1);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNlabelString, ustr);
	units_pb[i] = XmCreatePushButton(units_form, value_names[i], stdarg_args, stdarg_n);
	XtAddCallback(units_pb[i], XmNactivateCallback, (XtCallbackProc) units_cb, (XtPointer) (size_t) i);
	XtManageChild(units_pb[i]);

	return w;
}

static void style_name_cb(Widget w, int i, XmToggleButtonCallbackStruct * cbs)
{
	char *newname = lesstif_prompt_for("New name", PCB->RouteStyle.array[i].name);
	strncpy(PCB->RouteStyle.array[i].name, newname, sizeof(PCB->RouteStyle.array[i].name)-1);
	PCB->RouteStyle.array[i].name[sizeof(PCB->RouteStyle.array[i].name)-1] = '\0';
	free(newname);
	
	RouteStylesChanged(0, 0, 0, 0);
}

static void style_set_cb(Widget w, int i, XmToggleButtonCallbackStruct * cbs)
{
	PCB->RouteStyle.array[i].Thick = conf_core.design.line_thickness;
	PCB->RouteStyle.array[i].Diameter = conf_core.design.via_thickness;
	PCB->RouteStyle.array[i].Hole = conf_core.design.via_drilling_hole;
	PCB->RouteStyle.array[i].Clearance = conf_core.design.clearance;
	update_style_buttons();
}

static void style_selected(Widget w, int i, XmToggleButtonCallbackStruct * cbs)
{
	RouteStyleType *style;
	int j, n;
	if (cbs && cbs->set == 0) {
		XmToggleButtonSetState(w, 1, 0);
		return;
	}
	style = PCB->RouteStyle.array + i;
	SetLineSize(style->Thick);
	SetViaSize(style->Diameter, true);
	SetViaDrillingHole(style->Hole, true);
	SetClearanceWidth(style->Clearance);
	if (style_dialog) {
		for (j = 0; j < vtroutestyle_len(&PCB->RouteStyle); j++)
			if (j != i)
				XmToggleButtonSetState(style_pb[j], 0, 0);
			else
				XmToggleButtonSetState(style_pb[j], 1, 0);
		update_values();
	}
	else
		lesstif_update_status_line();
	for (n = 0; n < num_style_buttons; n++) {
		for (j = 0; j < vtroutestyle_len(&PCB->RouteStyle); j++)
			if (j != i)
				XmToggleButtonSetState(style_button_list[n].w[j], 0, 0);
			else
				XmToggleButtonSetState(style_button_list[n].w[j], 1, 0);
	}
}

static Widget style_button(int i)
{
	Widget pb, set;

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_WIDGET);
	stdarg(XmNtopWidget, i ? style_pb[i - 1] : value_form);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNlabelString, XmStringCreatePCB("Name"));
	set = XmCreatePushButton(style_dialog, "style", stdarg_args, stdarg_n);
	XtManageChild(set);
	XtAddCallback(set, XmNactivateCallback, (XtCallbackProc) style_name_cb, (XtPointer) (size_t) i);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_WIDGET);
	stdarg(XmNtopWidget, i ? style_pb[i - 1] : value_form);
	stdarg(XmNleftAttachment, XmATTACH_WIDGET);
	stdarg(XmNleftWidget, set);
	stdarg(XmNlabelString, XmStringCreatePCB("Set"));
	set = XmCreatePushButton(style_dialog, "style", stdarg_args, stdarg_n);
	XtManageChild(set);
	XtAddCallback(set, XmNactivateCallback, (XtCallbackProc) style_set_cb, (XtPointer) (size_t) i);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_WIDGET);
	stdarg(XmNtopWidget, i ? style_pb[i - 1] : value_form);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNleftAttachment, XmATTACH_WIDGET);
	stdarg(XmNleftWidget, set);
	stdarg(XmNlabelString, XmStringCreatePCB(PCB->RouteStyle.array[i].name));
	stdarg(XmNindicatorType, XmONE_OF_MANY);
	stdarg(XmNalignment, XmALIGNMENT_BEGINNING);
	pb = XmCreateToggleButton(style_dialog, "style", stdarg_args, stdarg_n);
	XtManageChild(pb);
	XtAddCallback(pb, XmNvalueChangedCallback, (XtCallbackProc) style_selected, (XtPointer) (size_t) i);
	return pb;
}

static const char adjuststyle_syntax[] = "AdjustStyle()";

static const char adjuststyle_help[] = "Displays the route style adjustment window.";

/* %start-doc actions AdjustStyle

%end-doc */

static int AdjustStyle(int argc, char **argv, Coord x, Coord y)
{
	if (!mainwind)
		return 1;
	if (style_dialog == 0) {
		int i;

		unit = conf_core.editor.grid_unit;
		ustr = XmStringCreateLocalized((char *) unit->suffix);

		stdarg_n = 0;
		stdarg(XmNautoUnmanage, False);
		stdarg(XmNtitle, "Route Styles");
		style_dialog = XmCreateFormDialog(mainwind, "style", stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		value_form = XmCreateForm(style_dialog, "values", stdarg_args, stdarg_n);
		XtManageChild(value_form);

		stdarg_n = 0;
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		stdarg(XmNfractionBase, 4);
		stdarg(XmNresizePolicy, XmRESIZE_GROW);
		units_form = XmCreateForm(value_form, "units", stdarg_args, stdarg_n);
		XtManageChild(units_form);

		stdarg_n = 0;
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNfractionBase, 4);
		value_labels = XmCreateForm(value_form, "values", stdarg_args, stdarg_n);
		XtManageChild(value_labels);

		stdarg_n = 0;
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_WIDGET);
		stdarg(XmNrightWidget, units_form);
		stdarg(XmNleftAttachment, XmATTACH_WIDGET);
		stdarg(XmNleftWidget, value_labels);
		stdarg(XmNfractionBase, 4);
		value_texts = XmCreateForm(value_form, "values", stdarg_args, stdarg_n);
		XtManageChild(value_texts);

		for (i = 0; i < SSNUM; i++) {
			style_values[i] = style_value(i);
			name_hashes[i] = hash(PCB->RouteStyle.array[i].name);
		}
		for (i = 0; i < vtroutestyle_len(&PCB->RouteStyle); i++)
			style_pb[i] = style_button(i);
		update_values();
		update_style_buttons();
	}
	XtManageChild(style_dialog);
	return 0;
}

static int RouteStylesChanged(int argc, char **argv, Coord x, Coord y)
{
	int i, j, h;
	if (!PCB || vtroutestyle_len(&PCB->RouteStyle) == 0)
		return 0;
	update_style_buttons();
	if (!style_dialog)
		return 0;
	for (j = 0; j < vtroutestyle_len(&PCB->RouteStyle); j++) {
		h = hash(PCB->RouteStyle.array[j].name);
		if (name_hashes[j] == h)
			continue;
		name_hashes[j] = h;
		stdarg_n = 0;
		stdarg(XmNlabelString, XmStringCreatePCB(PCB->RouteStyle.array[j].name));
		if (style_dialog)
			XtSetValues(style_pb[j], stdarg_args, stdarg_n);
		for (i = 0; i < num_style_buttons; i++)
			XtSetValues(style_button_list[i].w[j], stdarg_args, stdarg_n);
	}
	update_values();
	return 0;
}

void lesstif_insert_style_buttons(Widget menu)
{
	StyleButtons *sb;
	int s, i;

	num_style_buttons++;
	s = num_style_buttons * sizeof(StyleButtons);
	style_button_list = (StyleButtons *) realloc(style_button_list, s);
	sb = style_button_list + num_style_buttons - 1;
	sb->w = NULL;

	alloced_styles = vtroutestyle_len(&PCB->RouteStyle);
	style_pb = realloc(style_pb, sizeof(style_pb[0]) * alloced_styles);
	units_pb = realloc(units_pb, sizeof(units_pb[0]) * alloced_styles);
	name_hashes = realloc(name_hashes, sizeof(name_hashes[0]) * alloced_styles);
	sb->w = realloc(sb->w, sizeof(sb->w[0]) * alloced_styles);

	for (i = 0; i < vtroutestyle_len(&PCB->RouteStyle); i++) {
		Widget btn;
		stdarg_n = 0;
		stdarg(XmNindicatorType, XmONE_OF_MANY);
		stdarg(XmNlabelString, XmStringCreatePCB(PCB->RouteStyle.array[i].name));
		btn = XmCreateToggleButton(menu, "style", stdarg_args, stdarg_n);
		XtManageChild(btn);
		XtAddCallback(btn, XmNvalueChangedCallback, (XtCallbackProc) style_selected, (XtPointer) (size_t) i);
		sb->w[i] = btn;
	}
	update_style_buttons();
}

HID_Action lesstif_styles_action_list[] = {
	{"AdjustStyle", 0, AdjustStyle,
	 adjuststyle_help, adjuststyle_syntax}
	,
	{"RouteStylesChanged", 0, RouteStylesChanged,
	 routestyleschanged_help, routestyleschanged_syntax}
};

REGISTER_ACTIONS(lesstif_styles_action_list, lesstif_cookie)
