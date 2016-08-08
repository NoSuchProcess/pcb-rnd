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
 *  Contact addresses for paper mail and Email:
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#include "strflags.h"
#include "flags.h"
#include "const.h"
#include "macro.h"

#define N(x) x, sizeof(x)-1
static FlagBitsType pcb_flagbits[] = {
	{SHOWNUMBERFLAG, N("shownumber"), 1},
	{LOCALREFFLAG, N("localref"), 1},
	{CHECKPLANESFLAG, N("checkplanes"), 1},
	{SHOWDRCFLAG, N("showdrc"), 1},
	{RUBBERBANDFLAG, N("rubberband"), 1},
	{DESCRIPTIONFLAG, N("description"), 1},
	{NAMEONPCBFLAG, N("nameonpcb"), 1},
	{AUTODRCFLAG, N("autodrc"), 1},
	{ALLDIRECTIONFLAG, N("alldirection"), 1},
	{SWAPSTARTDIRFLAG, N("swapstartdir"), 1},
	{UNIQUENAMEFLAG, N("uniquename"), 1},
	{CLEARNEWFLAG, N("clearnew"), 1},
	{NEWFULLPOLYFLAG, N("newfullpoly"), 1},
	{SNAPPINFLAG, N("snappin"), 1},
	{SHOWMASKFLAG, N("showmask"), 1},
	{THINDRAWFLAG, N("thindraw"), 1},
	{ORTHOMOVEFLAG, N("orthomove"), 1},
	{LIVEROUTEFLAG, N("liveroute"), 1},
	{THINDRAWPOLYFLAG, N("thindrawpoly"), 1},
	{LOCKNAMESFLAG, N("locknames"), 1},
	{ONLYNAMESFLAG, N("onlynames"), 1},
	{HIDENAMESFLAG, N("hidenames"), 1},
	{ENABLEMINCUTFLAG, N("enablemincut"), 1},
};
#undef N

char *pcbflags_to_string(FlagType flags)
{
	return common_flags_to_string(flags, ALL_TYPES, pcb_flagbits, ENTRIES(pcb_flagbits));
}

FlagType string_to_pcbflags(const char *flagstring, int (*error) (const char *msg))
{
	return common_string_to_flags(flagstring, error, pcb_flagbits, ENTRIES(pcb_flagbits));
}
