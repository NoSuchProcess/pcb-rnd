/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2005 DJ Delorie
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "config.h"
#include "conf_core.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "global.h"
#include "data.h"
#include "pcb-printf.h"
#include "hid_flags.h"


RCSID("$Id$");

static int FlagCurrentStyle(int dummy)
{
	STYLE_LOOP(PCB);
	{
		if (style->Thick == conf_core.design.line_thickness &&
				style->Diameter == conf_core.design.via_thickness &&
				style->Hole == conf_core.design.via_drilling_hole && style->Keepaway == conf_core.design.keepaway)
			return n + 1;
	}
	END_LOOP;
	return 0;
}

static int FlagGrid(int dummy)
{
	return PCB->Grid > 1;
}

static int FlagGridSize(int dummy)
{
	return PCB->Grid;
}

static int FlagUnitsMm(int dummy)
{
	static const Unit *u = NULL;
	if (u == NULL)
		u = get_unit_struct("mm");
#warning we can save a memcmp here, probably
	return memcmp(&conf_core.editor.grid_unit, u, sizeof(Unit));
}

static int FlagUnitsMil(int dummy)
{
	static const Unit *u = NULL;
	if (u == NULL)
		u = get_unit_struct("mil");
#warning we can save a memcmp here, probably
	return memcmp(&conf_core.editor.grid_unit, u, sizeof(Unit));
}

static int FlagBuffer(int dummy)
{
	return (int) (conf_core.editor.buffer_number + 1);
}

static int FlagElementName(int dummy)
{
	if (conf_core.editor.name_on_pcb)
		return 2;
	if (conf_core.editor.description)
		return 1;
	return 3;
}

static int FlagSETTINGS(int ofs)
{
	return *(bool *) ((char *) (&conf_core) + ofs);
}

static int FlagMode(int x)
{
	if (x == -1)
		return conf_core.editor.mode;
	return conf_core.editor.mode == x;
}

static int FlagHaveRegex(int x)
{
#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
	return 1;
#else
	return 0;
#endif
}

enum {
	FL_SILK = -6,
	FL_PINS,
	FL_RATS,
	FL_VIAS,
	FL_BACK,
	FL_MASK
};

static int FlagLayerShown(int n)
{
	switch (n) {
	case FL_SILK:
		return PCB->ElementOn;
	case FL_PINS:
		return PCB->PinOn;
	case FL_RATS:
		return PCB->RatOn;
	case FL_VIAS:
		return PCB->ViaOn;
	case FL_BACK:
		return PCB->InvisibleObjectsOn;
	case FL_MASK:
		return conf_core.editor.show_mask;
	default:
		if (n >= 0 && n < max_copper_layer)
			return PCB->Data->Layer[n].On;
	}
	return 0;
}

static int FlagLayerActive(int n)
{
	int current_layer;
	if (PCB->RatDraw)
		current_layer = FL_RATS;
	else if (PCB->SilkActive)
		current_layer = FL_SILK;
	else
		return 0;

	return current_layer == n;
}

/* The cast to (int) is ONLY valid because we know we are
 * taking offsets on structures where the offset will fit
 * in an integer variable. It silences compile warnings on
 * 64bit machines.
 */
#define OffsetOf(a,b) (int)(size_t)(&(((a *)0)->b))
HID_Flag flags_flag_list[] = {
	{"style", FlagCurrentStyle, 0}
	,
	{"grid", FlagGrid, 0}
	,
	{"gridsize", FlagGridSize, 0}
	,
	{"elementname", FlagElementName, 0}
	,
	{"have_regex", FlagHaveRegex, 0}
	,

	{"silk_shown", FlagLayerShown, FL_SILK}
	,
	{"pins_shown", FlagLayerShown, FL_PINS}
	,
	{"rats_shown", FlagLayerShown, FL_RATS}
	,
	{"vias_shown", FlagLayerShown, FL_VIAS}
	,
	{"back_shown", FlagLayerShown, FL_BACK}
	,
	{"mask_shown", FlagLayerShown, FL_MASK}
	,
	{"silk_active", FlagLayerActive, FL_SILK}
	,
	{"rats_active", FlagLayerActive, FL_RATS}
	,

	{"mode", FlagMode, -1}
	,
	{"nomode", FlagMode, NO_MODE}
	,
	{"arcmode", FlagMode, ARC_MODE}
	,
	{"arrowmode", FlagMode, ARROW_MODE}
	,
	{"copymode", FlagMode, COPY_MODE}
	,
	{"insertpointmode", FlagMode, INSERTPOINT_MODE}
	,
	{"linemode", FlagMode, LINE_MODE}
	,
	{"lockmode", FlagMode, LOCK_MODE}
	,
	{"movemode", FlagMode, MOVE_MODE}
	,
	{"pastebuffermode", FlagMode, PASTEBUFFER_MODE}
	,
	{"polygonmode", FlagMode, POLYGON_MODE}
	,
	{"polygonholemode", FlagMode, POLYGONHOLE_MODE}
	,
	{"rectanglemode", FlagMode, RECTANGLE_MODE}
	,
	{"removemode", FlagMode, REMOVE_MODE}
	,
	{"rotatemode", FlagMode, ROTATE_MODE}
	,
	{"rubberbandmovemode", FlagMode, RUBBERBANDMOVE_MODE}
	,
	{"textmode", FlagMode, TEXT_MODE}
	,
	{"thermalmode", FlagMode, THERMAL_MODE}
	,
	{"viamode", FlagMode, VIA_MODE}
	,
	{"grid_units_mm", FlagUnitsMm, -1}
	,
	{"grid_units_mil", FlagUnitsMil, -1}
	,
#warning TODO: access these via the conf system
#if 0
	{"shownumber", FlagTESTFLAG, SHOWNUMBERFLAG}
	,
	{"localref", FlagTESTFLAG, LOCALREFFLAG}
	,
	{"checkplanes", FlagTESTFLAG, CHECKPLANESFLAG}
	,
	{"showdrc", FlagTESTFLAG, SHOWDRCFLAG}
	,
	{"rubberband", FlagTESTFLAG, RUBBERBANDFLAG}
	,
	{"description", FlagTESTFLAG, DESCRIPTIONFLAG}
	,
	{"nameonpcb", FlagTESTFLAG, NAMEONPCBFLAG}
	,
	{"autodrc", FlagTESTFLAG, AUTODRCFLAG}
	,
	{"alldirection", FlagTESTFLAG, ALLDIRECTIONFLAG}
	,
	{"swapstartdir", FlagTESTFLAG, SWAPSTARTDIRFLAG}
	,
	{"uniquename", FlagTESTFLAG, UNIQUENAMEFLAG}
	,
	{"clearnew", FlagTESTFLAG, CLEARNEWFLAG}
	,
	{"snappin", FlagTESTFLAG, SNAPPINFLAG}
	,
	{"showmask", FlagTESTFLAG, SHOWMASKFLAG}
	,
	{"thindraw", FlagTESTFLAG, THINDRAWFLAG}
	,
	{"orthomove", FlagTESTFLAG, ORTHOMOVEFLAG}
	,
	{"liveroute", FlagTESTFLAG, LIVEROUTEFLAG}
	,
	{"thindrawpoly", FlagTESTFLAG, THINDRAWPOLYFLAG}
	,
	{"locknames", FlagTESTFLAG, LOCKNAMESFLAG}
	,
	{"onlynames", FlagTESTFLAG, ONLYNAMESFLAG}
	,
	{"newfullpoly", FlagTESTFLAG, NEWFULLPOLYFLAG}
	,
	{"hidenames", FlagTESTFLAG, HIDENAMESFLAG}
	,
	{"enablestroke", FlagTESTFLAG, OffsetOf(conf_core_t, editor.enable_stroke)}
	,
	{"snapoffgridline", FlagTESTFLAG, ffsetOf(conf_core_t, editor.snap_offgrid_line)}
	,
	{"highlightonpoint", FlagTESTFLAG, OffsetOf(conf_core_t, editor.highlight_on_point)}
	,
	{"fullpoly", FlagSETTINGS, OffsetOf(conf_core_t, editor.full_poly)}
	,
	{"clearline", FlagSETTINGS, OffsetOf(conf_core_t, editor.clear_line)}
	,
	{"uniquenames", FlagSETTINGS, OffsetOf(conf_core_t, editor.unique_names)}
	,
	{"showsolderside", FlagSETTINGS, OffsetOf(conf_core_t, editor.show_solder_side)}
	,
	{"savelastcommand", FlagSETTINGS, OffsetOf(conf_core_t, editor.save_last_command)}
	,
	{"saveintmp", FlagSETTINGS, OffsetOf(conf_core_t, editor.save_in_tmp)}
	,
	{"drawgrid", FlagSETTINGS, OffsetOf(conf_core_t, editor.draw_grid)}
	,
	{"ratwarn", FlagSETTINGS, OffsetOf(conf_core_t, temp.rat_warn)}
	,
	{"stipplepolygons", FlagSETTINGS, OffsetOf(conf_core_t, editor.stipple_polygons)}
	,
	{"alldirectionlines", FlagSETTINGS, OffsetOf(conf_core_t, editor.all_direction_lines)}
	,
	{"rubberbandmode", FlagSETTINGS, OffsetOf(conf_core_t, editor.rubber_band_mode)}
	,
	{"swapstartdirection", FlagSETTINGS, OffsetOf(conf_core_t, editor.swap_start_direction)}
	,
	{"showdrcmode", FlagSETTINGS, OffsetOf(conf_core_t, editor.show_drc)}
	,
	{"resetafterelement", FlagSETTINGS, OffsetOf(conf_core_t, editor.reset_after_element)}
	,
	{"ringbellwhenfinished", FlagSETTINGS, OffsetOf(conf_core_t, editor.beep_when_finished)}
	,
#endif
	{"buffer", FlagBuffer, 0}
	,

};

REGISTER_FLAGS(flags_flag_list, NULL)
