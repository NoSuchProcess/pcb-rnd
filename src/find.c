/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */


/*
 * short description:
 * - lists for padstacks, lines, arcs, pads and for polygons are created.
 *   Every object that has to be checked is added to its list.
 *   Coarse searching is accomplished with the data rtrees.
 * - there's no 'speed-up' mechanism for polygons because they are not used
 *   as often as other objects
 * - the maximum distance between line and pin ... would depend on the angle
 *   between them. To speed up computation the limit is set to one half
 *   of the thickness of the objects (cause of square pins).
 *
 * PS:  means padstack
 * LO:  all non PS objects (layer objects like lines, arcs, polygons, texts)
 *
 * 1. first, the LO or PS at the given coordinates is looked up
 * 2. all LO connections to that PS are looked up next
 * 3. lookup of all LOs connected to LOs from (2).
 *    This step is repeated until no more new connections are found.
 * 4. lookup all PSs connected to the LOs from (2) and (3)
 * 5. start again with (1) for all new PSs from (4)
 *
 */

/* routines to find connections between objects */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

#include "math_helper.h"
#include "conf_core.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "rtree.h"
#include "polygon.h"
#include "search.h"
#include "undo.h"
#include "rats.h"
#include "plug_io.h"
#include "actions.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "layer.h"
#include "event.h"
#include "layer_vis.h"

#include "obj_text_draw.h"
#include "obj_pstk.h"
#include "obj_subc.h"
#include "obj_subc_parent.h"

#undef DEBUG

#define	LINELIST_ENTRY(L,I)	\
	(((pcb_line_t **)LineList[(L)].Data)[(I)])

#define	ARCLIST_ENTRY(L,I)	\
	(((pcb_arc_t **)ArcList[(L)].Data)[(I)])

#define RATLIST_ENTRY(I)	\
	(((pcb_rat_t **)RatList.Data)[(I)])

#define	POLYGONLIST_ENTRY(L,I)	\
	(((pcb_poly_t **)PolygonList[(L)].Data)[(I)])

#define	PADSTACKLIST_ENTRY(I)	\
	(((pcb_pstk_t **)PadstackList.Data)[(I)])

/* ---------------------------------------------------------------------------
 * the two 'dummy' structs for PVs and Pads are necessary for creating
 * connection lists which include the subcircuit's name
 */
typedef struct {
	void **Data;									/* pointer to index data */
	pcb_cardinal_t Location,						/* currently used position */
	  DrawLocation, Number,				/* number of objects in list */
	  Size;
} ListType, *ListTypePtr;

pcb_coord_t Bloat = 0;
int TheFlag = PCB_FLAG_FOUND;
static int OldFlag = PCB_FLAG_FOUND;

/* on DRC hit: the two offending objects */
static void  *pcb_found_obj1, *pcb_found_obj2;

pcb_find_callback_t pcb_find_callback = NULL;
#define make_callback(current_type, current_ptr, from_type, from_ptr, type) \
	do { \
		if (pcb_find_callback != NULL) \
			pcb_find_callback(current_type, current_ptr, from_type, from_ptr, type); \
	} while(0)

static pcb_bool User = pcb_false;				/* user action causing this */
static pcb_bool drc = pcb_false;				/* whether to stop if finding something not found */
static pcb_cardinal_t TotalPs;
static ListType LineList[PCB_MAX_LAYER+2],	/* list of objects to */
  PolygonList[PCB_MAX_LAYER+2], ArcList[PCB_MAX_LAYER+2],
  RatList, PadstackList;

static pcb_bool LookupLOConnectionsToPSList(pcb_bool);
static pcb_bool LookupLOConnectionsToLOList(pcb_bool);
static pcb_bool LookupLOConnectionsToLine(pcb_line_t *, pcb_cardinal_t, pcb_bool);
static pcb_bool LookupLOConnectionsToPolygon(pcb_poly_t *, pcb_cardinal_t);
static pcb_bool LookupLOConnectionsToArc(pcb_arc_t *, pcb_cardinal_t);
static pcb_bool LookupLOConnectionsToRatEnd(pcb_point_t *, pcb_cardinal_t);
static pcb_bool pcb_isc_ratp_line(pcb_point_t *, pcb_line_t *);
static pcb_bool pcb_isc_ratp_poly(pcb_point_t *Point, pcb_poly_t *polygon);
static pcb_bool pcb_isc_ratp_arc(pcb_point_t *Point, pcb_arc_t *arc);
static pcb_bool pcb_isc_arc_arc(pcb_arc_t *, pcb_arc_t *);
static pcb_bool ListsEmpty(pcb_bool);
static pcb_bool DoIt(pcb_bool, pcb_bool);
static void DumpList(void);
static pcb_bool ListStart(pcb_any_obj_t *obj);
static pcb_bool SetThing(void *group1_obj, void *group2_obj);


#include "find_geo.c"
#include "find_lookup.c"
#include "find_misc.c"

#include "find_any_isect.c"

#include "find2.c"
