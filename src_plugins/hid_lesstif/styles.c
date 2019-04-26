#include "xincludes.h"

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "compat_misc.h"
#include "data.h"
#include "pcb-printf.h"

#include "actions.h"
#include "hid.h"
#include "lesstif.h"
#include "stdarg.h"
#include "misc_util.h"
#include "event.h"

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

static Widget style_values[SSNUM];

/* dynamic arrays for styles */
static Widget *units_pb;

typedef struct {
	Widget *w;
} StyleButtons;

static int local_update = 0;
XmString xms_mm, xms_mil;

static const pcb_unit_t *unit = 0;
static XmString ustr;

static void update_one_value(int i, pcb_coord_t v)
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
	lesstif_update_status_line();
	unit = conf_core.editor.grid_unit;
	ustr = XmStringCreateLocalized((char *) conf_core.editor.grid_unit->suffix);
	update_values();
}
