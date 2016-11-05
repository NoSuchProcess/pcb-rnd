/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "flag.h"

char *pcbflags_to_string(FlagType flags);
FlagType string_to_pcbflags(const char *flagstring, int (*error) (const char *msg));

/* ---------------------------------------------------------------------------
 * PCB flags - kept only for file format compatibility reasons; these bits
   should be a mirror of booleans from the conf.
 */

/* %start-doc pcbfile ~pcbflags
@node PCBFlags
@section PCBFlags
@table @code
@item 0x00001
Pinout displays pin numbers instead of pin names.
@item 0x00002
Use local reference for moves, by setting the mark at the beginning of
each move.
@item 0x00004
When set, only polygons and their clearances are drawn, to see if
polygons have isolated regions.
@item 0x00008
Display DRC region on crosshair.
@item 0x00010
Do all move, mirror, rotate with rubberband connections.
@item 0x00020
Display descriptions of elements, instead of refdes.
@item 0x00040
Display names of elements, instead of refdes.
@item 0x00080
Auto-DRC flag.  When set, PCB doesn't let you place copper that
violates DRC.
@item 0x00100
Enable 'all-direction' lines.
@item 0x00200
Switch starting angle after each click.
@item 0x00400
Force unique names on board.
@item 0x00800
New lines/arc clear polygons.
@item 0x01000
Crosshair snaps to pins and pads.
@item 0x02000
Show the solder mask layer.
@item 0x04000
Draw with thin lines.
@item 0x08000
Move items orthogonally.
@item 0x10000
Draw autoroute paths real-time.
@item 0x20000
New polygons are full ones.
@item 0x40000
Names are locked, the mouse cannot select them.
@item 0x80000
Everything but names are locked, the mouse cannot select anything else.
@item 0x100000
New polygons are full polygons.
@item 0x200000
When set, element names are not drawn.
+@item 0x800000
+snap to certain off-grid points.
+@item 0x1000000
+highlight lines and arcs when the crosshair is on one of their endpoints.
@end table
%end-doc */

#define PCB_FLAGS               0x01ffffff	/* all used flags */

#define SHOWNUMBERFLAG          0x00000001
#define LOCALREFFLAG            0x00000002
#define CHECKPLANESFLAG         0x00000004
#define SHOWPCB_FLAG_DRC             0x00000008
#define RUBBERBANDFLAG          0x00000010
#define	DESCRIPTIONFLAG         0x00000020
#define	NAMEONPCBFLAG           0x00000040
#define AUTOPCB_FLAG_DRC             0x00000080
#define	ALLDIRECTIONFLAG        0x00000100
#define SWAPSTARTDIRFLAG        0x00000200
#define UNIQUENAMEFLAG          0x00000400
#define CLEARNEWFLAG            0x00000800
#define SNAPPCB_FLAG_PIN             0x00001000
#define SHOWMASKFLAG            0x00002000
#define THINDRAWFLAG            0x00004000
#define ORTHOMOVEFLAG           0x00008000
#define LIVEROUTEFLAG           0x00010000
#define THINDRAWPOLYFLAG        0x00020000
#define LOCKNAMESFLAG           0x00040000
#define ONLYNAMESFLAG           0x00080000
#define NEWPCB_FLAG_FULLPOLY         0x00100000
#define HIDENAMESFLAG           0x00200000
#define ENABLEPCB_FLAG_MINCUT        0x00400000
