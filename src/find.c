/* $Id$ */

/*
 *
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/*
 * short description:
 * - lists for pins and vias, lines, arcs, pads and for polygons are created.
 *   Every object that has to be checked is added to its list.
 *   Coarse searching is accomplished with the data rtrees.
 * - there's no 'speed-up' mechanism for polygons because they are not used
 *   as often as other objects 
 * - the maximum distance between line and pin ... would depend on the angle
 *   between them. To speed up computation the limit is set to one half
 *   of the thickness of the objects (cause of square pins).
 *
 * PV:  means pin or via (objects that connect layers)
 * LO:  all non PV objects (layer objects like lines, arcs, polygons, pads)
 *
 * 1. first, the LO or PV at the given coordinates is looked up
 * 2. all LO connections to that PV are looked up next
 * 3. lookup of all LOs connected to LOs from (2).
 *    This step is repeated until no more new connections are found.
 * 4. lookup all PVs connected to the LOs from (2) and (3)
 * 5. start again with (1) for all new PVs from (4)
 *
 */

/* routines to find connections between pins, vias, lines...
 */
#include "config.h"

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <math.h>
#include <setjmp.h>
#include <assert.h>

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#include "conf_core.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "misc.h"
#include "rtree.h"
#include "polygon.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "rats.h"
#include "plug_io.h"
#include "hid_actions.h"
#include "misc_util.h"

#undef DEBUG

RCSID("$Id$");




/* ---------------------------------------------------------------------------
 * some local macros
 */

#define	SEPARATE(FP)							\
	{											\
		int	i;									\
		fputc('#', (FP));						\
		for (i = conf_core.appearance.messages.char_per_line; i; i--)	\
			fputc('=', (FP));					\
		fputc('\n', (FP));						\
	}

#define	PADLIST_ENTRY(L,I)	\
	(((PadTypePtr *)PadList[(L)].Data)[(I)])

#define	LINELIST_ENTRY(L,I)	\
	(((LineTypePtr *)LineList[(L)].Data)[(I)])

#define	ARCLIST_ENTRY(L,I)	\
	(((ArcTypePtr *)ArcList[(L)].Data)[(I)])

#define RATLIST_ENTRY(I)	\
	(((RatTypePtr *)RatList.Data)[(I)])

#define	POLYGONLIST_ENTRY(L,I)	\
	(((PolygonTypePtr *)PolygonList[(L)].Data)[(I)])

#define	PVLIST_ENTRY(I)	\
	(((PinTypePtr *)PVList.Data)[(I)])

/* ---------------------------------------------------------------------------
 * some local types
 *
 * the two 'dummy' structs for PVs and Pads are necessary for creating
 * connection lists which include the element's name
 */
typedef struct {
	void **Data;									/* pointer to index data */
	Cardinal Location,						/* currently used position */
	  DrawLocation, Number,				/* number of objects in list */
	  Size;
} ListType, *ListTypePtr;

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Coord Bloat = 0;
static int TheFlag = FOUNDFLAG;
static int OldFlag = FOUNDFLAG;
static void *thing_ptr1, *thing_ptr2, *thing_ptr3;
static int thing_type;
find_callback_t find_callback = NULL;
#define make_callback(current_type, current_ptr, from_type, from_ptr, type) \
	do { \
		if (find_callback != NULL) \
			find_callback(current_type, current_ptr, from_type, from_ptr, type); \
	} while(0)

static bool User = false;				/* user action causing this */
static bool drc = false;				/* whether to stop if finding something not found */
static bool IsBad = false;
static Cardinal drcerr_count;		/* count of drc errors */
static Cardinal TotalP, TotalV, NumberOfPads[2];
static ListType LineList[MAX_LAYER],	/* list of objects to */
  PolygonList[MAX_LAYER], ArcList[MAX_LAYER], PadList[2], RatList, PVList;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static bool LookupLOConnectionsToPVList(bool);
static bool LookupLOConnectionsToLOList(bool);
static bool LookupPVConnectionsToLOList(bool);
static bool LookupPVConnectionsToPVList(void);
static bool LookupLOConnectionsToLine(LineTypePtr, Cardinal, bool);
static bool LookupLOConnectionsToPad(PadTypePtr, Cardinal);
static bool LookupLOConnectionsToPolygon(PolygonTypePtr, Cardinal);
static bool LookupLOConnectionsToArc(ArcTypePtr, Cardinal);
static bool LookupLOConnectionsToRatEnd(PointTypePtr, Cardinal);
static bool IsRatPointOnLineEnd(PointTypePtr, LineTypePtr);
static bool ArcArcIntersect(ArcTypePtr, ArcTypePtr);
static bool PrepareNextLoop(FILE *);
static bool PrintElementConnections(ElementTypePtr, FILE *, bool);
static bool ListsEmpty(bool);
static bool DoIt(bool, bool);
static void PrintElementNameList(ElementTypePtr, FILE *);
static void PrintConnectionElementName(ElementTypePtr, FILE *);
static void PrintConnectionListEntry(char *, ElementTypePtr, bool, FILE *);
static void PrintPadConnections(Cardinal, FILE *, bool);
static void PrintPinConnections(FILE *, bool);
static bool PrintAndSelectUnusedPinsAndPadsOfElement(ElementTypePtr, FILE *);
static void DrawNewConnections(void);
static void DumpList(void);
static void LocateError(Coord *, Coord *);
static void BuildObjectList(int *, long int **, int **);
static void GotoError(void);
static bool DRCFind(int, void *, void *, void *);
static bool ListStart(int, void *, void *, void *);
static bool LOTouchesLine(LineTypePtr Line, Cardinal LayerGroup);
static bool PVTouchesLine(LineTypePtr line);
static bool SetThing(int, void *, void *, void *);


#include "find_geo.c"
#include "find_lookup.c"
#include "find_drc.c"
#include "find_misc.c"

#include "find_debug.c"
#include "find_print.c"
