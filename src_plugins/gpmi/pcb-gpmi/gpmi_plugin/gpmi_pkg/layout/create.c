#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/undo.h"
#include "src/conf_core.h"


typedef struct flag_tr_s {
	int flag; /* flag or thermal */
	int gpmi, pcb;
} flag_tr_t;

static flag_tr_t flags[] = {
	{1, FL_SHOWNUMBER,   SHOWNUMBERFLAG},
	{1, FL_LOCALREF,     LOCALREFFLAG},
	{1, FL_CHECKPLANS,   CHECKPLANESFLAG},
	{1, FL_SHOWDRC,      SHOWDRCFLAG},
	{1, FL_RUBBERBAND,   RUBBERBANDFLAG},
	{1, FL_DESCRIPTION,  DESCRIPTIONFLAG},
	{1, FL_NAMEONPCB,    NAMEONPCBFLAG},
	{1, FL_AUTODRC,      AUTODRCFLAG},
	{1, FL_ALLDIRECTION, ALLDIRECTIONFLAG},
	{1, FL_SWAPSTARTDIR, SWAPSTARTDIRFLAG},
	{1, FL_UNIQUENAME,   UNIQUENAMEFLAG},
	{1, FL_CLEARNEW,     CLEARNEWFLAG},
	{1, FL_SNAPPIN,      SNAPPINFLAG},
	{1, FL_SHOWMASK,     SHOWMASKFLAG},
	{1, FL_THINDRAW,     THINDRAWFLAG},
	{1, FL_ORTHOMOVE,    ORTHOMOVEFLAG},
	{1, FL_LIVEROUTE,    LIVEROUTEFLAG},
	{1, FL_THINDRAWPOLY, THINDRAWPOLYFLAG},
	{1, FL_LOCKNAMES,    LOCKNAMESFLAG},
	{1, FL_ONLYNAMES,    ONLYNAMESFLAG},
	{1, FL_NEWFULLPOLY,  NEWFULLPOLYFLAG},
	{1, FL_HIDENAMES,    HIDENAMESFLAG},

	{0, FL_THERMALSTYLE1, 1},
	{0, FL_THERMALSTYLE2, 2},
	{0, FL_THERMALSTYLE3, 3},
	{0, FL_THERMALSTYLE4, 4},
	{0, FL_THERMALSTYLE5, 5},
	{0, 0, 0}
};

static FlagType get_flags(int in)
{
	flag_tr_t *f;
	static FlagType out;

	out.f = 0;
	memset(out.t, 0, sizeof(out.t));
	for(f = flags; f->gpmi != 0; f++) {
		if (in & f->gpmi) {
			if (f->flag)
				out.f |= f->pcb;
			else
				memset(out.t, f->pcb, sizeof(out.t));
		}
	}
	return out;
}

static void *layout_create_line_(int x1, int y1, int x2, int y2, int thickness, int clearance, multiple layout_flag_t flags)
{
	void *line;

	line = CreateNewLineOnLayer (CURRENT, x1, y1, x2, y2, thickness, clearance, get_flags(flags));
	if (line != NULL) {
		AddObjectToCreateUndoList (LINE_TYPE, CURRENT, line, line);
		return line;
	}
	return NULL;
}

int layout_create_line(int x1, int y1, int x2, int y2, int thickness, int clearance, multiple layout_flag_t flags)
{
	return layout_create_line_(x1, y1, x2, y2, thickness, clearance, flags) != NULL;
}

static void *layout_create_via_(int x, int y, int thickness, int clearance, int mask, int hole, const char *name, multiple layout_flag_t flags)
{
	void *pin;

	pin = CreateNewVia (PCB->Data, x, y, thickness, clearance, mask, hole, name, get_flags(flags));

	if (pin != NULL) {
		AddObjectToCreateUndoList (VIA_TYPE, pin, pin, pin);
		return pin;
	}
	return NULL;
}

int layout_create_via(int x, int y, int thickness, int clearance, int mask, int hole, const char *name, multiple layout_flag_t flags)
{
	return layout_create_via_(x, y, thickness, clearance, mask, hole, name, flags) != NULL;
}

static void *layout_create_arc_(int x, int y, int width, int height, int sa, int dir, int thickness, int clearance, multiple layout_flag_t flags)
{
	void *arc;
	arc = CreateNewArcOnLayer (CURRENT, x, y, width, height, sa, dir, thickness, clearance, get_flags(flags));
	if (arc != NULL) {
		AddObjectToCreateUndoList (ARC_TYPE, CURRENT, arc, arc);
		return 0;
	}
	return NULL;
}

int layout_create_arc(int x, int y, int width, int height, int sa, int dir, int thickness, int clearance, multiple layout_flag_t flags)
{
	return layout_create_arc_(x, y, width, height, sa, dir, thickness, clearance, flags) != NULL;
}

