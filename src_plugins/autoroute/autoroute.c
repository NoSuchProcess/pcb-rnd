/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 *  this file, autoroute.c, was written and is
 *  Copyright (c) 2001 C. Scott Ananian
 *  Copyright (c) 2006 harry eaton
 *  Copyright (c) 2009 harry eaton
 *
 *  Updated for pcb-rnd for subcircuits, padstacks and netlist
 *  Copyright (c) 2018,2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

/*
 *-------------------------------------------------------------------
 * This file implements a rectangle-expansion router, based on
 * "A Method for Gridless Routing of Printed Circuit Boards" by
 * A. C. Finch, K. J. Mackenzie, G. J. Balsdon, and G. Symonds in the
 * 1985 Proceedings of the 22nd ACM/IEEE Design Automation Conference.
 * This reference is available from the ACM Digital Library at
 * http://www.acm.org/dl for those with institutional or personal
 * access to it.  It's also available from your local engineering
 * library.
 *
 * The code is much closer to what is described in the paper now,
 * in that expansion areas can grow from corners and in all directions
 * at once. Previously, these were emulated with discrete boxes moving
 * in the cardinal directions. With the new method, there are fewer but
 * larger expansion boxes that one might do a better job of routing in.
 *--------------------------------------------------------------------
 */
#define NET_HEAP 1
#include "config.h"
#include "conf_core.h"

#include <assert.h>
#include <setjmp.h>

#include "data.h"
#include "macro.h"
#include "autoroute.h"
#include "box.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "heap.h"
#include "rtree.h"
#include "netlist2.h"
#include "mtspace.h"
#include "polygon.h"
#include "remove.h"
#include "obj_pinvia_therm.h"
#include "undo.h"
#include "vector.h"
#include "pcb-printf.h"
#include "layer.h"
#include "compat_nls.h"
#include <genvector/vtp0.h>
#include "layer.h"

#include "obj_line_draw.h"
#include "obj_pstk_draw.h"
#include "obj_pstk_inlines.h"

TODO("padstack: when style contains proto, remove this")
#include "src_plugins/lib_compat_help/pstk_compat.h"

#include "brave.h"
#include "rats.h"

#define autoroute_therm_style 4

/* #defines to enable some debugging output */
/*
#define ROUTE_VERBOSE
*/

/*
#define ROUTE_DEBUG
//#define DEBUG_SHOW_ROUTE_BOXES
#define DEBUG_SHOW_EXPANSION_BOXES
//#define DEBUG_SHOW_EDGES
//#define DEBUG_SHOW_VIA_BOXES
#define DEBUG_SHOW_TARGETS
#define DEBUG_SHOW_SOURCES
//#define DEBUG_SHOW_ZIGZAG
*/

static pcb_direction_t directionIncrement(pcb_direction_t dir)
{
	switch (dir) {
	case PCB_NORTH:
		dir = PCB_EAST;
		break;
	case PCB_EAST:
		dir = PCB_SOUTH;
		break;
	case PCB_SOUTH:
		dir = PCB_WEST;
		break;
	case PCB_WEST:
		dir = PCB_NE;
		break;
	case PCB_NE:
		dir = PCB_SE;
		break;
	case PCB_SE:
		dir = PCB_SW;
		break;
	case PCB_SW:
		dir = PCB_NW;
		break;
	case PCB_NW:
		dir = PCB_ANY_DIR;
		break;
	case PCB_ANY_DIR:
		dir = PCB_NORTH;
		break;
	}
	return dir;
}

#ifdef ROUTE_DEBUG
pcb_hid_t *ddraw = NULL;
static pcb_hid_gc_t ar_gc = 0;
#endif

#define EXPENSIVE 3e28
/* round up "half" thicknesses */
#define HALF_THICK(x) (((x)+1)/2)
/* a styles maximum bloat is its clearance plus the larger of its via radius
 * or line half-thickness. */
#define BLOAT(style)\
	((style)->Clearance + HALF_THICK((style)->Thick))
/* conflict penalty is less for traces laid down during previous pass than
 * it is for traces already laid down in this pass. */
#define CONFLICT_LEVEL(rb)\
	(((rb)->flags.is_odd==AutoRouteParameters.is_odd) ?\
	 HI_CONFLICT : LO_CONFLICT )
#define CONFLICT_PENALTY(rb)\
	((CONFLICT_LEVEL(rb)==HI_CONFLICT ? \
	 AutoRouteParameters.ConflictPenalty : \
	 CONFLICT_LEVEL(rb)==LO_CONFLICT ? \
	 AutoRouteParameters.LastConflictPenalty : 1) * (rb)->pass)

#define _NORTH 1
#define _EAST 2
#define _SOUTH 4
#define _WEST 8

#define LIST_LOOP(init, which, x) do {\
     routebox_t *__next_one__ = (init);\
   x = NULL;\
   if (!__next_one__)\
     assert(__next_one__);\
   else\
   while (!x  || __next_one__ != (init)) {\
     x = __next_one__;\
     /* save next one first in case the command modifies or frees it */\
     __next_one__ = x->which.next
#define FOREACH_SUBNET(net, p) do {\
  routebox_t *_pp_;\
  /* fail-fast: check subnet_processed flags */\
  LIST_LOOP(net, same_net, p); \
  assert(!p->flags.subnet_processed);\
  PCB_END_LOOP;\
  /* iterate through *distinct* subnets */\
  LIST_LOOP(net, same_net, p); \
  if (!p->flags.subnet_processed) {\
    LIST_LOOP(p, same_subnet, _pp_);\
    _pp_->flags.subnet_processed=1;\
    PCB_END_LOOP
#define END_FOREACH(net, p) \
  }; \
  PCB_END_LOOP;\
  /* reset subnet_processed flags */\
  LIST_LOOP(net, same_net, p); \
  p->flags.subnet_processed=0;\
  PCB_END_LOOP;\
} while (0)
#define SWAP(t, f, s) do { t a=s; s=f; f=a; } while (0)
/* notes:
 * all rectangles are assumed to be closed on the top and left and
 * open on the bottom and right.   That is, they include their top-left
 * corner but don't include their bottom and right edges.
 *
 * expansion regions are always half-closed.  This means that when
 * tracing paths, you must steer clear of the bottom and right edges.,
 * because these are not actually in the allowed box.
 *
 * All routeboxes *except* EXPANSION_AREAS now have their "box" bloated by
 * their particular required clearance. This simplifies the tree searching.
 * the "sbox" contains the unbloated box.
 */

/* enumerated type for conflict levels */
typedef enum { NO_CONFLICT = 0, LO_CONFLICT = 1, HI_CONFLICT = 2 } conflict_t;

typedef struct routebox_s routebox_t;

typedef struct routebox_list_s {
	routebox_t *next, *prev;
} routebox_list_t;

typedef enum etype { TERM, VIA, VIA_SHADOW, LINE, OTHER, EXPANSION_AREA, PLANE, THERMAL } etype;

struct routebox_s {
	pcb_box_t box, sbox;
	struct {
		pcb_coord_t x1, y1, x2, y2;
	} line; /* exact coords of the line we are going to draw if type is line; reverse engineering these from the bounding box using halfthick and other hacks lead to rounding errors, a few LSB flicker in coords, e.g. breaking rubber band */
	union {
		pcb_pstk_t *via;
		pcb_any_obj_t *term;
		routebox_t *via_shadow;	/* points to the via in r-tree which
																	 * points to the pcb_pstk_t in the PCB. */
		pcb_line_t *line;
		void *generic;							/* 'other' is polygon, arc, text */
		routebox_t *expansion_area;	/* previous expansion area in search */
	} parent;
	unsigned short group;
	unsigned short layer;
	etype type;
	struct {
		unsigned nonstraight:1;
		unsigned fixed:1;
		/* for searches */
		unsigned source:1;
		unsigned target:1;
		/* rects on same net as source and target don't need clearance areas */
		unsigned nobloat:1;
		/* mark circular pins, so that we be sure to connect them up properly */
		unsigned circular:1;
		/* we sometimes create routeboxen that don't actually belong to a
		 * r-tree yet -- make sure refcount of homelesss is set properly */
		unsigned homeless:1;
		/* was this nonfixed obstacle generated on an odd or even pass? */
		unsigned is_odd:1;
		/* fixed route boxes that have already been "routed through" in this
		 * search have their "touched" flag set. */
		unsigned touched:1;
		/* this is a status bit for iterating through *different* subnets */
		unsigned subnet_processed:1;
		/* some expansion_areas represent via candidates */
		unsigned is_via:1;
		/* mark non-straight lines which go from bottom-left to upper-right,
		 * instead of from upper-left to bottom-right. */
		unsigned bl_to_ur:1;
		/* mark polygons which are "transparent" for via-placement; that is,
		 * vias through the polygon will automatically be given a clearance
		 * and will not electrically connect to the polygon. */
		unsigned clear_poly:1;
		/* this marks "conflicting" routes that must be torn up to obtain
		 * a correct routing.  This flag allows us to return a correct routing
		 * even if the user cancels auto-route after a non-final pass. */
		unsigned is_bad:1;
		/* for assertion that 'box' is never changed after creation */
		unsigned inited:1;
		/* indicate this expansion ares is a thermal between the pin and plane */
		unsigned is_thermal;
	} flags;
	/* indicate the direction an expansion box came from */
	pcb_cost_t cost;
	pcb_cheap_point_t cost_point;
	/* reference count for homeless routeboxes; free when refcount==0 */
	int refcount;
	/* when routing with conflicts, we keep a record of what we're
	 * conflicting with.
	 */
	vector_t *conflicts_with;
	/* route style of the net associated with this routebox */
	pcb_route_style_t *style;
	/* congestion values for the edges of an expansion box */
	unsigned char n, e, s, w;
	/* what pass this this track was laid down on */
	unsigned char pass;
	/* the direction this came from, if any */
	pcb_direction_t came_from;
	/* circular lists with connectivity information. */
	routebox_list_t same_net, same_subnet, original_subnet, different_net;
	union {
		pcb_pstk_t *via;
		pcb_line_t *line;
	} livedraw_obj;
};

typedef struct routedata_s {
	int max_styles;
	/* one rtree per layer *group */
	pcb_rtree_t *layergrouptree[PCB_MAX_LAYERGRP];	/* no silkscreen layers here =) */
	/* root pointer into connectivity information */
	routebox_t *first_net;
	/* default routing style */
	pcb_route_style_t defaultstyle;
	/* style structures */
	pcb_route_style_t **styles; /* [max_styles+1] */
	/* what is the maximum bloat (clearance+line half-width or
	 * clearance+via_radius) for any style we've seen? */
	pcb_coord_t max_bloat;
	pcb_coord_t max_keep;
	mtspace_t *mtspace;
} routedata_t;

typedef struct edge_struct_s {
	routebox_t *rb;								/* path expansion edges are real routeboxen. */
	pcb_cheap_point_t cost_point;
	pcb_cost_t pcb_cost_to_point;					/* from source */
	pcb_cost_t cost;									/* cached edge cost */
	routebox_t *minpcb_cost_target;		/* minimum cost from cost_point to any target */
	vetting_t *work;							/* for via search edges */
	pcb_direction_t expand_dir;
	struct {
		/* this indicates that this 'edge' is a via candidate. */
		unsigned is_via:1;
		/* record "conflict level" of via candidates, in case we need to split
		 * them later. */
		unsigned via_conflict_level:2; /* conflict_t */
		/* when "routing with conflicts", sometimes edge is interior. */
		unsigned is_interior:1;
		/* this is a fake edge used to defer searching for via spaces */
		unsigned via_search:1;
		/* this is a via edge in a plane where the cost point moves for free */
		unsigned in_plane:1;
	} flags;
} edge_t;

static struct AutoRouteParameters_s {
	/* net style parameters */
	pcb_route_style_t *style;
	/* the present bloat */
	pcb_coord_t bloat;
	/* cost parameters */
	pcb_cost_t ViaCost,								/* additional "length" cost for using a via */
	  LastConflictPenalty,				/* length mult. for routing over last pass' trace */
	  ConflictPenalty,						/* length multiplier for routing over another trace */
	  JogPenalty,									/* additional "length" cost for changing direction */
	  CongestionPenalty,					/* (rational) length multiplier for routing in */
	  NewLayerPenalty,						/* penalty for routing on a previously unused layer */
	  MinPenalty;									/* smallest Direction Penalty */
	/* maximum conflict incidence before calling it "no path found" */
	int hi_conflict;
	/* are vias allowed? */
	pcb_bool use_vias;
	/* is this an odd or even pass? */
	pcb_bool is_odd;
	/* permit conflicts? */
	pcb_bool with_conflicts;
	/* is this a final "smoothing" pass? */
	pcb_bool is_smoothing;
	/* rip up nets regardless of conflicts? */
	pcb_bool rip_always;
	pcb_bool last_smooth;
	unsigned char pass;
} AutoRouteParameters;

typedef struct routeone_state_s {
	/* heap of all candidate expansion edges */
	pcb_heap_t *workheap;
	/* information about the best path found so far. */
	routebox_t *best_path, *best_target;
	pcb_cost_t best_cost;
} routeone_state_t;


static routebox_t *CreateExpansionArea(const pcb_box_t * area, pcb_cardinal_t group,
																			 routebox_t * parent, pcb_bool relax_edge_requirements, edge_t * edge);

static pcb_cost_t edge_cost(const edge_t * e, const pcb_cost_t too_big);
static void best_path_candidate(routeone_state_t *s, edge_t * e, routebox_t * best_target);

static pcb_box_t edge_to_box(const routebox_t * rb, pcb_direction_t expand_dir);

static void add_or_destroy_edge(routeone_state_t *s, edge_t * e);

static void
RD_DrawThermal(routedata_t * rd, pcb_coord_t X, pcb_coord_t Y, pcb_cardinal_t group, pcb_cardinal_t layer, routebox_t * subnet, pcb_bool is_bad);
static void ResetSubnet(routebox_t * net);
#ifdef ROUTE_DEBUG
static int showboxen = -2;
static int aabort = 0;
static void showroutebox(routebox_t * rb);
#endif

/* group number of groups that hold surface mount pads */
static pcb_layergrp_id_t front, back;
static pcb_bool usedGroup[PCB_MAX_LAYERGRP];
static int x_cost[PCB_MAX_LAYERGRP], y_cost[PCB_MAX_LAYERGRP];
static pcb_bool is_layer_group_active[PCB_MAX_LAYERGRP];
static int ro = 0;
static int smoothes = 1;
static int passes = 12;
static int routing_layers = 0;
static float total_wire_length = 0;
static int total_via_count = 0;

/* assertion helper for routeboxen */
#ifndef NDEBUG
static int __routepcb_box_is_good(routebox_t * rb)
{
	assert(rb && (rb->group < pcb_max_group(PCB)) &&
				 (rb->box.X1 <= rb->box.X2) && (rb->box.Y1 <= rb->box.Y2) &&
				 (rb->flags.homeless ?
					(rb->box.X1 != rb->box.X2) || (rb->box.Y1 != rb->box.Y2) : (rb->box.X1 != rb->box.X2) && (rb->box.Y1 != rb->box.Y2)));
	assert((rb->flags.source ? rb->flags.nobloat : 1) &&
				 (rb->flags.target ? rb->flags.nobloat : 1) &&
				 (rb->flags.homeless ? !rb->flags.touched : rb->refcount == 0) && (rb->flags.touched ? rb->type != EXPANSION_AREA : 1));
	assert((rb->flags.is_odd ? (!rb->flags.fixed) &&
					(rb->type == VIA || rb->type == VIA_SHADOW || rb->type == LINE || rb->type == PLANE) : 1));
	assert(rb->flags.clear_poly ? ((rb->type == OTHER || rb->type == PLANE) && rb->flags.fixed && !rb->flags.homeless) : 1);
	assert(rb->flags.inited);
/* run through conflict list showing none are homeless, targets or sources */
	if (rb->conflicts_with) {
		int i;
		for (i = 0; i < vector_size(rb->conflicts_with); i++) {
			routebox_t *c = vector_element(rb->conflicts_with, i);
			assert(!c->flags.homeless && !c->flags.source && !c->flags.target && !c->flags.fixed);
		}
	}
	assert(rb->style != NULL && rb->style != NULL);
	assert(rb->type == EXPANSION_AREA
				 || (rb->same_net.next && rb->same_net.prev && rb->same_subnet.next
						 && rb->same_subnet.prev && rb->original_subnet.next
						 && rb->original_subnet.prev && rb->different_net.next && rb->different_net.prev));
	return 1;
}

static int __edge_is_good(edge_t * e)
{
	assert(e && e->rb && __routepcb_box_is_good(e->rb));
	assert((e->rb->flags.homeless ? e->rb->refcount > 0 : 1));
	assert((0 <= e->expand_dir) && (e->expand_dir < 9)
				 && (e->flags.is_interior ? (e->expand_dir == PCB_ANY_DIR && e->rb->conflicts_with) : 1));
	assert((e->flags.is_via ? e->rb->flags.is_via : 1)
				 && (e->flags.via_conflict_level >= 0 && e->flags.via_conflict_level <= 2)
				 && (e->flags.via_conflict_level != 0 ? e->flags.is_via : 1));
	assert((e->pcb_cost_to_point >= 0) && e->cost >= 0);
	return 1;
}

int no_planes(const pcb_box_t * b, void *cl)
{
	routebox_t *rb = (routebox_t *) b;
	if (rb->type == PLANE)
		return 0;
	return 1;
}
#endif /* !NDEBUG */

/*---------------------------------------------------------------------
 * route utility functions.
 */

enum boxlist { NET, SUBNET, ORIGINAL, DIFFERENT_NET };
static routebox_list_t *__select_list(routebox_t * r, enum boxlist which)
{
	assert(r);
	switch (which) {
	default:
		assert(0);
	case NET:
		return &(r->same_net);
	case SUBNET:
		return &(r->same_subnet);
	case ORIGINAL:
		return &(r->original_subnet);
	case DIFFERENT_NET:
		return &(r->different_net);
	}
}

static void InitLists(routebox_t * r)
{
	static enum boxlist all[] = { NET, SUBNET, ORIGINAL, DIFFERENT_NET }
	, *p;
	for (p = all; p < all + (sizeof(all) / sizeof(*p)); p++) {
		routebox_list_t *rl = __select_list(r, *p);
		rl->prev = rl->next = r;
	}
}

static void MergeNets(routebox_t * a, routebox_t * b, enum boxlist which)
{
	routebox_list_t *al, *bl, *anl, *bnl;
	routebox_t *an, *bn;
	assert(a && b);
	assert(a != b);
	assert(a->type != EXPANSION_AREA);
	assert(b->type != EXPANSION_AREA);
	al = __select_list(a, which);
	bl = __select_list(b, which);
	assert(al && bl);
	an = al->next;
	bn = bl->next;
	assert(an && bn);
	anl = __select_list(an, which);
	bnl = __select_list(bn, which);
	assert(anl && bnl);
	bl->next = an;
	anl->prev = b;
	al->next = bn;
	bnl->prev = a;
}

static void RemoveFromNet(routebox_t * a, enum boxlist which)
{
	routebox_list_t *al, *anl, *apl;
	routebox_t *an, *ap;
	assert(a);
	al = __select_list(a, which);
	assert(al);
	an = al->next;
	ap = al->prev;
	if (an == a || ap == a)
		return;											/* not on any list */
	assert(an && ap);
	anl = __select_list(an, which);
	apl = __select_list(ap, which);
	assert(anl && apl);
	anl->prev = ap;
	apl->next = an;
	al->next = al->prev = a;
	return;
}

static void init_const_box(routebox_t * rb, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t clearance)
{
	pcb_box_t *bp = (pcb_box_t *) & rb->box;	/* note discarding const! */
	assert(!rb->flags.inited);
	assert(X1 <= X2 && Y1 <= Y2);
	bp->X1 = X1 - clearance;
	bp->Y1 = Y1 - clearance;
	bp->X2 = X2 + clearance;
	bp->Y2 = Y2 + clearance;
	bp = (pcb_box_t *) & rb->sbox;
	bp->X1 = X1;
	bp->Y1 = Y1;
	bp->X2 = X2;
	bp->Y2 = Y2;
	rb->flags.inited = 1;
}

static inline pcb_box_t shrink_routebox(const routebox_t * rb)
{
	return rb->sbox;
}

static inline pcb_cost_t box_area(const pcb_box_t b)
{
	pcb_cost_t ans = b.X2 - b.X1;
	return ans * (b.Y2 - b.Y1);
}

static inline pcb_cheap_point_t closest_point_in_routebox(const pcb_cheap_point_t * from, const routebox_t * rb)
{
	return pcb_closest_pcb_point_in_box(from, &rb->sbox);
}

static inline pcb_bool point_in_shrunk_box(const routebox_t * box, pcb_coord_t X, pcb_coord_t Y)
{
	pcb_box_t b = shrink_routebox(box);
	return pcb_point_in_box(&b, X, Y);
}

/*---------------------------------------------------------------------
 * routedata initialization functions.
 */
static routebox_t *AddTerm_(vtp0_t layergroupboxes[], pcb_any_obj_t *term, pcb_route_style_t *style, pcb_layer_t *layer)
{
	routebox_t **rbpp;
	pcb_layer_type_t lyt;
	int layergroup = -1;
	pcb_coord_t clr;

	lyt = pcb_layer_flags_(layer);
	if (!(lyt & PCB_LYT_COPPER))
		return NULL;
	if (lyt & PCB_LYT_TOP)
		layergroup = front;
	else if (lyt & PCB_LYT_BOTTOM)
		layergroup = back;
	else {
		if (term->type != PCB_OBJ_PSTK)
			assert(!"won't route to pad on internal layer");
		layergroup = pcb_layer_get_group_(layer);
	}
	assert(0 <= layergroup && layergroup < pcb_max_group(PCB));

	assert(PCB->LayerGroups.grp[layergroup].len > 0);
	rbpp = (routebox_t **)vtp0_alloc_append(&layergroupboxes[layergroup], 1);
	assert(rbpp);
	*rbpp = (routebox_t *)calloc(sizeof(**rbpp), 1);
	assert(*rbpp);
	(*rbpp)->group = layergroup;
	clr = pcb_obj_clearance_at(PCB, term, layer);
pcb_printf("***** CLR=%mm\n", clr);
	init_const_box(*rbpp,
								 /*X1 */ term->BoundingBox.X1 + clr,
								 /*Y1 */ term->BoundingBox.Y1 + clr,
								 /*X2 */ term->BoundingBox.X2 - clr,
								 /*Y2 */ term->BoundingBox.Y2 - clr,
								 style->Clearance);
	/* kludge for non-manhattan pads (which are not allowed at present) */
TODO("term:")
/*
	if (pad->Point1.X != pad->Point2.X && pad->Point1.Y != pad->Point2.Y)
		(*rbpp)->flags.nonstraight = 1;
*/
	(*rbpp)->flags.nonstraight = 0; /* otherwise the autorouter just ignores it */
	/* set aux. properties */
	(*rbpp)->type = TERM;
	(*rbpp)->parent.term = term;
	(*rbpp)->flags.fixed = 1;
	(*rbpp)->came_from = PCB_ANY_DIR;
	(*rbpp)->style = style;
	/* circular lists */
	InitLists(*rbpp);
	return *rbpp;
}

static routebox_t *AddTerm(vtp0_t layergroupboxes[], pcb_any_obj_t *term, pcb_route_style_t *style)
{
	assert(term->parent_type == PCB_PARENT_LAYER);
	return AddTerm_(layergroupboxes, term, style, term->parent.layer);
}

static routebox_t *AddPstk(vtp0_t layergroupboxes[], pcb_pstk_t *ps, pcb_route_style_t *style)
{
	int i;
	routebox_t *r, *last = NULL;

	for (i = 0; i < pcb_max_group(PCB); i++) {
		if (pcb_pstk_shape_at(PCB, ps, &PCB->Data->Layer[i]) != NULL) {
			r = AddTerm_(layergroupboxes, (pcb_any_obj_t *)ps, style, &PCB->Data->Layer[i]);
			if (r != NULL) {
				if (last != NULL) {
					MergeNets(r, last, NET);
					MergeNets(r, last, SUBNET);
					MergeNets(r, last, ORIGINAL);
				}
				last = r;
			}
		}
	}

	return last;
}


static routebox_t *AddLine(vtp0_t layergroupboxes[], int layergroup, pcb_line_t *line,
													 pcb_line_t *ptr, pcb_route_style_t * style)
{
	routebox_t **rbpp;
	assert(layergroupboxes && line);
	assert(0 <= layergroup && layergroup < pcb_max_group(PCB));
	assert(PCB->LayerGroups.grp[layergroup].len > 0);

	rbpp = (routebox_t **) vtp0_alloc_append(&layergroupboxes[layergroup], 1);
	*rbpp = (routebox_t *) malloc(sizeof(**rbpp));
	memset(*rbpp, 0, sizeof(**rbpp));
	(*rbpp)->group = layergroup;
	init_const_box(*rbpp,
								 /*X1 */ MIN(line->Point1.X,
														 line->Point2.X) - HALF_THICK(line->Thickness),
								 /*Y1 */ MIN(line->Point1.Y,
														 line->Point2.Y) - HALF_THICK(line->Thickness),
								 /*X2 */ MAX(line->Point1.X,
														 line->Point2.X) + HALF_THICK(line->Thickness),
								 /*Y2 */ MAX(line->Point1.Y,
														 line->Point2.Y) + HALF_THICK(line->Thickness), style->Clearance);
	/* kludge for non-manhattan lines */
	if (line->Point1.X != line->Point2.X && line->Point1.Y != line->Point2.Y) {
		(*rbpp)->flags.nonstraight = 1;
		(*rbpp)->flags.bl_to_ur =
			(MIN(line->Point1.X, line->Point2.X) == line->Point1.X) != (MIN(line->Point1.Y, line->Point2.Y) == line->Point1.Y);
#if defined(ROUTE_DEBUG) && defined(DEBUG_SHOW_ZIGZAG)
		showroutebox(*rbpp);
#endif
	}
	/* set aux. properties */
	(*rbpp)->type = LINE;
	(*rbpp)->line.x1 = line->Point1.X;
	(*rbpp)->line.y1 = line->Point1.Y;
	(*rbpp)->line.x2 = line->Point2.X;
	(*rbpp)->line.y2 = line->Point2.Y;
	(*rbpp)->parent.line = ptr;
	(*rbpp)->flags.fixed = 1;
	(*rbpp)->came_from = PCB_ANY_DIR;
	(*rbpp)->style = style;
	/* circular lists */
	InitLists(*rbpp);
	return *rbpp;
}

static routebox_t *AddIrregularObstacle(vtp0_t layergroupboxes[],
																				pcb_coord_t X1, pcb_coord_t Y1,
																				pcb_coord_t X2, pcb_coord_t Y2, pcb_cardinal_t layergroup, void *parent, pcb_route_style_t * style)
{
	routebox_t **rbpp;
	pcb_coord_t keep = style->Clearance;
	assert(layergroupboxes && parent);
	assert(X1 <= X2 && Y1 <= Y2);
	assert(0 <= layergroup && layergroup < pcb_max_group(PCB));
	assert(PCB->LayerGroups.grp[layergroup].len > 0);

	rbpp = (routebox_t **) vtp0_alloc_append(&layergroupboxes[layergroup], 1);
	*rbpp = (routebox_t *) malloc(sizeof(**rbpp));
	memset(*rbpp, 0, sizeof(**rbpp));
	(*rbpp)->group = layergroup;
	init_const_box(*rbpp, X1, Y1, X2, Y2, keep);
	(*rbpp)->flags.nonstraight = 1;
	(*rbpp)->type = OTHER;
	(*rbpp)->parent.generic = parent;
	(*rbpp)->flags.fixed = 1;
	(*rbpp)->style = style;
	/* circular lists */
	InitLists(*rbpp);
	return *rbpp;
}

static routebox_t *AddPolygon(vtp0_t layergroupboxes[], pcb_cardinal_t layer, pcb_poly_t *polygon, pcb_route_style_t * style)
{
	int is_not_rectangle = 1;
	pcb_layergrp_id_t layergroup = pcb_layer_get_group(PCB, layer);
	routebox_t *rb;
	assert(0 <= layergroup && layergroup < pcb_max_group(PCB));
	rb = AddIrregularObstacle(layergroupboxes,
														polygon->BoundingBox.X1,
														polygon->BoundingBox.Y1,
														polygon->BoundingBox.X2, polygon->BoundingBox.Y2, layergroup, polygon, style);
	if (polygon->PointN == 4 &&
			polygon->HoleIndexN == 0 &&
			(polygon->Points[0].X == polygon->Points[1].X ||
			 polygon->Points[0].Y == polygon->Points[1].Y) &&
			(polygon->Points[1].X == polygon->Points[2].X ||
			 polygon->Points[1].Y == polygon->Points[2].Y) &&
			(polygon->Points[2].X == polygon->Points[3].X ||
			 polygon->Points[2].Y == polygon->Points[3].Y) &&
			(polygon->Points[3].X == polygon->Points[0].X || polygon->Points[3].Y == polygon->Points[0].Y))
		is_not_rectangle = 0;
	rb->flags.nonstraight = is_not_rectangle;
	rb->layer = layer;
	rb->came_from = PCB_ANY_DIR;
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, polygon)) {
		rb->flags.clear_poly = 1;
		if (!is_not_rectangle)
			rb->type = PLANE;
	}
	return rb;
}

static void AddText(vtp0_t layergroupboxes[], pcb_cardinal_t layergroup, pcb_text_t *text, pcb_route_style_t * style)
{
	AddIrregularObstacle(layergroupboxes,
											 text->BoundingBox.X1, text->BoundingBox.Y1,
											 text->BoundingBox.X2, text->BoundingBox.Y2, layergroup, text, style);
}

static routebox_t *AddArc(vtp0_t layergroupboxes[], pcb_cardinal_t layergroup, pcb_arc_t *arc, pcb_route_style_t * style)
{
	return AddIrregularObstacle(layergroupboxes,
															arc->BoundingBox.X1, arc->BoundingBox.Y1,
															arc->BoundingBox.X2, arc->BoundingBox.Y2, layergroup, arc, style);
}

struct rb_info {
	pcb_box_t query;
	routebox_t *winner;
	jmp_buf env;
};

static pcb_r_dir_t __found_one_on_lg(const pcb_box_t * box, void *cl)
{
	struct rb_info *inf = (struct rb_info *) cl;
	routebox_t *rb = (routebox_t *) box;
	pcb_box_t sb;

	if (rb->flags.nonstraight)
		return PCB_R_DIR_NOT_FOUND;
	sb = pcb_shrink_box(&rb->box, rb->style->Clearance);
	if (inf->query.X1 >= sb.X2 || inf->query.X2 <= sb.X1 || inf->query.Y1 >= sb.Y2 || inf->query.Y2 <= sb.Y1)
		return PCB_R_DIR_NOT_FOUND;
	inf->winner = rb;
	if (rb->type == PLANE)
		return PCB_R_DIR_FOUND_CONTINUE;										/* keep looking for something smaller if a plane was found */
	longjmp(inf->env, 1);
	return PCB_R_DIR_NOT_FOUND;
}

static routebox_t *FindRouteBoxOnLayerGroup(routedata_t * rd, pcb_coord_t X, pcb_coord_t Y, pcb_cardinal_t layergroup)
{
	struct rb_info info;
	info.winner = NULL;
	info.query = pcb_point_box(X, Y);
	if (setjmp(info.env) == 0)
		pcb_r_search(rd->layergrouptree[layergroup], &info.query, NULL, __found_one_on_lg, &info, NULL);
	return info.winner;
}

#ifdef ROUTE_DEBUG_VERBOSE
static void DumpRouteBox(routebox_t * rb)
{
	pcb_printf("RB: %#mD-%#mD l%d; ", rb->box.X1, rb->box.Y1, rb->box.X2, rb->box.Y2, (int) rb->group);
	switch (rb->type) {
	case TERM:
		printf("TERM[%s] ", rb->parent.term->term);
		break;
	case LINE:
		printf("LINE ");
		break;
	case OTHER:
		printf("OTHER ");
		break;
	case EXPANSION_AREA:
		printf("EXPAREA ");
		break;
	default:
		printf("UNKNOWN ");
		break;
	}
	if (rb->flags.nonstraight)
		printf("(nonstraight) ");
	if (rb->flags.fixed)
		printf("(fixed) ");
	if (rb->flags.source)
		printf("(source) ");
	if (rb->flags.target)
		printf("(target) ");
	if (rb->flags.homeless)
		printf("(homeless) ");
	printf("\n");
}
#endif

/* Add obstacles extracting them from data (subc-recursive) */
static void CreateRouteData_subc(routedata_t *rd, vtp0_t layergroupboxes[], pcb_data_t *data)
{
	int i;

	PCB_PADSTACK_LOOP(data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_DRC, padstack))
			PCB_FLAG_CLEAR(PCB_FLAG_DRC, padstack);
		else
			AddPstk(layergroupboxes, padstack, rd->styles[rd->max_styles]);
	}
	PCB_END_LOOP;

	for (i = 0; i < data->LayerN; i++) {
		pcb_layergrp_id_t layergroup;
		pcb_layer_t *ly = &data->Layer[i];

		if (!(pcb_layer_flags_(ly) & PCB_LYT_COPPER))
			continue;

		layergroup = pcb_layer_get_group_(ly);

		/* add all (non-rat) lines */
		PCB_LINE_LOOP(ly);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_DRC, line)) {
				PCB_FLAG_CLEAR(PCB_FLAG_DRC, line);
				continue;
			}
			/* dice up non-straight lines into many tiny obstacles */
			if (line->Point1.X != line->Point2.X && line->Point1.Y != line->Point2.Y) {
				pcb_line_t fake_line = *line;
				pcb_coord_t dx = (line->Point2.X - line->Point1.X);
				pcb_coord_t dy = (line->Point2.Y - line->Point1.Y);
				int segs = MAX(PCB_ABS(dx), PCB_ABS(dy)) / (4 * rd->max_bloat + 1);
				int qq;
				segs = PCB_CLAMP(segs, 1, 32);	/* don't go too crazy */
				dx /= segs;
				dy /= segs;
				for (qq = 0; qq < segs - 1; qq++) {
					fake_line.Point2.X = fake_line.Point1.X + dx;
					fake_line.Point2.Y = fake_line.Point1.Y + dy;
					if (fake_line.Point2.X == line->Point2.X && fake_line.Point2.Y == line->Point2.Y)
						break;
					AddLine(layergroupboxes, layergroup, &fake_line, line, rd->styles[rd->max_styles]);
					fake_line.Point1 = fake_line.Point2;
				}
				fake_line.Point2 = line->Point2;
				AddLine(layergroupboxes, layergroup, &fake_line, line, rd->styles[rd->max_styles]);
			}
			else {
				AddLine(layergroupboxes, layergroup, line, line, rd->styles[rd->max_styles]);
			}
		}
		PCB_END_LOOP;
		/* add all polygons */
		PCB_POLY_LOOP(ly);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_DRC, polygon))
				PCB_FLAG_CLEAR(PCB_FLAG_DRC, polygon);
			else
				AddPolygon(layergroupboxes, i, polygon, rd->styles[rd->max_styles]);
		}
		PCB_END_LOOP;
		/* add all copper text */
		PCB_TEXT_LOOP(ly);
		{
			AddText(layergroupboxes, layergroup, text, rd->styles[rd->max_styles]);
		}
		PCB_END_LOOP;
		/* add all arcs */
		PCB_ARC_LOOP(ly);
		{
			AddArc(layergroupboxes, layergroup, arc, rd->styles[rd->max_styles]);
		}
		PCB_END_LOOP;
	}

	/* subc recursion - re-add terms */
	PCB_SUBC_LOOP(data);
	{
		CreateRouteData_subc(rd, layergroupboxes, subc->data);
	}
	PCB_END_LOOP;

}

static routebox_t *crd_add_line(routedata_t *rd, vtp0_t *layergroupboxes, pcb_layergrp_id_t group, pcb_any_obj_t *obj, int j, routebox_t **last_in_net, routebox_t **last_in_subnet)
{
	routebox_t *rb = NULL;
	pcb_line_t *line = (pcb_line_t *) obj;

	/* dice up non-straight lines into many tiny obstacles */
	if (line->Point1.X != line->Point2.X && line->Point1.Y != line->Point2.Y) {
		pcb_line_t fake_line = *line;
		pcb_coord_t dx = (line->Point2.X - line->Point1.X);
		pcb_coord_t dy = (line->Point2.Y - line->Point1.Y);
		int segs = MAX(PCB_ABS(dx), PCB_ABS(dy)) / (4 * BLOAT(rd->styles[j]) + 1);
		int qq;
		segs = PCB_CLAMP(segs, 1, 32); /* don't go too crazy */
		dx /= segs;
		dy /= segs;
		for (qq = 0; qq < segs - 1; qq++) {
			fake_line.Point2.X = fake_line.Point1.X + dx;
			fake_line.Point2.Y = fake_line.Point1.Y + dy;
			if (fake_line.Point2.X == line->Point2.X && fake_line.Point2.Y == line->Point2.Y)
				break;
			rb = AddLine(layergroupboxes, group, &fake_line, line, rd->styles[j]);
			if (*last_in_subnet && rb != *last_in_subnet)
				MergeNets(*last_in_subnet, rb, ORIGINAL);
			if (*last_in_net && rb != *last_in_net)
				MergeNets(*last_in_net, rb, NET);
			*last_in_subnet = *last_in_net = rb;
			fake_line.Point1 = fake_line.Point2;
		}
		fake_line.Point2 = line->Point2;
		rb = AddLine(layergroupboxes, group, &fake_line, line, rd->styles[j]);
	}
	else
		rb = AddLine(layergroupboxes, group, line, line, rd->styles[j]);

	return rb;
}

static routebox_t *crd_add_misc(routedata_t *rd, vtp0_t *layergroupboxes, pcb_any_obj_t *obj, int j)
{
	routebox_t *rb = NULL;

	switch (obj->type) {
	case PCB_OBJ_VOID: break;
	case PCB_OBJ_PSTK:
		rb = AddPstk(layergroupboxes, (pcb_pstk_t *)obj, rd->styles[j]);
		break;
	case PCB_OBJ_POLY:
		{
			pcb_poly_t *poly = (pcb_poly_t *)obj;
			pcb_layer_t *layer = obj->parent.layer;
			if (poly->term != NULL)
				rb = AddTerm(layergroupboxes, obj, rd->styles[j]);
			else
				rb = AddPolygon(layergroupboxes, pcb_layer_id(PCB->Data, layer), poly, rd->styles[j]);
		}
		break;
	case PCB_OBJ_LINE:
	case PCB_OBJ_TEXT:
	case PCB_OBJ_ARC:
		if (obj->term != NULL)
			rb = AddTerm(layergroupboxes, obj, rd->styles[j]);
		break;

	case PCB_OBJ_RAT:
	case PCB_OBJ_SUBC:
	case PCB_OBJ_NET:
	case PCB_OBJ_NET_TERM:
	case PCB_OBJ_LAYER:
	case PCB_OBJ_LAYERGRP:
		break; /* don't care about these */
	}
	return rb;
}

static void CreateRouteData_nets_old(routedata_t *rd, vtp0_t *layergroupboxes)
{
	pcb_netlist_list_t Nets;

	/* add the objects in the netlist first.
	 * then go and add all other objects that weren't already added
	 *
	 * this saves on searching the trees to find the nets
	 */
	/* use the PCB_FLAG_DRC to mark objects as they are entered */
	Nets = pcb_rat_collect_subnets(pcb_false);
	{
		routebox_t *last_net = NULL;
		PCB_NETLIST_LOOP(&Nets);
		{
			routebox_t *last_in_net = NULL;
			PCB_NET_LOOP(netlist);
			{
				routebox_t *last_in_subnet = NULL;
				int j;

				for (j = 0; j < rd->max_styles; j++)
					if (net->Style == rd->styles[j])
						break;
				PCB_CONNECTION_LOOP(net);
				{
					routebox_t *rb = NULL;
					PCB_FLAG_SET(PCB_FLAG_DRC, (pcb_pstk_t *)connection->obj);

					if ((connection->obj->type == PCB_OBJ_LINE) && (connection->obj->term != NULL))
						rb = AddTerm(layergroupboxes, connection->obj, rd->styles[j]);
					else if (connection->obj->type == PCB_OBJ_LINE) {
						/* lines are listed at each end, so skip one */
						/* this should probably by a macro named "BUMP_LOOP" */
						if (connection->obj->term == NULL)
							n--;
						rb = crd_add_line(rd, layergroupboxes, connection->group, connection->obj, j, &last_in_net, &last_in_subnet);
					}
					else
						rb = crd_add_misc(rd, layergroupboxes, connection->obj, j);

					assert(rb);

					/* update circular connectivity lists */
					if (last_in_subnet && rb != last_in_subnet)
						MergeNets(last_in_subnet, rb, ORIGINAL);
					if (last_in_net && rb != last_in_net)
						MergeNets(last_in_net, rb, NET);
					last_in_subnet = last_in_net = rb;
					rd->max_bloat = MAX(rd->max_bloat, BLOAT(rb->style));
					rd->max_keep = MAX(rd->max_keep, rb->style->Clearance);
				}
				PCB_END_LOOP;
			}
			PCB_END_LOOP;
			if (last_net && last_in_net)
				MergeNets(last_net, last_in_net, DIFFERENT_NET);
			last_net = last_in_net;
		}
		PCB_END_LOOP;
		rd->first_net = last_net;
	}
	pcb_netlist_list_free(&Nets);

	/* reset all nets to "original" connectivity (which we just set) */
	{
		routebox_t *net;
		LIST_LOOP(rd->first_net, different_net, net);
		ResetSubnet(net);
		PCB_END_LOOP;
	}
}

static void CreateRouteData_subnet(routedata_t *rd, vtp0_t *layergroupboxes, vtp0_t *objs, routebox_t **last_in_net, int j)
{
	routebox_t *last_in_subnet = NULL;
	size_t oi;

	for(oi = 0; oi < vtp0_len(objs); oi++) {
		routebox_t *rb = NULL;
		pcb_any_obj_t *obj = objs->array[oi];

		if (obj->type == PCB_OBJ_RAT)
			continue;

		PCB_FLAG_SET(PCB_FLAG_DRC, obj);

		if ((obj->type == PCB_OBJ_LINE) && (obj->term != NULL))
			rb = AddTerm(layergroupboxes, obj, rd->styles[j]);
		else if (obj->type == PCB_OBJ_LINE) {
			pcb_layer_t *layer = pcb_layer_get_real(obj->parent.layer);
			pcb_layergrp_id_t group = layer->meta.real.grp;
			rb = crd_add_line(rd, layergroupboxes, group, obj, j, last_in_net, &last_in_subnet);
		}
		else
			rb = crd_add_misc(rd, layergroupboxes, obj, j);

		assert(rb);

		/* update circular connectivity lists */
		if (last_in_subnet && rb != last_in_subnet)
			MergeNets(last_in_subnet, rb, ORIGINAL);
		if (*last_in_net && rb != *last_in_net)
			MergeNets(*last_in_net, rb, NET);
		last_in_subnet = *last_in_net = rb;
		rd->max_bloat = MAX(rd->max_bloat, BLOAT(rb->style));
		rd->max_keep = MAX(rd->max_keep, rb->style->Clearance);
	}
}

static void CreateRouteData_nets(routedata_t *rd, vtp0_t *layergroupboxes)
{
	htsp_entry_t *e;
	pcb_short_ctx_t sctx;
	vtp0_t subnets;
	routebox_t *last_net = NULL;

	vtp0_init(&subnets);
	pcb_net_short_ctx_init(&sctx, PCB, NULL);

	for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e)) {
		size_t sni;
		routebox_t *last_in_net = NULL;
		pcb_net_t *net = e->value;
		const char *style = pcb_attribute_get(&net->Attributes, "style");
		int j;

		if (style == NULL)
			style = "";

		for (j = 0; j < rd->max_styles; j++)
			if (strcmp(style, rd->styles[j]->name) == 0)
				break;

		sctx.current_net = (pcb_net_t *)e->value;
		pcb_net_map_subnets(&sctx, PCB_RATACC_ONLY_MANHATTAN, &subnets);
		for(sni = 0; sni < vtp0_len(&subnets); sni++)
			CreateRouteData_subnet(rd, layergroupboxes, subnets.array[sni], &last_in_net, j);
		pcb_net_reset_subnets(&subnets);

		if (last_net && last_in_net)
			MergeNets(last_net, last_in_net, DIFFERENT_NET);
		last_net = last_in_net;
	}
	rd->first_net = last_net;

	vtp0_uninit(&subnets);
	pcb_net_short_ctx_uninit(&sctx);

	/* reset all nets to "original" connectivity (which we just set) */
	{
		routebox_t *net;
		LIST_LOOP(rd->first_net, different_net, net);
		ResetSubnet(net);
		PCB_END_LOOP;
	}
}

static routedata_t *CreateRouteData()
{
	vtp0_t layergroupboxes[PCB_MAX_LAYERGRP];
	pcb_box_t bbox;
	routedata_t *rd;
	int group, i;

	/* check which layers are active first */
	routing_layers = 0;
	for (group = 0; group < pcb_max_group(PCB); group++) {
		for (i = 0; i < PCB->LayerGroups.grp[group].len; i++) {
			pcb_layer_id_t lid = PCB->LayerGroups.grp[group].lid[i];
			/* layer must be 1) copper and 2) on */
			if ((pcb_layer_flags(PCB, lid) & PCB_LYT_COPPER) && PCB->Data->Layer[lid].meta.real.vis) {
				routing_layers++;
				is_layer_group_active[group] = pcb_true;
				break;
			}
			else
				is_layer_group_active[group] = pcb_false;
		}
	}
	/* if via visibility is turned off, don't use them */
	AutoRouteParameters.use_vias = routing_layers > 1 && PCB->pstk_on;

	back = front = -1;
	if (pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &back, 1) <= 0)
		return NULL;
	if (pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &front, 1) <= 0)
		return NULL;

	/* determine preferred routing direction on each group */
	for (i = 0; i < pcb_max_group(PCB); i++) {
		if (i != back && i != front) {
			x_cost[i] = (i & 1) ? 2 : 1;
			y_cost[i] = (i & 1) ? 1 : 2;
		}
		else if (i == back) {
			x_cost[i] = 4;
			y_cost[i] = 2;
		}
		else {
			x_cost[i] = 2;
			y_cost[i] = 2;
		}
	}
	/* create routedata */
	rd = (routedata_t *) malloc(sizeof(*rd));
	memset((void *) rd, 0, sizeof(*rd));

	rd->max_styles = vtroutestyle_len(&PCB->RouteStyle);
/*	rd->layergrouptree = calloc(sizeof(rd->layergrouptree[0]), rd->max_layers);*/
	rd->styles = calloc(sizeof(rd->styles[0]), rd->max_styles+1);

	/* create default style */
	rd->defaultstyle.Thick = conf_core.design.line_thickness;
	rd->defaultstyle.Diameter = conf_core.design.via_thickness;
	rd->defaultstyle.Hole = conf_core.design.via_drilling_hole;
	rd->defaultstyle.Clearance = conf_core.design.clearance;
	rd->max_bloat = BLOAT(&rd->defaultstyle);
	rd->max_keep = conf_core.design.clearance;
	/* create styles structures */
	bbox.X1 = bbox.Y1 = 0;
	bbox.X2 = PCB->MaxWidth;
	bbox.Y2 = PCB->MaxHeight;
	for (i = 0; i < rd->max_styles + 1; i++) {
		pcb_route_style_t *style = (i < rd->max_styles) ? &PCB->RouteStyle.array[i] : &rd->defaultstyle;
		rd->styles[i] = style;
	}

	/* initialize pointer vectors */
	for (i = 0; i < pcb_max_group(PCB); i++) {
		vtp0_init(&layergroupboxes[i]);
		PCB_COPPER_GROUP_LOOP(PCB->Data, i);
		{
			if (!PCB_RTREE_EMPTY(layer->line_tree) || !PCB_RTREE_EMPTY(layer->arc_tree))
				usedGroup[i] = pcb_true;
			else
				usedGroup[i] = pcb_false;
		}
		PCB_END_LOOP;
	}
	usedGroup[front] = pcb_true;
	usedGroup[back] = pcb_true;

	if (pcb_brave & PCB_BRAVE_NETLIST2)
		CreateRouteData_nets(rd, layergroupboxes);
	else
		CreateRouteData_nets_old(rd, layergroupboxes);

	/* subc-recursively add all obstacles */
	CreateRouteData_subc(rd, layergroupboxes, PCB->Data);

	/* create r-trees from pointer lists */
	for (i = 0; i < pcb_max_group(PCB); i++) {
		/* create the r-tree */
		rd->layergrouptree[i] = pcb_r_create_tree();
		pcb_r_insert_array(rd->layergrouptree[i], (const pcb_box_t **) layergroupboxes[i].array, vtp0_len(&layergroupboxes[i]));
	}

	if (AutoRouteParameters.use_vias) {
		rd->mtspace = mtspace_create();

		/* create "empty-space" structures for via placement (now that we know
		 * appropriate clearances for all the fixed elements) */
		for (i = 0; i < pcb_max_group(PCB); i++) {
			int ip;
			for(ip = 0; ip < vtp0_len(&layergroupboxes[i]); ip++) {
				void **ptr = &layergroupboxes[i].array[ip];
				routebox_t *rb = (routebox_t *) * ptr;
				if (!rb->flags.clear_poly)
					mtspace_add(rd->mtspace, &rb->box, FIXED, rb->style->Clearance);
			}
		}
	}
	/* free pointer lists */
	for (i = 0; i < pcb_max_group(PCB); i++)
		vtp0_uninit(&layergroupboxes[i]);
	/* done! */
	return rd;
}

void DestroyRouteData(routedata_t ** rd)
{
	int i;
	for (i = 0; i < pcb_max_group(PCB); i++) {
		pcb_r_free_tree_data((*rd)->layergrouptree[i], free);
		pcb_r_destroy_tree(&(*rd)->layergrouptree[i]);
	}
	if (AutoRouteParameters.use_vias)
		mtspace_destroy(&(*rd)->mtspace);
/*	free((*rd)->layergrouptree);*/
	free((*rd)->styles);
	free(*rd);
	*rd = NULL;
}

/*-----------------------------------------------------------------
 * routebox reference counting.
 */

/* increment the reference count on a routebox. */
static void RB_up_count(routebox_t * rb)
{
	assert(rb->flags.homeless);
	rb->refcount++;
}

/* decrement the reference count on a routebox, freeing if this box becomes
 * unused. */
static void RB_down_count(routebox_t * rb)
{
	assert(rb->type == EXPANSION_AREA);
	assert(rb->flags.homeless);
	assert(rb->refcount > 0);
	if (--rb->refcount == 0) {
		if (rb->parent.expansion_area->flags.homeless)
			RB_down_count(rb->parent.expansion_area);
		free(rb);
	}
}

/*-----------------------------------------------------------------
 * Rectangle-expansion routing code.
 */

static void ResetSubnet(routebox_t * net)
{
	routebox_t *rb;
	/* reset connectivity of everything on this net */
	LIST_LOOP(net, same_net, rb);
	rb->same_subnet = rb->original_subnet;
	PCB_END_LOOP;
}

static inline pcb_cost_t pcb_cost_to_point_on_layer(const pcb_cheap_point_t * p1, const pcb_cheap_point_t * p2, pcb_cardinal_t point_layer)
{
	pcb_cost_t x_dist = p1->X - p2->X, y_dist = p1->Y - p2->Y, r;
	x_dist *= x_cost[point_layer];
	y_dist *= y_cost[point_layer];
	/* cost is proportional to orthogonal distance. */
	r = PCB_ABS(x_dist) + PCB_ABS(y_dist);
	if (p1->X != p2->X && p1->Y != p2->Y)
		r += AutoRouteParameters.JogPenalty;
	return r;
}

static pcb_cost_t pcb_cost_to_point(const pcb_cheap_point_t * p1, pcb_cardinal_t point_layer1, const pcb_cheap_point_t * p2, pcb_cardinal_t point_layer2)
{
	pcb_cost_t r = pcb_cost_to_point_on_layer(p1, p2, point_layer1);
	/* apply via cost penalty if layers differ */
	if (point_layer1 != point_layer2)
		r += AutoRouteParameters.ViaCost;
	return r;
}

/* return the minimum *cost* from a point to a box on any layer.
 * It's safe to return a smaller than minimum cost
 */
static pcb_cost_t pcb_cost_to_layerless_box(const pcb_cheap_point_t * p, pcb_cardinal_t point_layer, const pcb_box_t * b)
{
	pcb_cheap_point_t p2 = pcb_closest_pcb_point_in_box(p, b);
	register pcb_cost_t c1, c2;

	c1 = p2.X - p->X;
	c2 = p2.Y - p->Y;

	c1 = PCB_ABS(c1);
	c2 = PCB_ABS(c2);
	if (c1 < c2)
		return c1 * AutoRouteParameters.MinPenalty + c2;
	else
		return c2 * AutoRouteParameters.MinPenalty + c1;
}

/* get to actual pins/pad target coordinates */
pcb_bool TargetPoint(pcb_cheap_point_t * nextpoint, const routebox_t * target)
{
/*	if (target->type == PIN) {
		nextpoint->X = target->parent.pin->X;
		nextpoint->Y = target->parent.pin->Y;
		return pcb_true;
	}*/
	nextpoint->X = PCB_BOX_CENTER_X(target->sbox);
	nextpoint->Y = PCB_BOX_CENTER_Y(target->sbox);
	return pcb_false;
}

/* return the *minimum cost* from a point to a route box, including possible
 * via costs if the route box is on a different layer.
 * assume routbox is bloated unless it is an expansion area
 */
static pcb_cost_t pcb_cost_to_routebox(const pcb_cheap_point_t * p, pcb_cardinal_t point_layer, const routebox_t * rb)
{
	register pcb_cost_t trial = 0;
	pcb_cheap_point_t p2 = closest_point_in_routebox(p, rb);
	if (!usedGroup[point_layer] || !usedGroup[rb->group])
		trial = AutoRouteParameters.NewLayerPenalty;
	if ((p2.X - p->X) * (p2.Y - p->Y) != 0)
		trial += AutoRouteParameters.JogPenalty;
	/* special case for defered via searching */
	if (point_layer > pcb_max_group(PCB) || point_layer == rb->group)
		return trial + PCB_ABS(p2.X - p->X) + PCB_ABS(p2.Y - p->Y);
	/* if this target is only a via away, then the via is cheaper than the congestion */
	if (p->X == p2.X && p->Y == p2.Y)
		return trial + 1;
	trial += AutoRouteParameters.ViaCost;
	trial += PCB_ABS(p2.X - p->X) + PCB_ABS(p2.Y - p->Y);
	return trial;
}


static pcb_box_t bloat_routebox(routebox_t * rb)
{
	pcb_box_t r;
	pcb_coord_t clearance;
	assert(__routepcb_box_is_good(rb));

	if (rb->flags.nobloat)
		return rb->sbox;

	/* Obstacle exclusion zones get bloated by the larger of
	 * the two required clearances plus half the track width.
	 */
	clearance = MAX(AutoRouteParameters.style->Clearance, rb->style->Clearance);
	r = pcb_bloat_box(&rb->sbox, clearance + HALF_THICK(AutoRouteParameters.style->Thick));
	return r;
}


#ifdef ROUTE_DEBUG  /* only for debugging expansion areas */

typedef short pcb_dimension_t;
/* makes a line on the solder layer silk surrounding the box */
static void showbox(pcb_box_t b, pcb_dimension_t thickness, int group)
{
	pcb_line_t *line;
	pcb_layer_t *csl, *SLayer = LAYER_PTR(group);
	pcb_layer_id_t cs_id;
	if (showboxen < -1)
		return;
	if (showboxen != -1 && showboxen != group)
		return;

	if (ddraw != NULL) {
		ddraw->set_line_width(ar_gc, thickness);
		ddraw->set_line_cap(ar_gc, pcb_cap_round);
		ddraw->set_color(ar_gc, SLayer->Color);

		ddraw->draw_line(ar_gc, b.X1, b.Y1, b.X2, b.Y1);
		ddraw->draw_line(ar_gc, b.X1, b.Y2, b.X2, b.Y2);
		ddraw->draw_line(ar_gc, b.X1, b.Y1, b.X1, b.Y2);
		ddraw->draw_line(ar_gc, b.X2, b.Y1, b.X2, b.Y2);
	}

#if 1
	if (pcb_layer_find(PCB_LYT_TOP | PCB_LYT_SILK, &cs_id, 1) > 0)  {
		csl = LAYER_PTR(cs_id);
		if (b.Y1 == b.Y2 || b.X1 == b.X2)
			thickness = 5;
		line = pcb_line_new(csl, b.X1, b.Y1, b.X2, b.Y1, thickness, 0, pcb_flag_make(0));
		pcb_undo_add_obj_to_create(csl, line, line);
		if (b.Y1 != b.Y2) {
			line = pcb_line_new(csl, b.X1, b.Y2, b.X2, b.Y2, thickness, 0, pcb_flag_make(0));
			pcb_undo_add_obj_to_create(PCB_OBJ_LINE, csl, line, line);
		}
		line = pcb_line_new(csl, b.X1, b.Y1, b.X1, b.Y2, thickness, 0, pcb_flag_make(0));
		pcb_undo_add_obj_to_create(PCB_OBJ_LINE, csl, line, line);
		if (b.X1 != b.X2) {
			line = pcb_line_new(csl, b.X2, b.Y1, b.X2, b.Y2, thickness, 0, pcb_flag_make(0));
			pcb_undo_add_obj_to_create(PCB_OBJ_LINE, csl, line, line);
		}
	}
#endif
}
#endif

#if defined(ROUTE_DEBUG)
static void showedge(edge_t * e)
{
	pcb_box_t *b = (pcb_box_t *) e->rb;

	if (ddraw == NULL)
		return;

	ddraw->set_line_cap(ar_gc, pcb_cap_round);
	ddraw->set_line_width(ar_gc, 1);
	ddraw->set_color(ar_gc, Settings.MaskColor);

	switch (e->expand_dir) {
	case PCB_NORTH:
		ddraw->draw_line(ar_gc, b->X1, b->Y1, b->X2, b->Y1);
		break;
	case PCB_SOUTH:
		ddraw->draw_line(ar_gc, b->X1, b->Y2, b->X2, b->Y2);
		break;
	case PCB_WEST:
		ddraw->draw_line(ar_gc, b->X1, b->Y1, b->X1, b->Y2);
		break;
	case PCB_EAST:
		ddraw->draw_line(ar_gc, b->X2, b->Y1, b->X2, b->Y2);
		break;
	default:
		break;
	}
}
#endif

#if defined(ROUTE_DEBUG)
static void showroutebox(routebox_t * rb)
{
	pcb_layerid_t cs_id;
	if (pcb_layer_find(PCB_LYT_TOP | PCB_LYT_SILK, &cs_id, 1) > 0)
		showbox(rb->sbox, rb->flags.source ? 20 : (rb->flags.target ? 10 : 1), rb->flags.is_via ? LAYER_PTR(cs_id) : rb->group);
}
#endif

/* return a "parent" of this edge which immediately precedes it in the route.*/
static routebox_t *route_parent(routebox_t * rb)
{
	while (rb->flags.homeless && !rb->flags.is_via && !rb->flags.is_thermal) {
		assert(rb->type == EXPANSION_AREA);
		rb = rb->parent.expansion_area;
		assert(rb);
	}
	return rb;
}

static vector_t *path_conflicts(routebox_t * rb, routebox_t * conflictor, pcb_bool branch)
{
	if (branch)
		rb->conflicts_with = vector_duplicate(rb->conflicts_with);
	else if (!rb->conflicts_with)
		rb->conflicts_with = vector_create();
	vector_append(rb->conflicts_with, conflictor);
	return rb->conflicts_with;
}

/* Touch everything (except fixed) on each net found
 * in the conflicts vector. If the vector is different
 * from the last one touched, untouch the last batch
 * and touch the new one. Always call with touch=1
 * (except for recursive call). Call with NULL, 1 to
 * clear the last batch touched.
 *
 * touched items become invisible to current path
 * so we don't encounter the same conflictor more
 * than once
 */

static void touch_conflicts(vector_t * conflicts, int touch)
{
	static vector_t *last = NULL;
	static int last_size = 0;
	int i, n;
	i = 0;
	if (touch) {
		if (last && conflicts != last)
			touch_conflicts(last, 0);
		if (!conflicts)
			return;
		last = conflicts;
		i = last_size;
	}
	n = vector_size(conflicts);
	for (; i < n; i++) {
		routebox_t *rb = (routebox_t *) vector_element(conflicts, i);
		routebox_t *p;
		LIST_LOOP(rb, same_net, p);
		if (!p->flags.fixed)
			p->flags.touched = touch;
		PCB_END_LOOP;
	}
	if (!touch) {
		last = NULL;
		last_size = 0;
	}
	else
		last_size = n;
}

/* return a "parent" of this edge which resides in a r-tree somewhere */
/* -- actually, this "parent" *may* be a via box, which doesn't live in
 * a r-tree. -- */
static routebox_t *nonhomeless_parent(routebox_t * rb)
{
	return route_parent(rb);
}

/* some routines to find the minimum *cost* from a cost point to
 * a target (any target) */
struct minpcb_cost_target_closure {
	const pcb_cheap_point_t *CostPoint;
	pcb_cardinal_t CostPointLayer;
	routebox_t *nearest;
	pcb_cost_t nearest_cost;
};
static pcb_r_dir_t __region_within_guess(const pcb_box_t * region, void *cl)
{
	struct minpcb_cost_target_closure *mtc = (struct minpcb_cost_target_closure *) cl;
	pcb_cost_t pcb_cost_to_region;
	if (mtc->nearest == NULL)
		return PCB_R_DIR_FOUND_CONTINUE;
	pcb_cost_to_region = pcb_cost_to_layerless_box(mtc->CostPoint, mtc->CostPointLayer, region);
	assert(pcb_cost_to_region >= 0);
	/* if no guess yet, all regions are "close enough" */
	/* note that cost is *strictly more* than minimum distance, so we'll
	 * always search a region large enough. */
	return (pcb_cost_to_region < mtc->nearest_cost) ? PCB_R_DIR_FOUND_CONTINUE : PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t __found_new_guess(const pcb_box_t * box, void *cl)
{
	struct minpcb_cost_target_closure *mtc = (struct minpcb_cost_target_closure *) cl;
	routebox_t *guess = (routebox_t *) box;
	pcb_cost_t pcb_cost_to_guess = pcb_cost_to_routebox(mtc->CostPoint, mtc->CostPointLayer, guess);
	assert(pcb_cost_to_guess >= 0);
	/* if this is cheaper than previous guess... */
	if (pcb_cost_to_guess < mtc->nearest_cost) {
		mtc->nearest = guess;
		mtc->nearest_cost = pcb_cost_to_guess;	/* this is our new guess! */
		return PCB_R_DIR_FOUND_CONTINUE;
	}
	else
		return PCB_R_DIR_NOT_FOUND;										/* not less expensive than our last guess */
}

/* target_guess is our guess at what the nearest target is, or NULL if we
 * just plum don't have a clue. */
static routebox_t *minpcb_cost_target_to_point(const pcb_cheap_point_t * CostPoint,
																					 pcb_cardinal_t CostPointLayer, pcb_rtree_t * targets, routebox_t * target_guess)
{
	struct minpcb_cost_target_closure mtc;
	assert(target_guess == NULL || target_guess->flags.target);	/* this is a target, right? */
	mtc.CostPoint = CostPoint;
	mtc.CostPointLayer = CostPointLayer;
	mtc.nearest = target_guess;
	if (mtc.nearest)
		mtc.nearest_cost = pcb_cost_to_routebox(mtc.CostPoint, mtc.CostPointLayer, mtc.nearest);
	else
		mtc.nearest_cost = EXPENSIVE;
	pcb_r_search(targets, NULL, __region_within_guess, __found_new_guess, &mtc, NULL);
	assert(mtc.nearest != NULL && mtc.nearest_cost >= 0);
	assert(mtc.nearest->flags.target);	/* this is a target, right? */
	return mtc.nearest;
}

/* create edge from field values */
/* minpcb_cost_target_guess can be NULL */
static edge_t *CreateEdge(routebox_t * rb,
													pcb_coord_t CostPointX, pcb_coord_t CostPointY,
													pcb_cost_t pcb_cost_to_point, routebox_t * minpcb_cost_target_guess, pcb_direction_t expand_dir, pcb_rtree_t * targets)
{
	edge_t *e;
	assert(__routepcb_box_is_good(rb));
	e = (edge_t *) malloc(sizeof(*e));
	memset((void *) e, 0, sizeof(*e));
	assert(e);
	e->rb = rb;
	if (rb->flags.homeless)
		RB_up_count(rb);
	e->cost_point.X = CostPointX;
	e->cost_point.Y = CostPointY;
	e->pcb_cost_to_point = pcb_cost_to_point;
	e->flags.via_search = 0;
	/* if this edge is created in response to a target, use it */
	if (targets)
		e->minpcb_cost_target = minpcb_cost_target_to_point(&e->cost_point, rb->group, targets, minpcb_cost_target_guess);
	else
		e->minpcb_cost_target = minpcb_cost_target_guess;
	e->expand_dir = expand_dir;
	assert(e->rb && e->minpcb_cost_target);	/* valid edge? */
	assert(!e->flags.is_via || e->expand_dir == PCB_ANY_DIR);
	/* cost point should be on edge (unless this is a plane/via/conflict edge) */
#if 0
	assert(rb->type == PLANE || rb->conflicts_with != NULL || rb->flags.is_via
				 || rb->flags.is_thermal
				 || ((expand_dir == PCB_NORTH || expand_dir == PCB_SOUTH) ? rb->sbox.X1 <=
						 CostPointX && CostPointX < rb->sbox.X2 && CostPointY == (expand_dir == PCB_NORTH ? rb->sbox.Y1 : rb->sbox.Y2 - 1) :
						 /* expand_dir==EAST || expand_dir==WEST */
						 rb->sbox.Y1 <= CostPointY && CostPointY < rb->sbox.Y2 &&
						 CostPointX == (expand_dir == PCB_EAST ? rb->sbox.X2 - 1 : rb->sbox.X1)));
#endif
	assert(__edge_is_good(e));
	/* done */
	return e;
}

/* create edge, using previous edge to fill in defaults. */
/* most of the work here is in determining a new cost point */
static edge_t *CreateEdge2(routebox_t * rb, pcb_direction_t expand_dir,
													 edge_t * previous_edge, pcb_rtree_t * targets, routebox_t * guess)
{
	pcb_box_t thisbox;
	pcb_cheap_point_t thiscost, prevcost;
	pcb_cost_t d;

	assert(rb && previous_edge);
	/* okay, find cheapest costpoint to costpoint of previous edge */
	thisbox = edge_to_box(rb, expand_dir);
	prevcost = previous_edge->cost_point;
	/* find point closest to target */
	thiscost = pcb_closest_pcb_point_in_box(&prevcost, &thisbox);
	/* compute cost-to-point */
	d = pcb_cost_to_point_on_layer(&prevcost, &thiscost, rb->group);
	/* add in jog penalty */
	if (previous_edge->expand_dir != expand_dir)
		d += AutoRouteParameters.JogPenalty;
	/* okay, new edge! */
	return CreateEdge(rb, thiscost.X, thiscost.Y,
										previous_edge->pcb_cost_to_point + d, guess ? guess : previous_edge->minpcb_cost_target, expand_dir, targets);
}

/* create via edge, using previous edge to fill in defaults. */
static edge_t *CreateViaEdge(const pcb_box_t * area, pcb_cardinal_t group,
														 routebox_t * parent, edge_t * previous_edge,
														 conflict_t to_site_conflict, conflict_t through_site_conflict, pcb_rtree_t * targets)
{
	routebox_t *rb;
	pcb_cheap_point_t costpoint;
	pcb_cost_t d;
	edge_t *ne;
	pcb_cost_t scale[3];

	scale[0] = 1;
	scale[1] = AutoRouteParameters.LastConflictPenalty;
	scale[2] = AutoRouteParameters.ConflictPenalty;

	assert(pcb_box_is_good(area));
	assert(AutoRouteParameters.with_conflicts || (to_site_conflict == NO_CONFLICT && through_site_conflict == NO_CONFLICT));
	rb = CreateExpansionArea(area, group, parent, pcb_true, previous_edge);
	rb->flags.is_via = 1;
	rb->came_from = PCB_ANY_DIR;
#if defined(ROUTE_DEBUG) && defined(DEBUG_SHOW_VIA_BOXES)
	showroutebox(rb);
#endif /* ROUTE_DEBUG && DEBUG_SHOW_VIA_BOXES */
	/* for planes, choose a point near the target */
	if (previous_edge->flags.in_plane) {
		routebox_t *target;
		pcb_cheap_point_t pnt;
		/* find a target near this via box */
		pnt.X = PCB_BOX_CENTER_X(*area);
		pnt.Y = PCB_BOX_CENTER_Y(*area);
		target = minpcb_cost_target_to_point(&pnt, rb->group, targets, previous_edge->minpcb_cost_target);
		/* now find point near the target */
		pnt.X = PCB_BOX_CENTER_X(target->box);
		pnt.Y = PCB_BOX_CENTER_Y(target->box);
		costpoint = closest_point_in_routebox(&pnt, rb);
		/* we moved from the previous cost point through the plane which is free travel */
		d = (scale[through_site_conflict] * pcb_cost_to_point(&costpoint, group, &costpoint, previous_edge->rb->group));
		ne = CreateEdge(rb, costpoint.X, costpoint.Y, previous_edge->pcb_cost_to_point + d, target, PCB_ANY_DIR, NULL);
		ne->minpcb_cost_target = target;
	}
	else {
		routebox_t *target;
		target = previous_edge->minpcb_cost_target;
		costpoint = closest_point_in_routebox(&previous_edge->cost_point, rb);
		d =
			(scale[to_site_conflict] *
			 pcb_cost_to_point_on_layer(&costpoint, &previous_edge->cost_point,
															previous_edge->rb->group)) +
			(scale[through_site_conflict] * pcb_cost_to_point(&costpoint, group, &costpoint, previous_edge->rb->group));
		/* if the target is just this via away, then this via is cheaper */
		if (target->group == group && point_in_shrunk_box(target, costpoint.X, costpoint.Y))
			d -= AutoRouteParameters.ViaCost / 2;
		ne =
			CreateEdge(rb, costpoint.X, costpoint.Y, previous_edge->pcb_cost_to_point + d, previous_edge->minpcb_cost_target, PCB_ANY_DIR, targets);
	}
	ne->flags.is_via = 1;
	ne->flags.via_conflict_level = to_site_conflict;
	assert(__edge_is_good(ne));
	return ne;
}

/* create "interior" edge for routing with conflicts */
/* Presently once we "jump inside" the conflicting object
 * we consider it a routing highway to travel inside since
 * it will become available if the conflict is elliminated.
 * That is why we ignore the interior_edge argument.
 */
static edge_t *CreateEdgeWithConflicts(const pcb_box_t * interior_edge,
																			 routebox_t * container, edge_t * previous_edge,
																			 pcb_cost_t cost_penalty_to_box, pcb_rtree_t * targets)
{
	routebox_t *rb;
	pcb_cheap_point_t costpoint;
	pcb_cost_t d;
	edge_t *ne;
	assert(interior_edge && container && previous_edge && targets);
	assert(!container->flags.homeless);
	assert(AutoRouteParameters.with_conflicts);
	assert(container->flags.touched == 0);
	assert(previous_edge->rb->group == container->group);
	/* use the caller's idea of what this box should be */
	rb = CreateExpansionArea(interior_edge, previous_edge->rb->group, previous_edge->rb, pcb_true, previous_edge);
	path_conflicts(rb, container, pcb_true);	/* crucial! */
	costpoint = pcb_closest_pcb_point_in_box(&previous_edge->cost_point, interior_edge);
	d = pcb_cost_to_point_on_layer(&costpoint, &previous_edge->cost_point, previous_edge->rb->group);
	d *= cost_penalty_to_box;
	d += previous_edge->pcb_cost_to_point;
	ne = CreateEdge(rb, costpoint.X, costpoint.Y, d, NULL, PCB_ANY_DIR, targets);
	ne->flags.is_interior = 1;
	assert(__edge_is_good(ne));
	return ne;
}

static void KillEdge(void *edge)
{
	edge_t *e = (edge_t *) edge;
	assert(e);
	if (e->rb->flags.homeless)
		RB_down_count(e->rb);
	if (e->flags.via_search)
		mtsFreeWork(&e->work);
	free(e);
}

static void DestroyEdge(edge_t ** e)
{
	assert(e && *e);
	KillEdge(*e);
	*e = NULL;
}

/* cost function for an edge. */
static pcb_cost_t edge_cost(const edge_t * e, const pcb_cost_t too_big)
{
	pcb_cost_t penalty = e->pcb_cost_to_point;
	if (e->rb->flags.is_thermal || e->rb->type == PLANE)
		return penalty;							/* thermals are cheap */
	if (penalty > too_big)
		return penalty;

	/* pcb_cost_to_routebox adds in our via correction, too. */
	return penalty + pcb_cost_to_routebox(&e->cost_point, e->rb->group, e->minpcb_cost_target);
}

/* given an edge of a box, return a box containing exactly the points on that
 * edge.  Note that the return box is treated as closed; that is, the bottom and
 * right "edges" consist of points (just barely) not in the (half-open) box. */
static pcb_box_t edge_to_box(const routebox_t * rb, pcb_direction_t expand_dir)
{
	pcb_box_t b = shrink_routebox(rb);
	/* narrow box down to just the appropriate edge */
	switch (expand_dir) {
	case PCB_NORTH:
		b.Y2 = b.Y1 + 1;
		break;
	case PCB_EAST:
		b.X1 = b.X2 - 1;
		break;
	case PCB_SOUTH:
		b.Y1 = b.Y2 - 1;
		break;
	case PCB_WEST:
		b.X2 = b.X1 + 1;
		break;
	default:
		assert(0);
	}
	/* done! */
	return b;
}

struct broken_boxes {
	pcb_box_t left, center, right;
	pcb_bool is_valid_left, is_valid_center, is_valid_right;
};

static struct broken_boxes break_box_edge(const pcb_box_t * original, pcb_direction_t which_edge, routebox_t * breaker)
{
	pcb_box_t origbox, breakbox;
	struct broken_boxes result;

	assert(original && breaker);

	origbox = *original;
	breakbox = bloat_routebox(breaker);
	PCB_BOX_ROTATE_TO_NORTH(origbox, which_edge);
	PCB_BOX_ROTATE_TO_NORTH(breakbox, which_edge);
	result.right.Y1 = result.center.Y1 = result.left.Y1 = origbox.Y1;
	result.right.Y2 = result.center.Y2 = result.left.Y2 = origbox.Y1 + 1;
	/* validity of breaker is not important because the boxes are marked invalid */
	/*assert (breakbox.X1 <= origbox.X2 && breakbox.X2 >= origbox.X1); */
	/* left edge piece */
	result.left.X1 = origbox.X1;
	result.left.X2 = breakbox.X1;
	/* center (ie blocked) edge piece */
	result.center.X1 = MAX(breakbox.X1, origbox.X1);
	result.center.X2 = MIN(breakbox.X2, origbox.X2);
	/* right edge piece */
	result.right.X1 = breakbox.X2;
	result.right.X2 = origbox.X2;
	/* validity: */
	result.is_valid_left = (result.left.X1 < result.left.X2);
	result.is_valid_center = (result.center.X1 < result.center.X2);
	result.is_valid_right = (result.right.X1 < result.right.X2);
	/* rotate back */
	PCB_BOX_ROTATE_FROM_NORTH(result.left, which_edge);
	PCB_BOX_ROTATE_FROM_NORTH(result.center, which_edge);
	PCB_BOX_ROTATE_FROM_NORTH(result.right, which_edge);
	/* done */
	return result;
}

#ifndef NDEBUG
static int share_edge(const pcb_box_t * child, const pcb_box_t * parent)
{
	return
		(child->X1 == parent->X2 || child->X2 == parent->X1 ||
		 child->Y1 == parent->Y2 || child->Y2 == parent->Y1) &&
		((parent->X1 <= child->X1 && child->X2 <= parent->X2) || (parent->Y1 <= child->Y1 && child->Y2 <= parent->Y2));
}

static int edge_intersect(const pcb_box_t * child, const pcb_box_t * parent)
{
	return (child->X1 <= parent->X2) && (child->X2 >= parent->X1) && (child->Y1 <= parent->Y2) && (child->Y2 >= parent->Y1);
}
#endif

/* area is the expansion area, on layer group 'group'. 'parent' is the
 * immediately preceding expansion area, for backtracing. 'lastarea' is
 * the last expansion area created, we string these together in a loop
 * so we can remove them all easily at the end. */
static routebox_t *CreateExpansionArea(const pcb_box_t * area, pcb_cardinal_t group,
																			 routebox_t * parent, pcb_bool relax_edge_requirements, edge_t * src_edge)
{
	routebox_t *rb = (routebox_t *) malloc(sizeof(*rb));
	memset((void *) rb, 0, sizeof(*rb));
	assert(area && parent);
	init_const_box(rb, area->X1, area->Y1, area->X2, area->Y2, 0);
	rb->group = group;
	rb->type = EXPANSION_AREA;
	/* should always share edge or overlap with parent */
#ifndef NDEBUG
	{
		/* work around rounding errors: grow both boxes by 2 nm */
		pcb_box_t b1 = rb->sbox, b2 = parent->sbox;
		b1.X1--;b1.Y1--;b1.X2++;b1.Y2++;
		b2.X1--;b2.Y1--;b2.X2++;b2.Y2++;
		assert(relax_edge_requirements ? pcb_box_intersect(&b1, &b2)
					 : share_edge(&rb->sbox, &parent->sbox));
	}
#endif
	rb->parent.expansion_area = route_parent(parent);
	rb->cost_point = pcb_closest_pcb_point_in_box(&rb->parent.expansion_area->cost_point, area);
	rb->cost =
		rb->parent.expansion_area->cost +
		pcb_cost_to_point_on_layer(&rb->parent.expansion_area->cost_point, &rb->cost_point, rb->group);
	assert(relax_edge_requirements ? edge_intersect(&rb->sbox, &parent->sbox)
				 : share_edge(&rb->sbox, &parent->sbox));
	if (rb->parent.expansion_area->flags.homeless)
		RB_up_count(rb->parent.expansion_area);
	rb->flags.homeless = 1;
	rb->flags.nobloat = 1;
	rb->style = AutoRouteParameters.style;
	rb->conflicts_with = parent->conflicts_with;
/* we will never link an EXPANSION_AREA into the nets because they
 * are *ONLY* used for path searching. No need to call  InitLists ()
 */
	rb->came_from = src_edge->expand_dir;
#if defined(ROUTE_DEBUG) && defined(DEBUG_SHOW_EXPANSION_BOXES)
	showroutebox(rb);
#endif /* ROUTE_DEBUG && DEBUG_SHOW_EXPANSION_BOXES */
	return rb;
}

/*------ Expand ------*/
struct E_result {
	routebox_t *parent;
	routebox_t *n, *e, *s, *w;
	pcb_coord_t keep, bloat;
	pcb_box_t inflated, orig;
	int done;
};

/* test method for Expand()
 * this routebox potentially is a blocker limiting expansion
 * if this is so, we limit the inflate box so another exactly
 * like it wouldn't be seen. We do this while keep the inflated
 * box as large as possible.
 */
static pcb_r_dir_t __Expand_this_rect(const pcb_box_t * box, void *cl)
{
	struct E_result *res = (struct E_result *) cl;
	routebox_t *rb = (routebox_t *) box;
	pcb_box_t rbox;
	pcb_coord_t dn, de, ds, dw, bloat;

	/* we don't see conflicts already encountered */
	if (rb->flags.touched)
		return PCB_R_DIR_NOT_FOUND;

	/* The inflated box outer edges include its own
	 * track width plus its own clearance.
	 *
	 * To check for intersection, we need to expand
	 * anything with greater clearance by its excess
	 * clearance.
	 *
	 * If something has nobloat then we need to shrink
	 * the inflated box back and see if it still touches.
	 */

	if (rb->flags.nobloat) {
		rbox = rb->sbox;
		bloat = res->bloat;
		if (rbox.X2 <= res->inflated.X1 + bloat ||
				rbox.X1 >= res->inflated.X2 - bloat || rbox.Y1 >= res->inflated.Y2 - bloat || rbox.Y2 <= res->inflated.Y1 + bloat)
			return PCB_R_DIR_NOT_FOUND;									/* doesn't touch */
	}
	else {
		if (rb->style->Clearance > res->keep)
			rbox = pcb_bloat_box(&rb->sbox, rb->style->Clearance - res->keep);
		else
			rbox = rb->sbox;

		if (rbox.X2 <= res->inflated.X1 || rbox.X1 >= res->inflated.X2
				|| rbox.Y1 >= res->inflated.Y2 || rbox.Y2 <= res->inflated.Y1)
			return PCB_R_DIR_NOT_FOUND;									/* doesn't touch */
		bloat = 0;
	}

	/* this is an intersecting box; it has to jump through a few more hoops */
	if (rb == res->parent || rb->parent.expansion_area == res->parent)
		return PCB_R_DIR_NOT_FOUND;										/* don't see what we came from */

	/* if we are expanding a source edge, don't let other sources
	 * or their expansions stop us.
	 */
#if 1
	if (res->parent->flags.source)
		if (rb->flags.source || (rb->type == EXPANSION_AREA && rb->parent.expansion_area->flags.source))
			return PCB_R_DIR_NOT_FOUND;
#endif

	/* we ignore via expansion boxes because maybe its
	 * cheaper to get there without the via through
	 * the path we're exploring  now.
	 */
	if (rb->flags.is_via && rb->type == EXPANSION_AREA)
		return PCB_R_DIR_NOT_FOUND;

	if (rb->type == PLANE) {			/* expanding inside a plane is not good */
		if (rbox.X1 < res->orig.X1 && rbox.X2 > res->orig.X2 && rbox.Y1 < res->orig.Y1 && rbox.Y2 > res->orig.Y2) {
			res->inflated = pcb_bloat_box(&res->orig, res->bloat);
			return PCB_R_DIR_FOUND_CONTINUE;
		}
	}
	/* calculate the distances from original box to this blocker */
	dn = de = ds = dw = 0;
	if (!(res->done & _NORTH) && rbox.Y1 <= res->orig.Y1 && rbox.Y2 > res->inflated.Y1)
		dn = res->orig.Y1 - rbox.Y2;
	if (!(res->done & _EAST) && rbox.X2 >= res->orig.X2 && rbox.X1 < res->inflated.X2)
		de = rbox.X1 - res->orig.X2;
	if (!(res->done & _SOUTH) && rbox.Y2 >= res->orig.Y2 && rbox.Y1 < res->inflated.Y2)
		ds = rbox.Y1 - res->orig.Y2;
	if (!(res->done & _WEST) && rbox.X1 <= res->orig.X1 && rbox.X2 > res->inflated.X1)
		dw = res->orig.X1 - rbox.X2;
	if (dn <= 0 && de <= 0 && ds <= 0 && dw <= 0)
		return PCB_R_DIR_FOUND_CONTINUE;
	/* now shrink the inflated box to the largest blocking direction */
	if (dn >= de && dn >= ds && dn >= dw) {
		res->inflated.Y1 = rbox.Y2 - bloat;
		res->n = rb;
	}
	else if (de >= ds && de >= dw) {
		res->inflated.X2 = rbox.X1 + bloat;
		res->e = rb;
	}
	else if (ds >= dw) {
		res->inflated.Y2 = rbox.Y1 + bloat;
		res->s = rb;
	}
	else {
		res->inflated.X1 = rbox.X2 - bloat;
		res->w = rb;
	}
	return PCB_R_DIR_FOUND_CONTINUE;
}

static pcb_bool boink_box(routebox_t * rb, struct E_result *res, pcb_direction_t dir)
{
	pcb_coord_t bloat;
	if (rb->style->Clearance > res->keep)
		bloat = res->keep - rb->style->Clearance;
	else
		bloat = 0;
	if (rb->flags.nobloat)
		bloat = res->bloat;
	switch (dir) {
	case PCB_NORTH:
	case PCB_SOUTH:
		if (rb->sbox.X2 <= res->inflated.X1 + bloat || rb->sbox.X1 >= res->inflated.X2 - bloat)
			return pcb_false;
		return pcb_true;
	case PCB_EAST:
	case PCB_WEST:
		if (rb->sbox.Y1 >= res->inflated.Y2 - bloat || rb->sbox.Y2 <= res->inflated.Y1 + bloat)
			return pcb_false;
		return pcb_true;
		break;
	default:
		assert(0);
	}
	return pcb_false;
}

/* main Expand routine.
 *
 * The expansion probe edge includes the clearance and half thickness
 * as the search is performed in order to see everything relevant.
 * The result is backed off by this amount before being returned.
 * Targets (and other no-bloat routeboxes) go all the way to touching.
 * This is accomplished by backing off the probe edge when checking
 * for touch against such an object. Usually the expanding edge
 * bumps into neighboring pins on the same device that require a
 * clearance, preventing seeing a target immediately. Rather than await
 * another expansion to actually touch the target, the edge breaker code
 * looks past the clearance to see these targets even though they
 * weren't actually touched in the expansion.
 */
struct E_result *Expand(pcb_rtree_t * rtree, edge_t * e, const pcb_box_t * box)
{
	static struct E_result ans;
	int noshrink;									/* bit field of which edges to not shrink */

	ans.bloat = AutoRouteParameters.bloat;
	ans.orig = *box;
	ans.n = ans.e = ans.s = ans.w = NULL;

	/* the inflated box must be bloated in all directions that it might
	 * hit something in order to guarantee that we see object in the
	 * tree it might hit. The tree holds objects bloated by their own
	 * clearance so we are guaranteed to honor that.
	 */
	switch (e->expand_dir) {
	case PCB_ANY_DIR:
		ans.inflated.X1 = (e->rb->came_from == PCB_EAST ? ans.orig.X1 : 0);
		ans.inflated.Y1 = (e->rb->came_from == PCB_SOUTH ? ans.orig.Y1 : 0);
		ans.inflated.X2 = (e->rb->came_from == PCB_WEST ? ans.orig.X2 : PCB->MaxWidth);
		ans.inflated.Y2 = (e->rb->came_from == PCB_NORTH ? ans.orig.Y2 : PCB->MaxHeight);
		if (e->rb->came_from == PCB_NORTH)
			ans.done = noshrink = _SOUTH;
		else if (e->rb->came_from == PCB_EAST)
			ans.done = noshrink = _WEST;
		else if (e->rb->came_from == PCB_SOUTH)
			ans.done = noshrink = _NORTH;
		else if (e->rb->came_from == PCB_WEST)
			ans.done = noshrink = _EAST;
		else
			ans.done = noshrink = 0;
		break;
	case PCB_NORTH:
		ans.done = _SOUTH + _EAST + _WEST;
		noshrink = _SOUTH;
		ans.inflated.X1 = box->X1 - ans.bloat;
		ans.inflated.X2 = box->X2 + ans.bloat;
		ans.inflated.Y2 = box->Y2;
		ans.inflated.Y1 = 0;				/* far north */
		break;
	case PCB_NE:
		ans.done = _SOUTH + _WEST;
		noshrink = 0;
		ans.inflated.X1 = box->X1 - ans.bloat;
		ans.inflated.X2 = PCB->MaxWidth;
		ans.inflated.Y2 = box->Y2 + ans.bloat;
		ans.inflated.Y1 = 0;
		break;
	case PCB_EAST:
		ans.done = _NORTH + _SOUTH + _WEST;
		noshrink = _WEST;
		ans.inflated.Y1 = box->Y1 - ans.bloat;
		ans.inflated.Y2 = box->Y2 + ans.bloat;
		ans.inflated.X1 = box->X1;
		ans.inflated.X2 = PCB->MaxWidth;
		break;
	case PCB_SE:
		ans.done = _NORTH + _WEST;
		noshrink = 0;
		ans.inflated.X1 = box->X1 - ans.bloat;
		ans.inflated.X2 = PCB->MaxWidth;
		ans.inflated.Y2 = PCB->MaxHeight;
		ans.inflated.Y1 = box->Y1 - ans.bloat;
		break;
	case PCB_SOUTH:
		ans.done = _NORTH + _EAST + _WEST;
		noshrink = _NORTH;
		ans.inflated.X1 = box->X1 - ans.bloat;
		ans.inflated.X2 = box->X2 + ans.bloat;
		ans.inflated.Y1 = box->Y1;
		ans.inflated.Y2 = PCB->MaxHeight;
		break;
	case PCB_SW:
		ans.done = _NORTH + _EAST;
		noshrink = 0;
		ans.inflated.X1 = 0;
		ans.inflated.X2 = box->X2 + ans.bloat;
		ans.inflated.Y2 = PCB->MaxHeight;
		ans.inflated.Y1 = box->Y1 - ans.bloat;
		break;
	case PCB_WEST:
		ans.done = _NORTH + _SOUTH + _EAST;
		noshrink = _EAST;
		ans.inflated.Y1 = box->Y1 - ans.bloat;
		ans.inflated.Y2 = box->Y2 + ans.bloat;
		ans.inflated.X1 = 0;
		ans.inflated.X2 = box->X2;
		break;
	case PCB_NW:
		ans.done = _SOUTH + _EAST;
		noshrink = 0;
		ans.inflated.X1 = 0;
		ans.inflated.X2 = box->X2 + ans.bloat;
		ans.inflated.Y2 = box->Y2 + ans.bloat;
		ans.inflated.Y1 = 0;
		break;
	default:
		noshrink = ans.done = 0;
		assert(0);
	}
	ans.keep = e->rb->style->Clearance;
	ans.parent = nonhomeless_parent(e->rb);
	pcb_r_search(rtree, &ans.inflated, NULL, __Expand_this_rect, &ans, NULL);
/* because the overlaping boxes are found in random order, some blockers
 * may have limited edges prematurely, so we check if the blockers realy
 * are blocking, and make another try if not
 */
	if (ans.n && !boink_box(ans.n, &ans, PCB_NORTH))
		ans.inflated.Y1 = 0;
	else
		ans.done |= _NORTH;
	if (ans.e && !boink_box(ans.e, &ans, PCB_EAST))
		ans.inflated.X2 = PCB->MaxWidth;
	else
		ans.done |= _EAST;
	if (ans.s && !boink_box(ans.s, &ans, PCB_SOUTH))
		ans.inflated.Y2 = PCB->MaxHeight;
	else
		ans.done |= _SOUTH;
	if (ans.w && !boink_box(ans.w, &ans, PCB_WEST))
		ans.inflated.X1 = 0;
	else
		ans.done |= _WEST;
	if (ans.done != _NORTH + _EAST + _SOUTH + _WEST) {
		pcb_r_search(rtree, &ans.inflated, NULL, __Expand_this_rect, &ans, NULL);
	}
	if ((noshrink & _NORTH) == 0)
		ans.inflated.Y1 += ans.bloat;
	if ((noshrink & _EAST) == 0)
		ans.inflated.X2 -= ans.bloat;
	if ((noshrink & _SOUTH) == 0)
		ans.inflated.Y2 -= ans.bloat;
	if ((noshrink & _WEST) == 0)
		ans.inflated.X1 += ans.bloat;
	return &ans;
}

/* blocker_to_heap puts the blockers into a heap so they
 * can be retrieved in clockwise order. If a blocker
 * is also a target, it gets put into the vector too.
 * It returns 1 for any fixed blocker that is not part
 * of this net and zero otherwise.
 */
static int blocker_to_heap(pcb_heap_t * heap, routebox_t * rb, pcb_box_t * box, pcb_direction_t dir)
{
	pcb_box_t b = rb->sbox;
	if (rb->style->Clearance > AutoRouteParameters.style->Clearance)
		b = pcb_bloat_box(&b, rb->style->Clearance - AutoRouteParameters.style->Clearance);
	b = pcb_clip_box(&b, box);
	assert(pcb_box_is_good(&b));
	/* we want to look at the blockers clockwise around the box */
	switch (dir) {
		/* we need to use the other coordinate fraction to resolve
		 * ties since we want the shorter of the furthest
		 * first.
		 */
	case PCB_NORTH:
		pcb_heap_insert(heap, b.X1 - b.X1 / (b.X2 + 1.0), rb);
		break;
	case PCB_EAST:
		pcb_heap_insert(heap, b.Y1 - b.Y1 / (b.Y2 + 1.0), rb);
		break;
	case PCB_SOUTH:
		pcb_heap_insert(heap, -(b.X2 + b.X1 / (b.X2 + 1.0)), rb);
		break;
	case PCB_WEST:
		pcb_heap_insert(heap, -(b.Y2 + b.Y1 / (b.Y2 + 1.0)), rb);
		break;
	default:
		assert(0);
	}
	if (rb->flags.fixed && !rb->flags.target && !rb->flags.source)
		return 1;
	return 0;
}

/* this creates an EXPANSION_AREA to bridge small gaps or,
 * (more commonly) create a supper-thin box to provide a
 * home for an expansion edge.
 */
static routebox_t *CreateBridge(const pcb_box_t * area, routebox_t * parent, pcb_direction_t dir)
{
	routebox_t *rb = (routebox_t *) malloc(sizeof(*rb));
	memset((void *) rb, 0, sizeof(*rb));
	assert(area && parent);
	init_const_box(rb, area->X1, area->Y1, area->X2, area->Y2, 0);
	rb->group = parent->group;
	rb->type = EXPANSION_AREA;
	rb->came_from = dir;
	rb->cost_point = pcb_closest_pcb_point_in_box(&parent->cost_point, area);
	rb->cost = parent->cost + pcb_cost_to_point_on_layer(&parent->cost_point, &rb->cost_point, rb->group);
	rb->parent.expansion_area = route_parent(parent);
	if (rb->parent.expansion_area->flags.homeless)
		RB_up_count(rb->parent.expansion_area);
	rb->flags.homeless = 1;
	rb->flags.nobloat = 1;
	rb->style = parent->style;
	rb->conflicts_with = parent->conflicts_with;
#if defined(ROUTE_DEBUG) && defined(DEBUG_SHOW_EDGES)
	showroutebox(rb);
#endif
	return rb;
}

/* moveable_edge prepares the new search edges based on the
 * starting box, direction and blocker if any.
 */
void
moveable_edge(vector_t * result, const pcb_box_t * box, pcb_direction_t dir,
							routebox_t * rb,
							routebox_t * blocker, edge_t * e, pcb_rtree_t * targets,
							routeone_state_t *s, pcb_rtree_t * tree, vector_t * area_vec)
{
	pcb_box_t b;
	assert(pcb_box_is_good(box));
	b = *box;
	/* for the cardinal directions, move the box to overlap the
	 * the parent by 1 unit. Corner expansions overlap more
	 * and their starting boxes are pre-prepared.
	 * Check if anything is headed off the board edges
	 */
	switch (dir) {
	default:
		break;
	case PCB_NORTH:
		b.Y2 = b.Y1;
		b.Y1--;
		if (b.Y1 <= AutoRouteParameters.bloat)
			return;										/* off board edge */
		break;
	case PCB_EAST:
		b.X1 = b.X2;
		b.X2++;
		if (b.X2 >= PCB->MaxWidth - AutoRouteParameters.bloat)
			return;										/* off board edge */
		break;
	case PCB_SOUTH:
		b.Y1 = b.Y2;
		b.Y2++;
		if (b.Y2 >= PCB->MaxHeight - AutoRouteParameters.bloat)
			return;										/* off board edge */
		break;
	case PCB_WEST:
		b.X2 = b.X1;
		b.X1--;
		if (b.X1 <= AutoRouteParameters.bloat)
			return;										/* off board edge */
		break;
	case PCB_NE:
		if (b.Y1 <= AutoRouteParameters.bloat + 1 && b.X2 >= PCB->MaxWidth - AutoRouteParameters.bloat - 1)
			return;										/* off board edge */
		if (b.Y1 <= AutoRouteParameters.bloat + 1)
			dir = PCB_EAST;								/* north off board edge */
		if (b.X2 >= PCB->MaxWidth - AutoRouteParameters.bloat - 1)
			dir = PCB_NORTH;							/* east off board edge */
		break;
	case PCB_SE:
		if (b.Y2 >= PCB->MaxHeight - AutoRouteParameters.bloat - 1 && b.X2 >= PCB->MaxWidth - AutoRouteParameters.bloat - 1)
			return;										/* off board edge */
		if (b.Y2 >= PCB->MaxHeight - AutoRouteParameters.bloat - 1)
			dir = PCB_EAST;								/* south off board edge */
		if (b.X2 >= PCB->MaxWidth - AutoRouteParameters.bloat - 1)
			dir = PCB_SOUTH;							/* east off board edge */
		break;
	case PCB_SW:
		if (b.Y2 >= PCB->MaxHeight - AutoRouteParameters.bloat - 1 && b.X1 <= AutoRouteParameters.bloat + 1)
			return;										/* off board edge */
		if (b.Y2 >= PCB->MaxHeight - AutoRouteParameters.bloat - 1)
			dir = PCB_WEST;								/* south off board edge */
		if (b.X1 <= AutoRouteParameters.bloat + 1)
			dir = PCB_SOUTH;							/* west off board edge */
		break;
	case PCB_NW:
		if (b.Y1 <= AutoRouteParameters.bloat + 1 && b.X1 <= AutoRouteParameters.bloat + 1)
			return;										/* off board edge */
		if (b.Y1 <= AutoRouteParameters.bloat + 1)
			dir = PCB_WEST;								/* north off board edge */
		if (b.X1 <= AutoRouteParameters.bloat + 1)
			dir = PCB_NORTH;							/* west off board edge */
		break;
	}

	if (!blocker) {
		edge_t *ne;
		routebox_t *nrb = CreateBridge(&b, rb, dir);
		/* move the cost point in corner expansions
		 * these boxes are bigger, so move close to the target
		 */
		if (dir == PCB_NE || dir == PCB_SE || dir == PCB_SW || dir == PCB_NW) {
			pcb_cheap_point_t p;
			p = pcb_closest_pcb_point_in_box(&nrb->cost_point, &e->minpcb_cost_target->sbox);
			p = pcb_closest_pcb_point_in_box(&p, &b);
			nrb->cost += pcb_cost_to_point_on_layer(&p, &nrb->cost_point, nrb->group);
			nrb->cost_point = p;
		}
		ne = CreateEdge(nrb, nrb->cost_point.X, nrb->cost_point.Y, nrb->cost, NULL, dir, targets);
		vector_append(result, ne);
	}
	else if (AutoRouteParameters.with_conflicts && !blocker->flags.target
					 && !blocker->flags.fixed && !blocker->flags.touched && !blocker->flags.source && blocker->type != EXPANSION_AREA) {
		edge_t *ne;
		routebox_t *nrb;
		/* make a bridge to the edge of the blocker
		 * in all directions from there
		 */
		switch (dir) {
		case PCB_NORTH:
			b.Y1 = blocker->sbox.Y2 - 1;
			break;
		case PCB_EAST:
			b.X2 = blocker->sbox.X1 + 1;
			break;
		case PCB_SOUTH:
			b.Y2 = blocker->sbox.Y1 + 1;
			break;
		case PCB_WEST:
			b.X1 = blocker->sbox.X2 - 1;
			break;
		default:
			assert(0);
		}
		if (!pcb_box_is_good(&b))
			return;										/* how did this happen ? */
		nrb = CreateBridge(&b, rb, dir);
		pcb_r_insert_entry(tree, &nrb->box);
		vector_append(area_vec, nrb);
		nrb->flags.homeless = 0;		/* not homeless any more */
		/* mark this one as conflicted */
		path_conflicts(nrb, blocker, pcb_true);
		/* and make an expansion edge */
		nrb->cost_point = pcb_closest_pcb_point_in_box(&nrb->cost_point, &blocker->sbox);
		nrb->cost +=
			pcb_cost_to_point_on_layer(&nrb->parent.expansion_area->cost_point, &nrb->cost_point, nrb->group) * CONFLICT_PENALTY(blocker);

		ne = CreateEdge(nrb, nrb->cost_point.X, nrb->cost_point.Y, nrb->cost, NULL, PCB_ANY_DIR, targets);
		ne->flags.is_interior = 1;
		vector_append(result, ne);
	}
#if 1
	else if (blocker->type == EXPANSION_AREA) {
		if (blocker->cost < rb->cost || blocker->cost <= rb->cost +
				pcb_cost_to_point_on_layer(&blocker->cost_point, &rb->cost_point, rb->group))
			return;
		if (blocker->conflicts_with || rb->conflicts_with)
			return;
		/* does the blocker overlap this routebox ?? */
		/* does this re-parenting operation leave a memory leak? */
		if (blocker->parent.expansion_area->flags.homeless)
			RB_down_count(blocker->parent.expansion_area);
		blocker->parent.expansion_area = rb;
		return;
	}
#endif
	else if (blocker->flags.target) {
		routebox_t *nrb;
		edge_t *ne;
		b = pcb_bloat_box(&b, 1);
		if (!pcb_box_intersect(&b, &blocker->sbox)) {
			/* if the expansion edge stopped before touching, expand the bridge */
			switch (dir) {
			case PCB_NORTH:
				b.Y1 -= AutoRouteParameters.bloat + 1;
				break;
			case PCB_EAST:
				b.X2 += AutoRouteParameters.bloat + 1;
				break;
			case PCB_SOUTH:
				b.Y2 += AutoRouteParameters.bloat + 1;
				break;
			case PCB_WEST:
				b.X1 -= AutoRouteParameters.bloat + 1;
				break;
			default:
				assert(0);
			}
		}
		assert(pcb_box_intersect(&b, &blocker->sbox));
		b = pcb_shrink_box(&b, 1);
		nrb = CreateBridge(&b, rb, dir);
		pcb_r_insert_entry(tree, &nrb->box);
		vector_append(area_vec, nrb);
		nrb->flags.homeless = 0;		/* not homeless any more */
		ne = CreateEdge(nrb, nrb->cost_point.X, nrb->cost_point.Y, nrb->cost, blocker, dir, NULL);
		best_path_candidate(s, ne, blocker);
		DestroyEdge(&ne);
	}
}

struct break_info {
	pcb_heap_t *heap;
	routebox_t *parent;
	pcb_box_t box;
	pcb_direction_t dir;
	pcb_bool ignore_source;
};

static pcb_r_dir_t __GatherBlockers(const pcb_box_t * box, void *cl)
{
	routebox_t *rb = (routebox_t *) box;
	struct break_info *bi = (struct break_info *) cl;
	pcb_box_t b;

	if (bi->parent == rb || rb->flags.touched || bi->parent->parent.expansion_area == rb)
		return PCB_R_DIR_NOT_FOUND;
	if (rb->flags.source && bi->ignore_source)
		return PCB_R_DIR_NOT_FOUND;
	b = rb->sbox;
	if (rb->style->Clearance > AutoRouteParameters.style->Clearance)
		b = pcb_bloat_box(&b, rb->style->Clearance - AutoRouteParameters.style->Clearance);
	if (b.X2 <= bi->box.X1 || b.X1 >= bi->box.X2 || b.Y1 >= bi->box.Y2 || b.Y2 <= bi->box.Y1)
		return PCB_R_DIR_NOT_FOUND;
	if (blocker_to_heap(bi->heap, rb, &bi->box, bi->dir))
		return PCB_R_DIR_FOUND_CONTINUE;
	return PCB_R_DIR_NOT_FOUND;
}

/* shrink the box to the last limit for the previous direction,
 * i.e. if dir is SOUTH, then this means fixing up an EAST leftover
 * edge, which would be the southern most edge for that example.
 */
static inline pcb_box_t previous_edge(pcb_coord_t last, pcb_direction_t i, const pcb_box_t * b)
{
	pcb_box_t db = *b;
	switch (i) {
	case PCB_EAST:
		db.X1 = last;
		break;
	case PCB_SOUTH:
		db.Y1 = last;
		break;
	case PCB_WEST:
		db.X2 = last;
		break;
	default:
		pcb_message(PCB_MSG_ERROR, "previous edge bogus direction!");
		assert(0);
	}
	return db;
}

/* Break all the edges of the box that need breaking, handling
 * targets as they are found, and putting any moveable edges
 * in the return vector.
 */
vector_t *BreakManyEdges(routeone_state_t * s, pcb_rtree_t * targets, pcb_rtree_t * tree,
												 vector_t * area_vec, struct E_result * ans, routebox_t * rb, edge_t * e)
{
	struct break_info bi;
	vector_t *edges;
	pcb_heap_t *heap[4];
	pcb_coord_t first, last;
	pcb_coord_t bloat;
	pcb_direction_t dir;
	routebox_t fake;

	edges = vector_create();
	bi.ignore_source = rb->parent.expansion_area->flags.source;
	bi.parent = rb;
	/* we add 2 to the bloat.
	 * 1 will get us to the actual blocker that Expand() hit
	 * but 1 more is needed because the new expansion edges
	 * move out by 1 so they don't overlap their parents
	 * this extra expansion could "trap" the edge if
	 * there is a blocker 2 units from the original rb,
	 * it is 1 unit from the new expansion edge which
	 * would prevent expansion. So we want to break the
	 * edge on it now to avoid the trap.
	 */

	bloat = AutoRouteParameters.bloat + 2;
	/* for corner expansion, we need to have a fake blocker
	 * to prevent expansion back where we came from since
	 * we still need to break portions of all 4 edges
	 */
	if (e->expand_dir == PCB_NE || e->expand_dir == PCB_SE || e->expand_dir == PCB_SW || e->expand_dir == PCB_NW) {
		pcb_box_t *fb = (pcb_box_t *) & fake.sbox;
		memset(&fake, 0, sizeof(fake));
		*fb = e->rb->sbox;
		fake.flags.fixed = 1;				/* this stops expansion there */
		fake.type = LINE;
		fake.style = AutoRouteParameters.style;
#ifndef NDEBUG
		/* the routbox_is_good checker wants a lot more! */
		fake.flags.inited = 1;
		fb = (pcb_box_t *) & fake.box;
		*fb = e->rb->sbox;
		fake.same_net.next = fake.same_net.prev = &fake;
		fake.same_subnet.next = fake.same_subnet.prev = &fake;
		fake.original_subnet.next = fake.original_subnet.prev = &fake;
		fake.different_net.next = fake.different_net.prev = &fake;
#endif
	}
	/* gather all of the blockers in heaps so they can be accessed
	 * in clockwise order, which allows finding corners that can
	 * be expanded.
	 */
	for (dir = PCB_NORTH; dir <= PCB_WEST; dir = directionIncrement(dir)) {
		int tmp;
		/* don't break the edge we came from */
		if (e->expand_dir != ((dir + 2) % 4)) {
			heap[dir] = pcb_heap_create();
			bi.box = pcb_bloat_box(&rb->sbox, bloat);
			bi.heap = heap[dir];
			bi.dir = dir;
			/* convert to edge */
			switch (dir) {
			case PCB_NORTH:
				bi.box.Y2 = bi.box.Y1 + bloat + 1;
				/* for corner expansion, block the start edges and
				 * limit the blocker search to only the new edge segment
				 */
				if (e->expand_dir == PCB_SE || e->expand_dir == PCB_SW)
					blocker_to_heap(heap[dir], &fake, &bi.box, dir);
				if (e->expand_dir == PCB_SE)
					bi.box.X1 = e->rb->sbox.X2;
				if (e->expand_dir == PCB_SW)
					bi.box.X2 = e->rb->sbox.X1;
				pcb_r_search(tree, &bi.box, NULL, __GatherBlockers, &bi, &tmp);
				rb->n = tmp;
				break;
			case PCB_EAST:
				bi.box.X1 = bi.box.X2 - bloat - 1;
				/* corner, same as above */
				if (e->expand_dir == PCB_SW || e->expand_dir == PCB_NW)
					blocker_to_heap(heap[dir], &fake, &bi.box, dir);
				if (e->expand_dir == PCB_SW)
					bi.box.Y1 = e->rb->sbox.Y2;
				if (e->expand_dir == PCB_NW)
					bi.box.Y2 = e->rb->sbox.Y1;
				pcb_r_search(tree, &bi.box, NULL, __GatherBlockers, &bi, &tmp);
				rb->e = tmp;
				break;
			case PCB_SOUTH:
				bi.box.Y1 = bi.box.Y2 - bloat - 1;
				/* corner, same as above */
				if (e->expand_dir == PCB_NE || e->expand_dir == PCB_NW)
					blocker_to_heap(heap[dir], &fake, &bi.box, dir);
				if (e->expand_dir == PCB_NE)
					bi.box.X1 = e->rb->sbox.X2;
				if (e->expand_dir == PCB_NW)
					bi.box.X2 = e->rb->sbox.X1;
				pcb_r_search(tree, &bi.box, NULL, __GatherBlockers, &bi, &tmp);
				rb->s = tmp;
				break;
			case PCB_WEST:
				bi.box.X2 = bi.box.X1 + bloat + 1;
				/* corner, same as above */
				if (e->expand_dir == PCB_NE || e->expand_dir == PCB_SE)
					blocker_to_heap(heap[dir], &fake, &bi.box, dir);
				if (e->expand_dir == PCB_SE)
					bi.box.Y1 = e->rb->sbox.Y2;
				if (e->expand_dir == PCB_NE)
					bi.box.Y2 = e->rb->sbox.Y1;
				pcb_r_search(tree, &bi.box, NULL, __GatherBlockers, &bi, &tmp);
				rb->w = tmp;
				break;
			default:
				assert(0);
			}
		}
		else
			heap[dir] = NULL;
	}
#if 1
	rb->cost += (rb->n + rb->e + rb->s + rb->w) * AutoRouteParameters.CongestionPenalty / box_area(rb->sbox);
#endif
/* now handle the blockers:
 * Go around the expansion area clockwise (North->East->South->West)
 * pulling blockers from the heap (which makes them come out in the right
 * order). Break the edges on the blocker and make the segments and corners
 * moveable as possible.
 */
	first = last = -1;
	for (dir = PCB_NORTH; dir <= PCB_WEST; dir = directionIncrement(dir)) {
		if (heap[dir] && !pcb_heap_is_empty(heap[dir])) {
			/* pull the very first one out of the heap outside of the
			 * heap loop because it is special; it can be part of a corner
			 */
			routebox_t *blk = (routebox_t *) pcb_heap_remove_smallest(heap[dir]);
			pcb_box_t b = rb->sbox;
			struct broken_boxes broke = break_box_edge(&b, dir, blk);
			if (broke.is_valid_left) {
				/* if last > 0, then the previous edge had a segment
				 * joining this one, so it forms a valid corner expansion
				 */
				if (last > 0) {
					/* make a corner expansion */
					pcb_box_t db = b;
					switch (dir) {
					case PCB_EAST:
						/* possible NE expansion */
						db.X1 = last;
						db.Y2 = MIN(db.Y2, broke.left.Y2);
						break;
					case PCB_SOUTH:
						/* possible SE expansion */
						db.Y1 = last;
						db.X1 = MAX(db.X1, broke.left.X1);
						break;
					case PCB_WEST:
						/* possible SW expansion */
						db.X2 = last;
						db.Y1 = MAX(db.Y1, broke.left.Y1);
						break;
					default:
						assert(0);
						break;
					}
					moveable_edge(edges, &db, (pcb_direction_t) (dir + 3), rb, NULL, e, targets, s, NULL, NULL);
				}
				else if (dir == PCB_NORTH) {	/* north is start, so nothing "before" it */
					/* save for a possible corner once we've
					 * finished circling the box
					 */
					first = MAX(b.X1, broke.left.X2);
				}
				else {
					/* this is just a boring straight expansion
					 * since the orthogonal segment was blocked
					 */
					moveable_edge(edges, &broke.left, dir, rb, NULL, e, targets, s, NULL, NULL);
				}
			}													/* broke.is_valid_left */
			else if (last > 0) {
				/* if the last one didn't become a corner,
				 * we still want to expand it straight out
				 * in the direction of the previous edge,
				 * which it belongs to.
				 */
				pcb_box_t db = previous_edge(last, dir, &rb->sbox);
				moveable_edge(edges, &db, (pcb_direction_t) (dir - 1), rb, NULL, e, targets, s, NULL, NULL);
			}
			if (broke.is_valid_center && !blk->flags.source)
				moveable_edge(edges, &broke.center, dir, rb, blk, e, targets, s, tree, area_vec);
			/* this is the heap extraction loop. We break out
			 * if there's nothing left in the heap, but if we * are blocked all the way to the far edge, we can
			 * just leave stuff in the heap when it is destroyed
			 */
			while (broke.is_valid_right) {
				/* move the box edge to the next potential free point */
				switch (dir) {
				case PCB_NORTH:
					last = b.X1 = MAX(broke.right.X1, b.X1);
					break;
				case PCB_EAST:
					last = b.Y1 = MAX(broke.right.Y1, b.Y1);
					break;
				case PCB_SOUTH:
					last = b.X2 = MIN(broke.right.X2, b.X2);
					break;
				case PCB_WEST:
					last = b.Y2 = MIN(broke.right.Y2, b.Y2);
					break;
				default:
					assert(0);
				}
				if (pcb_heap_is_empty(heap[dir]))
					break;
				blk = (routebox_t *) pcb_heap_remove_smallest(heap[dir]);
				broke = break_box_edge(&b, dir, blk);
				if (broke.is_valid_left)
					moveable_edge(edges, &broke.left, dir, rb, NULL, e, targets, s, NULL, NULL);
				if (broke.is_valid_center && !blk->flags.source)
					moveable_edge(edges, &broke.center, dir, rb, blk, e, targets, s, tree, area_vec);
			}
			if (!broke.is_valid_right)
				last = -1;
		}
		else {											/* if (heap[dir]) */

			/* nothing touched this edge! Expand the whole edge unless
			 * (1) it hit the board edge or (2) was the source of our expansion
			 *
			 * for this case (of hitting nothing) we give up trying for corner
			 * expansions because it is likely that they're not possible anyway
			 */
			if ((e->expand_dir == PCB_ANY_DIR ? e->rb->came_from : e->expand_dir) != ((dir + 2) % 4)) {
				/* ok, we are not going back on ourselves, and the whole edge seems free */
				moveable_edge(edges, &rb->sbox, dir, rb, NULL, e, targets, s, NULL, NULL);
			}

			if (last > 0) {
				/* expand the leftover from the prior direction */
				pcb_box_t db = previous_edge(last, dir, &rb->sbox);
				moveable_edge(edges, &db, (pcb_direction_t) (dir - 1), rb, NULL, e, targets, s, NULL, NULL);
			}
			last = -1;
		}
	}															/* for loop */
	/* finally, check for the NW corner now that we've come full circle */
	if (first > 0 && last > 0) {
		pcb_box_t db = rb->sbox;
		db.X2 = first;
		db.Y2 = last;
		moveable_edge(edges, &db, PCB_NW, rb, NULL, e, targets, s, NULL, NULL);
	}
	else {
		if (first > 0) {
			pcb_box_t db = rb->sbox;
			db.X2 = first;
			moveable_edge(edges, &db, PCB_NORTH, rb, NULL, e, targets, s, NULL, NULL);
		}
		else if (last > 0) {
			pcb_box_t db = rb->sbox;
			db.Y2 = last;
			moveable_edge(edges, &db, PCB_WEST, rb, NULL, e, targets, s, NULL, NULL);
		}
	}
	/* done with all expansion edges of this box */
	for (dir = PCB_NORTH; dir <= PCB_WEST; dir = directionIncrement(dir)) {
		if (heap[dir])
			pcb_heap_destroy(&heap[dir]);
	}
	return edges;
}

static routebox_t *rb_source(routebox_t * rb)
{
	while (rb && !rb->flags.source) {
		assert(rb->type == EXPANSION_AREA);
		rb = rb->parent.expansion_area;
	}
	assert(rb);
	return rb;
}

/* ------------ */

struct foib_info {
	const pcb_box_t *box;
	routebox_t *intersect;
	jmp_buf env;
};

static pcb_r_dir_t foib_rect_in_reg(const pcb_box_t * box, void *cl)
{
	struct foib_info *foib = (struct foib_info *) cl;
	pcb_box_t rbox;
	routebox_t *rb = (routebox_t *) box;
	if (rb->flags.touched)
		return PCB_R_DIR_NOT_FOUND;
/*  if (rb->type == EXPANSION_AREA && !rb->flags.is_via)*/
	/*   return PCB_R_DIR_NOT_FOUND; */
	rbox = bloat_routebox(rb);
	if (!pcb_box_intersect(&rbox, foib->box))
		return PCB_R_DIR_NOT_FOUND;
	/* this is an intersector! */
	foib->intersect = (routebox_t *) box;
	longjmp(foib->env, 1);				/* skip to the end! */
	return PCB_R_DIR_FOUND_CONTINUE;
}

static routebox_t *FindOneInBox(pcb_rtree_t * rtree, routebox_t * rb)
{
	struct foib_info foib;
	pcb_box_t r;

	r = rb->sbox;
	foib.box = &r;
	foib.intersect = NULL;

	if (setjmp(foib.env) == 0)
		pcb_r_search(rtree, &r, NULL, foib_rect_in_reg, &foib, NULL);
	return foib.intersect;
}

struct therm_info {
	routebox_t *plane;
	pcb_box_t query;
	jmp_buf env;
};
static pcb_r_dir_t ftherm_rect_in_reg(const pcb_box_t * box, void *cl)
{
	routebox_t *rbox = (routebox_t *) box;
	struct therm_info *ti = (struct therm_info *) cl;
	pcb_box_t sq, sb;

	if (rbox->type != TERM)
		return PCB_R_DIR_NOT_FOUND;
	if (rbox->group != ti->plane->group)
		return PCB_R_DIR_NOT_FOUND;

	sb = shrink_routebox(rbox);
	switch (rbox->type) {
	case TERM:
	case VIA_SHADOW:
		sq = pcb_shrink_box(&ti->query, rbox->style->Diameter);
		if (!pcb_box_intersect(&sb, &sq))
			return PCB_R_DIR_NOT_FOUND;
		sb.X1 = PCB_BOX_CENTER_X(sb);
		sb.Y1 = PCB_BOX_CENTER_Y(sb);
		break;
	default:
		assert(0);
	}
	ti->plane = rbox;
	longjmp(ti->env, 1);
	return PCB_R_DIR_FOUND_CONTINUE;
}

/* check for a pin or via target that a polygon can just use a thermal to connect to */
routebox_t *FindThermable(pcb_rtree_t * rtree, routebox_t * rb)
{
	struct therm_info info;

	info.plane = rb;
	info.query = shrink_routebox(rb);

	if (setjmp(info.env) == 0) {
		pcb_r_search(rtree, &info.query, NULL, ftherm_rect_in_reg, &info, NULL);
		return NULL;
	}
	return info.plane;
}

/*--------------------------------------------------------------------
 * Route-tracing code: once we've got a path of expansion boxes, trace
 * a line through them to actually create the connection.
 */
static void RD_DrawThermal(routedata_t * rd, pcb_coord_t X, pcb_coord_t Y, pcb_cardinal_t group, pcb_cardinal_t layer, routebox_t * subnet, pcb_bool is_bad)
{
	routebox_t *rb;
	rb = (routebox_t *) malloc(sizeof(*rb));
	memset((void *) rb, 0, sizeof(*rb));
	init_const_box(rb, X, Y, X + 1, Y + 1, 0);
	rb->group = group;
	rb->layer = layer;
	rb->flags.fixed = 0;
	rb->flags.is_bad = is_bad;
	rb->flags.is_odd = AutoRouteParameters.is_odd;
	rb->flags.circular = 0;
	rb->style = AutoRouteParameters.style;
	rb->type = THERMAL;
	InitLists(rb);
	MergeNets(rb, subnet, NET);
	MergeNets(rb, subnet, SUBNET);
	/* add it to the r-tree, this may be the whole route! */
	pcb_r_insert_entry(rd->layergrouptree[rb->group], &rb->box);
	rb->flags.homeless = 0;
}

static void RD_DrawVia(routedata_t * rd, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t radius, routebox_t * subnet, pcb_bool is_bad)
{
	routebox_t *rb, *first_via = NULL;
	int i;
	int ka = AutoRouteParameters.style->Clearance;
	pcb_pstk_t *live_via = NULL;

	if (conf_core.editor.live_routing) {
TODO("padstack: when style contains proto, remove this")
		live_via = pcb_pstk_new_compat_via(PCB->Data, -1, X, Y, AutoRouteParameters.style->Hole, radius * 2, 2 * AutoRouteParameters.style->Clearance, 0, PCB_PSTK_COMPAT_ROUND, 1);
		if (live_via != NULL)
			pcb_pstk_invalidate_draw(live_via);
	}

	/* a via cuts through every layer group */
	for (i = 0; i < pcb_max_group(PCB); i++) {
		if (!is_layer_group_active[i])
			continue;
		rb = (routebox_t *) malloc(sizeof(*rb));
		memset((void *) rb, 0, sizeof(*rb));
		init_const_box(rb,
									 /*X1 */ X - radius, /*Y1 */ Y - radius,
									 /*X2 */ X + radius + 1, /*Y2 */ Y + radius + 1, ka);
		rb->group = i;
		rb->flags.fixed = 0;				/* indicates that not on PCB yet */
		rb->flags.is_odd = AutoRouteParameters.is_odd;
		rb->flags.is_bad = is_bad;
		rb->came_from = PCB_ANY_DIR;
		rb->flags.circular = pcb_true;
		rb->style = AutoRouteParameters.style;
		rb->pass = AutoRouteParameters.pass;
		if (first_via == NULL) {
			rb->type = VIA;
			rb->parent.via = NULL;		/* indicates that not on PCB yet */
			first_via = rb;
			/* only add the first via to mtspace, not the shadows too */
			mtspace_add(rd->mtspace, &rb->box, rb->flags.is_odd ? ODD : EVEN, rb->style->Clearance);
		}
		else {
			rb->type = VIA_SHADOW;
			rb->parent.via_shadow = first_via;
		}
		InitLists(rb);
		/* add these to proper subnet. */
		MergeNets(rb, subnet, NET);
		MergeNets(rb, subnet, SUBNET);
		assert(__routepcb_box_is_good(rb));
		/* and add it to the r-tree! */
		pcb_r_insert_entry(rd->layergrouptree[rb->group], &rb->box);
		rb->flags.homeless = 0;			/* not homeless anymore */
		rb->livedraw_obj.via = live_via;
	}
}

static void
RD_DrawLine(routedata_t * rd,
						pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2,
						pcb_coord_t Y2, pcb_coord_t halfthick, pcb_cardinal_t group, routebox_t * subnet, pcb_bool is_bad, pcb_bool is_45)
{
	/* we hold the line in a queue to concatenate segments that
	 * ajoin one another. That reduces the number of things in
	 * the trees and allows conflict boxes to be larger, both of
	 * which are really useful.
	 */
	static pcb_coord_t qX1 = -1, qY1, qX2, qY2;
	static pcb_coord_t qhthick;
	static pcb_cardinal_t qgroup;
	static pcb_bool qis_45, qis_bad;
	static routebox_t *qsn;

	routebox_t *rb;
	pcb_coord_t ka = AutoRouteParameters.style->Clearance;

	/* don't draw zero-length segments. */
	if (X1 == X2 && Y1 == Y2)
		return;
	if (qX1 == -1) {							/* first ever */
		qX1 = X1;
		qY1 = Y1;
		qX2 = X2;
		qY2 = Y2;
		qhthick = halfthick;
		qgroup = group;
		qis_45 = is_45;
		qis_bad = is_bad;
		qsn = subnet;
		return;
	}
	/* Check if the lines concatenat. We only check the
	 * normal expected nextpoint=lastpoint condition
	 */
	if (X1 == qX2 && Y1 == qY2 && qhthick == halfthick && qgroup == group) {
		if (qX1 == qX2 && X1 == X2) {	/* everybody on the same X here */
			qY2 = Y2;
			return;
		}
		if (qY1 == qY2 && Y1 == Y2) {	/* same Y all around */
			qX2 = X2;
			return;
		}
	}
	/* dump the queue, no match here */
	if (qX1 == -1)
		return;											/* but not this! */
	rb = (routebox_t *) malloc(sizeof(*rb));
	memset((void *) rb, 0, sizeof(*rb));
	assert(is_45 ? (PCB_ABS(qX2 - qX1) == PCB_ABS(qY2 - qY1))	/* line must be 45-degrees */
				 : (qX1 == qX2 || qY1 == qY2) /* line must be ortho */ );
	init_const_box(rb,
								 /*X1 */ MIN(qX1, qX2) - qhthick,
								 /*Y1 */ MIN(qY1, qY2) - qhthick,
								 /*X2 */ MAX(qX1, qX2) + qhthick + 1,
								 /*Y2 */ MAX(qY1, qY2) + qhthick + 1, ka);
	rb->group = qgroup;
	rb->type = LINE;
	rb->line.x1 = qX1;
	rb->line.x2 = qX2;
	rb->line.y1 = qY1;
	rb->line.y2 = qY2;
	rb->parent.line = NULL;				/* indicates that not on PCB yet */
	rb->flags.fixed = 0;					/* indicates that not on PCB yet */
	rb->flags.is_odd = AutoRouteParameters.is_odd;
	rb->flags.is_bad = qis_bad;
	rb->came_from = PCB_ANY_DIR;
	rb->flags.homeless = 0;				/* we're putting this in the tree */
	rb->flags.nonstraight = qis_45;
	rb->flags.bl_to_ur = ((qX2 >= qX1 && qY2 <= qY1)
												|| (qX2 <= qX1 && qY2 >= qY1));
	rb->style = AutoRouteParameters.style;
	rb->pass = AutoRouteParameters.pass;
	InitLists(rb);
	/* add these to proper subnet. */
	MergeNets(rb, qsn, NET);
	MergeNets(rb, qsn, SUBNET);
	assert(__routepcb_box_is_good(rb));
	/* and add it to the r-tree! */
	pcb_r_insert_entry(rd->layergrouptree[rb->group], &rb->box);

	if (conf_core.editor.live_routing) {
		pcb_layer_t *layer = LAYER_PTR(PCB->LayerGroups.grp[rb->group].lid[0]);
		pcb_line_t *line = pcb_line_new(layer, qX1, qY1, qX2, qY2,
																					2 * qhthick, 0, pcb_flag_make(0));
		rb->livedraw_obj.line = line;
		if (line != NULL)
			pcb_line_invalidate_draw(layer, line);
	}

	/* and to the via space structures */
	if (AutoRouteParameters.use_vias)
		mtspace_add(rd->mtspace, &rb->box, rb->flags.is_odd ? ODD : EVEN, rb->style->Clearance);
	usedGroup[rb->group] = pcb_true;
	/* and queue this one */
	qX1 = X1;
	qY1 = Y1;
	qX2 = X2;
	qY2 = Y2;
	qhthick = halfthick;
	qgroup = group;
	qis_45 = is_45;
	qis_bad = is_bad;
	qsn = subnet;
}

static pcb_bool
RD_DrawManhattanLine(routedata_t * rd,
										 const pcb_box_t * box1, const pcb_box_t * box2,
										 pcb_cheap_point_t start, pcb_cheap_point_t end,
										 pcb_coord_t halfthick, pcb_cardinal_t group, routebox_t * subnet, pcb_bool is_bad, pcb_bool last_was_x)
{
	pcb_cheap_point_t knee = start;
	if (end.X == start.X) {
		RD_DrawLine(rd, start.X, start.Y, end.X, end.Y, halfthick, group, subnet, is_bad, pcb_false);
		return pcb_false;
	}
	else if (end.Y == start.Y) {
		RD_DrawLine(rd, start.X, start.Y, end.X, end.Y, halfthick, group, subnet, is_bad, pcb_false);
		return pcb_true;
	}
	/* find where knee belongs */
	if (pcb_point_in_box(box1, end.X, start.Y)
			|| pcb_point_in_box(box2, end.X, start.Y)) {
		knee.X = end.X;
		knee.Y = start.Y;
	}
	else {
		knee.X = start.X;
		knee.Y = end.Y;
	}
	if ((knee.X == end.X && !last_was_x) && (pcb_point_in_box(box1, start.X, end.Y)
																					 || pcb_point_in_box(box2, start.X, end.Y))) {
		knee.X = start.X;
		knee.Y = end.Y;
	}
	assert(AutoRouteParameters.is_smoothing || pcb_point_in_closed_box(box1, knee.X, knee.Y)
				 || pcb_point_in_closed_box(box2, knee.X, knee.Y));

	if (1 || !AutoRouteParameters.is_smoothing) {
		/* draw standard manhattan paths */
		RD_DrawLine(rd, start.X, start.Y, knee.X, knee.Y, halfthick, group, subnet, is_bad, pcb_false);
		RD_DrawLine(rd, knee.X, knee.Y, end.X, end.Y, halfthick, group, subnet, is_bad, pcb_false);
	}
	else {
		/* draw 45-degree path across knee */
		pcb_coord_t len45 = MIN(PCB_ABS(start.X - end.X), PCB_ABS(start.Y - end.Y));
		pcb_cheap_point_t kneestart = knee, kneeend = knee;
		if (kneestart.X == start.X)
			kneestart.Y += (kneestart.Y > start.Y) ? -len45 : len45;
		else
			kneestart.X += (kneestart.X > start.X) ? -len45 : len45;
		if (kneeend.X == end.X)
			kneeend.Y += (kneeend.Y > end.Y) ? -len45 : len45;
		else
			kneeend.X += (kneeend.X > end.X) ? -len45 : len45;
		RD_DrawLine(rd, start.X, start.Y, kneestart.X, kneestart.Y, halfthick, group, subnet, is_bad, pcb_false);
		RD_DrawLine(rd, kneestart.X, kneestart.Y, kneeend.X, kneeend.Y, halfthick, group, subnet, is_bad, pcb_true);
		RD_DrawLine(rd, kneeend.X, kneeend.Y, end.X, end.Y, halfthick, group, subnet, is_bad, pcb_false);
	}
	return (knee.X != end.X);
}

/* for smoothing, don't pack traces to min clearance gratuitously */
#if 0
static void add_clearance(pcb_cheap_point_t * nextpoint, const pcb_box_t * b)
{
	if (nextpoint->X == b->X1) {
		if (nextpoint->X + AutoRouteParameters.style->Clearance < (b->X1 + b->X2) / 2)
			nextpoint->X += AutoRouteParameters.style->Clearance;
		else
			nextpoint->X = (b->X1 + b->X2) / 2;
	}
	else if (nextpoint->X == b->X2) {
		if (nextpoint->X - AutoRouteParameters.style->Clearance > (b->X1 + b->X2) / 2)
			nextpoint->X -= AutoRouteParameters.style->Clearance;
		else
			nextpoint->X = (b->X1 + b->X2) / 2;
	}
	else if (nextpoint->Y == b->Y1) {
		if (nextpoint->Y + AutoRouteParameters.style->Clearance < (b->Y1 + b->Y2) / 2)
			nextpoint->Y += AutoRouteParameters.style->Clearance;
		else
			nextpoint->Y = (b->Y1 + b->Y2) / 2;
	}
	else if (nextpoint->Y == b->Y2) {
		if (nextpoint->Y - AutoRouteParameters.style->Clearance > (b->Y1 + b->Y2) / 2)
			nextpoint->Y -= AutoRouteParameters.style->Clearance;
		else
			nextpoint->Y = (b->Y1 + b->Y2) / 2;
	}
}
#endif

/* This back-traces the expansion boxes along the best path
 * it draws the lines that will make the actual path.
 * during refinement passes, it should try to maximize the area
 * for other tracks so routing completion is easier.
 *
 * during smoothing passes, it should try to make a better path,
 * possibly using diagonals, etc. The path boxes are larger on
 * average now so there is more possiblity to decide on a nice
 * path. Any combination of lines and arcs is possible, so long
 * as they don't poke more than half thick outside the path box.
 */

static void TracePath(routedata_t * rd, routebox_t * path, const routebox_t * target, routebox_t * subnet, pcb_bool is_bad)
{
	pcb_bool last_x = pcb_false;
	pcb_coord_t halfwidth = HALF_THICK(AutoRouteParameters.style->Thick);
	pcb_coord_t radius = HALF_THICK(AutoRouteParameters.style->Diameter);
	pcb_cheap_point_t lastpoint, nextpoint;
	routebox_t *lastpath;
	pcb_box_t b;

	assert(subnet->style == AutoRouteParameters.style);
	/*XXX: because we round up odd thicknesses, there's the possibility that
	 * a connecting line end-point might be 0.005 mil off the "real" edge.
	 * don't worry about this because line *thicknesses* are always >= 0.01 mil. */

	/* if we start with a thermal the target was a plane
	 * or the target was a pin and the source a plane
	 * in which case this thermal is the whole path
	 */
	if (path->flags.is_thermal) {
		/* the target was a plane, so we need to find a good spot for the via
		 * now. It's logical to place it close to the source box which
		 * is where we're utlimately headed on this path. However, it
		 * must reside in the plane as well as the via area too.
		 */
		nextpoint.X = PCB_BOX_CENTER_X(path->sbox);
		nextpoint.Y = PCB_BOX_CENTER_Y(path->sbox);
		if (path->parent.expansion_area->flags.is_via) {
			TargetPoint(&nextpoint, rb_source(path));
			/* nextpoint is the middle of the source terminal now */
			b = pcb_clip_box(&path->sbox, &path->parent.expansion_area->sbox);
			nextpoint = pcb_closest_pcb_point_in_box(&nextpoint, &b);
			/* now it's in the via and plane near the source */
		}
		else {											/* no via coming, target must have been a pin */

			assert(target->type == TERM);
			TargetPoint(&nextpoint, target);
		}
		assert(pcb_point_in_box(&path->sbox, nextpoint.X, nextpoint.Y));
		RD_DrawThermal(rd, nextpoint.X, nextpoint.Y, path->group, path->layer, subnet, is_bad);
	}
	else {
		/* start from best place of target box */
		lastpoint.X = PCB_BOX_CENTER_X(target->sbox);
		lastpoint.Y = PCB_BOX_CENTER_Y(target->sbox);
		TargetPoint(&lastpoint, target);
		if (AutoRouteParameters.last_smooth && pcb_box_in_box(&path->sbox, &target->sbox))
			path = path->parent.expansion_area;
		b = path->sbox;
		if (path->flags.circular)
			b = pcb_shrink_box(&b, MIN(b.X2 - b.X1, b.Y2 - b.Y1) / 5);
		nextpoint = pcb_closest_pcb_point_in_box(&lastpoint, &b);
		if (AutoRouteParameters.last_smooth)
			RD_DrawLine(rd, lastpoint.X, lastpoint.Y, nextpoint.X, nextpoint.Y, halfwidth, path->group, subnet, is_bad, pcb_true);
		else
			last_x = RD_DrawManhattanLine(rd, &target->sbox, &path->sbox,
																		lastpoint, nextpoint, halfwidth, path->group, subnet, is_bad, last_x);
	}
#if defined(ROUTE_DEBUG) && defined(DEBUG_SHOW_ROUTE_BOXES)
	showroutebox(path);
#if defined(ROUTE_VERBOSE)
	pcb_printf("TRACEPOINT start %#mD\n", nextpoint.X, nextpoint.Y);
#endif
#endif

	do {
		lastpoint = nextpoint;
		lastpath = path;
		assert(path->type == EXPANSION_AREA);
		path = path->parent.expansion_area;
		b = path->sbox;
		if (path->flags.circular)
			b = pcb_shrink_box(&b, MIN(b.X2 - b.X1, b.Y2 - b.Y1) / 5);
		assert(b.X1 != b.X2 && b.Y1 != b.Y2);	/* need someplace to put line! */
		/* find point on path perimeter closest to last point */
		/* if source terminal, try to hit a good place */
		nextpoint = pcb_closest_pcb_point_in_box(&lastpoint, &b);
#if 0
		/* leave more clearance if this is a smoothing pass */
		if (AutoRouteParameters.is_smoothing && (nextpoint.X != lastpoint.X || nextpoint.Y != lastpoint.Y))
			add_clearance(&nextpoint, &b);
#endif
		if (path->flags.source && path->type != PLANE)
			TargetPoint(&nextpoint, path);
		assert(pcb_point_in_box(&lastpath->box, lastpoint.X, lastpoint.Y));
		assert(pcb_point_in_box(&path->box, nextpoint.X, nextpoint.Y));
#if defined(ROUTE_DEBUG_VERBOSE)
		printf("TRACEPATH: ");
		DumpRouteBox(path);
		pcb_printf("TRACEPATH: point %#mD to point %#mD layer %d\n",
							 lastpoint.X, lastpoint.Y, nextpoint.X, nextpoint.Y, path->group);
#endif

		/* draw orthogonal lines from lastpoint to nextpoint */
		/* knee is placed in lastpath box */
		/* should never cause line to leave union of lastpath/path boxes */
		if (AutoRouteParameters.last_smooth)
			RD_DrawLine(rd, lastpoint.X, lastpoint.Y, nextpoint.X, nextpoint.Y, halfwidth, path->group, subnet, is_bad, pcb_true);
		else
			last_x = RD_DrawManhattanLine(rd, &lastpath->sbox, &path->sbox,
																		lastpoint, nextpoint, halfwidth, path->group, subnet, is_bad, last_x);
		if (path->flags.is_via) {		/* if via, then add via */
#ifdef ROUTE_VERBOSE
			printf(" (vias)");
#endif
			assert(pcb_point_in_box(&path->box, nextpoint.X, nextpoint.Y));
			RD_DrawVia(rd, nextpoint.X, nextpoint.Y, radius, subnet, is_bad);
		}

		assert(lastpath->flags.is_via || path->group == lastpath->group);

#if defined(ROUTE_DEBUG) && defined(DEBUG_SHOW_ROUTE_BOXES)
		showroutebox(path);
#endif /* ROUTE_DEBUG && DEBUG_SHOW_ROUTE_BOXES */
		/* if this is connected to a plane, draw the thermal */
		if (path->flags.is_thermal || path->type == PLANE)
			RD_DrawThermal(rd, lastpoint.X, lastpoint.Y, path->group, path->layer, subnet, is_bad);
		/* when one hop from the source, make an extra path in *this* box */
		if (path->type == EXPANSION_AREA && path->parent.expansion_area->flags.source && path->parent.expansion_area->type != PLANE) {
			/* find special point on source (if it exists) */
			if (TargetPoint(&lastpoint, path->parent.expansion_area)) {
				lastpoint = closest_point_in_routebox(&lastpoint, path);
				b = shrink_routebox(path);
#if 0
				if (AutoRouteParameters.is_smoothing)
					add_clearance(&lastpoint, &b);
#else
				if (AutoRouteParameters.last_smooth)
					RD_DrawLine(rd, lastpoint.X, lastpoint.Y, nextpoint.X, nextpoint.Y, halfwidth, path->group, subnet, is_bad, pcb_true);
				else
#endif
					last_x = RD_DrawManhattanLine(rd, &b, &b, nextpoint, lastpoint, halfwidth, path->group, subnet, is_bad, last_x);
#if defined(ROUTE_DEBUG_VERBOSE)
				printf("TRACEPATH: ");
				DumpRouteBox(path);
				pcb_printf
					("TRACEPATH: (to source) point %#mD to point %#mD layer %d\n",
					 nextpoint.X, nextpoint.Y, lastpoint.X, lastpoint.Y, path->group);
#endif

				nextpoint = lastpoint;
			}
		}
	}
	while (!path->flags.source);
	/* flush the line queue */
	RD_DrawLine(rd, -1, 0, 0, 0, 0, 0, NULL, pcb_false, pcb_false);

	if (conf_core.editor.live_routing)
		pcb_draw();

#ifdef ROUTE_DEBUG
	if (ddraw != NULL)
		ddraw->flush_debug_draw();
#endif
}

/* create a fake "edge" used to defer via site searching. */
static void
CreateSearchEdge(routeone_state_t *s, vetting_t * work, edge_t * parent,
								 routebox_t * rb, conflict_t conflict, pcb_rtree_t * targets, pcb_bool in_plane)
{
	routebox_t *target;
	pcb_box_t b;
	pcb_cost_t cost;
	assert(__routepcb_box_is_good(rb));
	/* find the cheapest target */
#if 0
	target = minpcb_cost_target_to_point(&parent->cost_point, pcb_max_group(PCB) + 1, targets, parent->minpcb_cost_target);
#else
	target = parent->minpcb_cost_target;
#endif
	b = shrink_routebox(target);
	cost = parent->pcb_cost_to_point + AutoRouteParameters.ViaCost + pcb_cost_to_layerless_box(&rb->cost_point, 0, &b);
	if (cost < s->best_cost) {
		edge_t *ne;
		ne = (edge_t *) malloc(sizeof(*ne));
		memset((void *) ne, 0, sizeof(*ne));
		assert(ne);
		ne->flags.via_search = 1;
		ne->flags.in_plane = in_plane;
		ne->rb = rb;
		if (rb->flags.homeless)
			RB_up_count(rb);
		ne->work = work;
		ne->minpcb_cost_target = target;
		ne->flags.via_conflict_level = conflict;
		ne->pcb_cost_to_point = parent->pcb_cost_to_point;
		ne->cost_point = parent->cost_point;
		ne->cost = cost;
		pcb_heap_insert(s->workheap, ne->cost, ne);
	}
	else {
		mtsFreeWork(&work);
	}
}

static void add_or_destroy_edge(routeone_state_t *s, edge_t * e)
{
	e->cost = edge_cost(e, s->best_cost);
	assert(__edge_is_good(e));
	assert(is_layer_group_active[e->rb->group]);
	if (e->cost < s->best_cost)
		pcb_heap_insert(s->workheap, e->cost, e);
	else
		DestroyEdge(&e);
}

static void best_path_candidate(routeone_state_t *s, edge_t * e, routebox_t * best_target)
{
	e->cost = edge_cost(e, EXPENSIVE);
	if (s->best_path == NULL || e->cost < s->best_cost) {
#if defined(ROUTE_DEBUG) && defined (ROUTE_VERBOSE)
		printf("New best path seen! cost = %f\n", e->cost);
#endif
		/* new best path! */
		if (s->best_path && s->best_path->flags.homeless)
			RB_down_count(s->best_path);
		s->best_path = e->rb;
		s->best_target = best_target;
		s->best_cost = e->cost;
		assert(s->best_cost >= 0);
		/* don't free this when we destroy edge! */
		if (s->best_path->flags.homeless)
			RB_up_count(s->best_path);
	}
}


/* vectors for via site candidates (see mtspace.h) */
struct routeone_via_site_state {
	vector_t *free_space_vec;
	vector_t *lo_conflict_space_vec;
	vector_t *hi_conflict_space_vec;
};

void
add_via_sites(routeone_state_t *s,
							struct routeone_via_site_state *vss,
							mtspace_t * mtspace, routebox_t * within,
							conflict_t within_conflict_level, edge_t * parent_edge, pcb_rtree_t * targets, pcb_coord_t shrink, pcb_bool in_plane)
{
	pcb_coord_t radius, clearance;
	vetting_t *work;
	pcb_box_t region = shrink_routebox(within);
	pcb_shrink_box(&region, shrink);

	radius = HALF_THICK(AutoRouteParameters.style->Diameter);
	clearance = AutoRouteParameters.style->Clearance;
	assert(AutoRouteParameters.use_vias);
	/* XXX: need to clip 'within' to shrunk_pcb_bounds, because when
	   XXX: routing with conflicts may poke over edge. */

	/* ask for a via box near our cost_point first */
	work = mtspace_query_rect(mtspace, &region, radius, clearance,
														NULL, vss->free_space_vec,
														vss->lo_conflict_space_vec,
														vss->hi_conflict_space_vec,
														AutoRouteParameters.is_odd, AutoRouteParameters.with_conflicts, &parent_edge->cost_point);
	if (!work)
		return;
	CreateSearchEdge(s, work, parent_edge, within, within_conflict_level, targets, in_plane);
}

void
do_via_search(edge_t * search, routeone_state_t *s,
							struct routeone_via_site_state *vss, mtspace_t * mtspace, pcb_rtree_t * targets)
{
	int i, j, count = 0;
	pcb_coord_t radius, clearance;
	vetting_t *work;
	routebox_t *within;
	conflict_t within_conflict_level;

	radius = HALF_THICK(AutoRouteParameters.style->Diameter);
	clearance = AutoRouteParameters.style->Clearance;
	work = mtspace_query_rect(mtspace, NULL, 0, 0,
														search->work, vss->free_space_vec,
														vss->lo_conflict_space_vec,
														vss->hi_conflict_space_vec, AutoRouteParameters.is_odd, AutoRouteParameters.with_conflicts, NULL);
	within = search->rb;
	within_conflict_level = search->flags.via_conflict_level;
	for (i = 0; i < 3; i++) {
		vector_t *v =
			(i == NO_CONFLICT ? vss->free_space_vec :
			 i == LO_CONFLICT ? vss->lo_conflict_space_vec : i == HI_CONFLICT ? vss->hi_conflict_space_vec : NULL);
		assert(v);
		while (!vector_is_empty(v)) {
			pcb_box_t cliparea;
			pcb_box_t *area = (pcb_box_t *) vector_remove_last(v);
			if (!(i == NO_CONFLICT || AutoRouteParameters.with_conflicts)) {
				free(area);
				continue;
			}
			/* answers are bloated by radius + clearance */
			cliparea = pcb_shrink_box(area, radius + clearance);
			pcb_close_box(&cliparea);
			free(area);
			assert(pcb_box_is_good(&cliparea));
			count++;
			for (j = 0; j < pcb_max_group(PCB); j++) {
				edge_t *ne;
				if (j == within->group || !is_layer_group_active[j])
					continue;
				ne = CreateViaEdge(&cliparea, j, within, search, within_conflict_level, (conflict_t) i, targets);
				add_or_destroy_edge(s, ne);
			}
		}
	}
	/* prevent freeing of work when this edge is destroyed */
	search->flags.via_search = 0;
	if (!work)
		return;
	CreateSearchEdge(s, work, search, within, within_conflict_level, targets, search->flags.in_plane);
	assert(vector_is_empty(vss->free_space_vec));
	assert(vector_is_empty(vss->lo_conflict_space_vec));
	assert(vector_is_empty(vss->hi_conflict_space_vec));
}

/* vector of expansion areas to be eventually removed from r-tree
 * this is a global for troubleshooting
 */
vector_t *area_vec;

/* some routines for use in gdb while debugging */
#if defined(ROUTE_DEBUG)
static void list_conflicts(routebox_t * rb)
{
	int i, n;
	if (!rb->conflicts_with)
		return;
	n = vector_size(rb->conflicts_with);
	for (i = 0; i < n; i++)
		printf("%p, ", (void *)vector_element(rb->conflicts_with, i));
}

static void show_area_vec(int lay)
{
	int n, save;

	if (!area_vec)
		return;
	save = showboxen;
	showboxen = lay;
	for (n = 0; n < vector_size(area_vec); n++) {
		routebox_t *rb = (routebox_t *) vector_element(area_vec, n);
		showroutebox(rb);
	}
	showboxen = save;
}

static pcb_bool net_id(routebox_t * rb, long int id)
{
	routebox_t *p;
	LIST_LOOP(rb, same_net, p);
	if (p->flags.source && p->parent.pad->ID == id)
		return pcb_true;
	PCB_END_LOOP;
	return pcb_false;
}

static void trace_parents(routebox_t * rb)
{
	while (rb && rb->type == EXPANSION_AREA) {
		printf(" %p ->", (void *)rb);
		rb = rb->parent.expansion_area;
	}
	if (rb)
		printf(" %p is source\n", (void *)rb);
	else
		printf("NULL!\n");
}

static void show_one(routebox_t * rb)
{
	int save = showboxen;
	showboxen = -1;
	showroutebox(rb);
	showboxen = save;
}

static void show_path(routebox_t * rb)
{
	while (rb && rb->type == EXPANSION_AREA) {
		show_one(rb);
		rb = rb->parent.expansion_area;
	}
	show_one(rb);
}

static void show_sources(routebox_t * rb)
{
	routebox_t *p;
	if (!rb->flags.source && !rb->flags.target) {
		printf("start with a source or target please\n");
		return;
	}
	LIST_LOOP(rb, same_net, p);
	if (p->flags.source)
		show_one(p);
	PCB_END_LOOP;
}

#endif

static pcb_r_dir_t __conflict_source(const pcb_box_t * box, void *cl)
{
	routebox_t *rb = (routebox_t *) box;
	if (rb->flags.touched || rb->flags.fixed)
		return PCB_R_DIR_NOT_FOUND;
	else {
		routebox_t *dis = (routebox_t *) cl;
		path_conflicts(dis, rb, pcb_false);
		touch_conflicts(dis->conflicts_with, 1);
	}
	return PCB_R_DIR_FOUND_CONTINUE;
}

static void source_conflicts(pcb_rtree_t * tree, routebox_t * rb)
{
	if (!AutoRouteParameters.with_conflicts)
		return;
	pcb_r_search(tree, &rb->sbox, NULL, __conflict_source, rb, NULL);
	touch_conflicts(NULL, 1);
}

typedef struct routeone_status_s {
	pcb_bool found_route;
	int route_had_conflicts;
	pcb_cost_t best_route_cost;
	pcb_bool net_completely_routed;
} routeone_status_t;


static routeone_status_t RouteOne(routedata_t * rd, routebox_t * from, routebox_t * to, int max_edges)
{
	routeone_status_t result;
	routebox_t *p;
	int seen, i;
	const pcb_box_t **target_list;
	int num_targets;
	pcb_rtree_t *targets;
	/* vector of source edges for filtering */
	vector_t *source_vec;
	/* working vector */
	vector_t *edge_vec;

	routeone_state_t s;
	struct routeone_via_site_state vss;

	assert(rd && from);
	result.route_had_conflicts = 0;
	/* no targets on to/from net need clearance areas */
	LIST_LOOP(from, same_net, p);
	p->flags.nobloat = 1;
	PCB_END_LOOP;
	/* set 'source' flags */
	LIST_LOOP(from, same_subnet, p);
	if (!p->flags.nonstraight)
		p->flags.source = 1;
	PCB_END_LOOP;

	/* count up the targets */
	num_targets = 0;
	seen = 0;
	/* remove source/target flags from non-straight obstacles, because they
	 * don't fill their bounding boxes and so connecting to them
	 * after we've routed is problematic.  Better solution? */
	if (to) {											/* if we're routing to a specific target */
		if (!to->flags.source) {		/* not already connected */
			/* check that 'to' and 'from' are on the same net */
			seen = 0;
#ifndef NDEBUG
			LIST_LOOP(from, same_net, p);
			if (p == to)
				seen = 1;
			PCB_END_LOOP;
#endif
			assert(seen);							/* otherwise from and to are on different nets! */
			/* set target flags only on 'to's subnet */
			LIST_LOOP(to, same_subnet, p);
			if (!p->flags.nonstraight && is_layer_group_active[p->group]) {
				p->flags.target = 1;
				num_targets++;
			}
			PCB_END_LOOP;
		}
	}
	else {
		/* all nodes on the net but not connected to from are targets */
		LIST_LOOP(from, same_net, p);
		if (!p->flags.source && is_layer_group_active[p->group]
				&& !p->flags.nonstraight) {
			p->flags.target = 1;
			num_targets++;
		}
		PCB_END_LOOP;
	}

	/* if no targets, then net is done!  reset flags and return. */
	if (num_targets == 0) {
		LIST_LOOP(from, same_net, p);
		p->flags.source = p->flags.target = p->flags.nobloat = 0;
		PCB_END_LOOP;
		result.found_route = pcb_false;
		result.net_completely_routed = pcb_true;
		result.best_route_cost = 0;
		result.route_had_conflicts = 0;

		return result;
	}
	result.net_completely_routed = pcb_false;

	/* okay, there's stuff to route */
	assert(!from->flags.target);
	assert(num_targets > 0);
	/* create list of target pointers and from that a r-tree of targets */
	target_list = (const pcb_box_t **) malloc(num_targets * sizeof(*target_list));
	i = 0;
	LIST_LOOP(from, same_net, p);
	if (p->flags.target) {
		target_list[i++] = &p->box;
#if defined(ROUTE_DEBUG) && defined(DEBUG_SHOW_TARGETS)
		showroutebox(p);
#endif
	}
	PCB_END_LOOP;
	targets = pcb_r_create_tree();
	pcb_r_insert_array(targets, (const pcb_box_t **)target_list, i);
	assert(i <= num_targets);
	free(target_list);

	source_vec = vector_create();
	/* touch the source subnet to prepare check for conflictors */
	LIST_LOOP(from, same_subnet, p);
	p->flags.touched = 1;
	PCB_END_LOOP;
	LIST_LOOP(from, same_subnet, p);
	{
		/* we need the test for 'source' because this box may be nonstraight */
		if (p->flags.source && is_layer_group_active[p->group]) {
			pcb_cheap_point_t cp;
			edge_t *e;
			pcb_box_t b = shrink_routebox(p);

#if defined(ROUTE_DEBUG) && defined(DEBUG_SHOW_SOURCES)
			showroutebox(p);
#endif
			/* may expand in all directions from source; center edge cost point. */
			/* note that planes shouldn't really expand, but we need an edge */

			cp.X = PCB_BOX_CENTER_X(b);
			cp.Y = PCB_BOX_CENTER_Y(b);
			e = CreateEdge(p, cp.X, cp.Y, 0, NULL, PCB_ANY_DIR, targets);
			cp = pcb_closest_pcb_point_in_box(&cp, &e->minpcb_cost_target->sbox);
			cp = pcb_closest_pcb_point_in_box(&cp, &b);
			e->cost_point = cp;
			p->cost_point = cp;
			source_conflicts(rd->layergrouptree[p->group], p);
			vector_append(source_vec, e);
		}
	}
	PCB_END_LOOP;
	LIST_LOOP(from, same_subnet, p);
	p->flags.touched = 0;
	PCB_END_LOOP;
	/* break source edges; some edges may be too near obstacles to be able
	 * to exit from. */

	/* okay, main expansion-search routing loop. */
	/* set up the initial activity heap */
	s.workheap = pcb_heap_create();
	assert(s.workheap);
	while (!vector_is_empty(source_vec)) {
		edge_t *e = (edge_t *) vector_remove_last(source_vec);
		assert(is_layer_group_active[e->rb->group]);
		e->cost = edge_cost(e, EXPENSIVE);
		pcb_heap_insert(s.workheap, e->cost, e);
	}
	vector_destroy(&source_vec);
	/* okay, process items from heap until it is empty! */
	s.best_path = NULL;
	s.best_cost = EXPENSIVE;
	area_vec = vector_create();
	edge_vec = vector_create();
	vss.free_space_vec = vector_create();
	vss.lo_conflict_space_vec = vector_create();
	vss.hi_conflict_space_vec = vector_create();
	while (!pcb_heap_is_empty(s.workheap)) {
		edge_t *e = (edge_t *) pcb_heap_remove_smallest(s.workheap);
#ifdef ROUTE_DEBUG
		if (aabort)
			goto dontexpand;
#endif
		/* don't bother expanding this edge if the minimum possible edge cost
		 * is already larger than the best edge cost we've found. */
		if (s.best_path && e->cost >= s.best_cost) {
			pcb_heap_free(s.workheap, KillEdge);
			goto dontexpand;					/* skip this edge */
		}
		/* surprisingly it helps to give up and not try too hard to find
		 * a route! This is not only faster, but results in better routing.
		 * who would have guessed?
		 */
		if (seen++ > max_edges)
			goto dontexpand;
		assert(__edge_is_good(e));
		/* mark or unmark conflictors as needed */
		touch_conflicts(e->rb->conflicts_with, 1);
		if (e->flags.via_search) {
			do_via_search(e, &s, &vss, rd->mtspace, targets);
			goto dontexpand;
		}
		/* we should never add edges on inactive layer groups to the heap. */
		assert(is_layer_group_active[e->rb->group]);
#if defined(ROUTE_DEBUG) && defined(DEBUG_SHOW_EXPANSION_BOXES)
		/*showedge (e); */
#endif
		if (e->rb->flags.is_thermal) {
			best_path_candidate(&s, e, e->minpcb_cost_target);
			goto dontexpand;
		}
		/* for a plane, look for quick connections with thermals or vias */
		if (e->rb->type == PLANE) {
			routebox_t *pin = FindThermable(targets, e->rb);
			if (pin) {
				pcb_box_t b = shrink_routebox(pin);
				edge_t *ne;
				routebox_t *nrb;
				assert(pin->flags.target);
				nrb = CreateExpansionArea(&b, e->rb->group, e->rb, pcb_true, e);
				nrb->flags.is_thermal = 1;
				/* moving through the plane is free */
				e->cost_point.X = b.X1;
				e->cost_point.Y = b.Y1;
				ne = CreateEdge2(nrb, e->expand_dir, e, NULL, pin);
				best_path_candidate(&s, ne, pin);
				DestroyEdge(&ne);
			}
			else {
				/* add in possible via sites in plane */
				if (AutoRouteParameters.use_vias && e->cost + AutoRouteParameters.ViaCost < s.best_cost) {
					/* we need a giant thermal */
					routebox_t *nrb = CreateExpansionArea(&e->rb->sbox, e->rb->group, e->rb,
																								pcb_true, e);
					edge_t *ne = CreateEdge2(nrb, e->expand_dir, e, NULL,
																	 e->minpcb_cost_target);
					nrb->flags.is_thermal = 1;
					add_via_sites(&s, &vss, rd->mtspace, nrb, NO_CONFLICT, ne, targets, e->rb->style->Diameter, pcb_true);
				}
			}
			goto dontexpand;					/* planes only connect via thermals */
		}
		if (e->flags.is_via) {			/* special case via */
			routebox_t *intersecting;
			assert(AutoRouteParameters.use_vias);
			assert(e->expand_dir == PCB_ANY_DIR);
			assert(vector_is_empty(edge_vec));
			/* if there is already something here on this layer (like an
			 * EXPANSION_AREA), then we don't want to expand from here
			 * at least not inside the expansion area. A PLANE on the
			 * other hand may be a target, or not.
			 */
			intersecting = FindOneInBox(rd->layergrouptree[e->rb->group], e->rb);

			if (intersecting && intersecting->flags.target && intersecting->type == PLANE) {
				/* we have hit a plane */
				edge_t *ne;
				routebox_t *nrb;
				pcb_box_t b = shrink_routebox(e->rb);
				/* limit via region to that inside the plane */
				pcb_clip_box(&b, &intersecting->sbox);
				nrb = CreateExpansionArea(&b, e->rb->group, e->rb, pcb_true, e);
				nrb->flags.is_thermal = 1;
				ne = CreateEdge2(nrb, e->expand_dir, e, NULL, intersecting);
				best_path_candidate(&s, ne, intersecting);
				DestroyEdge(&ne);
				goto dontexpand;
			}
			else if (intersecting == NULL) {
				/* this via candidate is in an open area; add it to r-tree as
				 * an expansion area */
				assert(e->rb->type == EXPANSION_AREA && e->rb->flags.is_via);
				/*assert (!pcb_r_search(rd->layergrouptree[e->rb->group],
				   &e->rb->box, NULL, no_planes,0));
				 */
				pcb_r_insert_entry(rd->layergrouptree[e->rb->group], &e->rb->box);
				e->rb->flags.homeless = 0;	/* not homeless any more */
				/* add to vector of all expansion areas in r-tree */
				vector_append(area_vec, e->rb);
				/* mark reset refcount to 0, since this is not homeless any more. */
				e->rb->refcount = 0;
				/* go ahead and expand this edge! */
			}
			else if (1)
				goto dontexpand;
			else if (0) {							/* XXX: disabling this causes no via
																   collisions. */
				pcb_box_t a = bloat_routebox(intersecting), b;
				edge_t *ne;
				int i, j;
				/* something intersects this via candidate.  split via candidate
				 * into pieces and add these pieces to the workheap. */
				for (i = 0; i < 3; i++) {
					for (j = 0; j < 3; j++) {
						b = shrink_routebox(e->rb);
						switch (i) {
						case 0:
							b.X2 = MIN(b.X2, a.X1);
							break;						/* left */
						case 1:
							b.X1 = MAX(b.X1, a.X1);
							b.X2 = MIN(b.X2, a.X2);
							break;						/*c */
						case 2:
							b.X1 = MAX(b.X1, a.X2);
							break;						/* right */
						default:
							assert(0);
						}
						switch (j) {
						case 0:
							b.Y2 = MIN(b.Y2, a.Y1);
							break;						/* top */
						case 1:
							b.Y1 = MAX(b.Y1, a.Y1);
							b.Y2 = MIN(b.Y2, a.Y2);
							break;						/*c */
						case 2:
							b.Y1 = MAX(b.Y1, a.Y2);
							break;						/* bottom */
						default:
							assert(0);
						}
						/* skip if this box is not valid */
						if (!(b.X1 < b.X2 && b.Y1 < b.Y2))
							continue;
						if (i == 1 && j == 1) {
							/* this bit of the via space is obstructed. */
							if (intersecting->type == EXPANSION_AREA || intersecting->flags.fixed)
								continue;				/* skip this bit, it's already been done. */
							/* create an edge with conflicts, if enabled */
							if (!AutoRouteParameters.with_conflicts)
								continue;
							ne = CreateEdgeWithConflicts(&b, intersecting, e, 1
																					 /*cost penalty to box */
																					 , targets);
							add_or_destroy_edge(&s, ne);
						}
						else {
							/* if this is not the intersecting piece, create a new
							 * (hopefully unobstructed) via edge and add it back to the
							 * workheap. */
							ne = CreateViaEdge(&b, e->rb->group, e->rb->parent.expansion_area, e, e->flags.via_conflict_level, NO_CONFLICT
																 /* value here doesn't matter */
																 , targets);
							add_or_destroy_edge(&s, ne);
						}
					}
				}
				goto dontexpand;
			}
			/* between the time these edges are inserted and the
			 * time they are processed, new expansion boxes (which
			 * conflict with these edges) may be added to the graph!
			 * w.o vias this isn't a problem because the broken box
			 * is not homeless. */
		}
		if (1) {
			routebox_t *nrb;
			struct E_result *ans;
			pcb_box_t b;
			vector_t *broken;
			if (e->flags.is_interior) {
				assert(AutoRouteParameters.with_conflicts);	/* no interior edges unless
																										   routing with conflicts! */
				assert(e->rb->conflicts_with);
				b = e->rb->sbox;
				switch (e->rb->came_from) {
				case PCB_NORTH:
					b.Y2 = b.Y1 + 1;
					b.X1 = PCB_BOX_CENTER_X(b);
					b.X2 = b.X1 + 1;
					break;
				case PCB_EAST:
					b.X1 = b.X2 - 1;
					b.Y1 = PCB_BOX_CENTER_Y(b);
					b.Y2 = b.Y1 + 1;
					break;
				case PCB_SOUTH:
					b.Y1 = b.Y2 - 1;
					b.X1 = PCB_BOX_CENTER_X(b);
					b.X2 = b.X1 + 1;
					break;
				case PCB_WEST:
					b.X2 = b.X1 + 1;
					b.Y1 = PCB_BOX_CENTER_Y(b);
					b.Y2 = b.Y1 + 1;
					break;
				default:
					assert(0);
				}
			}
			/* sources may not expand to their own edges because of
			 * adjacent blockers.
			 */
			else if (e->rb->flags.source)
				b = pcb_box_center(&e->rb->sbox);
			else
				b = e->rb->sbox;
			ans = Expand(rd->layergrouptree[e->rb->group], e, &b);
			if (!pcb_box_intersect(&ans->inflated, &ans->orig))
				goto dontexpand;
#if 0
			/* skip if it didn't actually expand */
			if (ans->inflated.X1 >= e->rb->sbox.X1 &&
					ans->inflated.X2 <= e->rb->sbox.X2 && ans->inflated.Y1 >= e->rb->sbox.Y1 && ans->inflated.Y2 <= e->rb->sbox.Y2)
				goto dontexpand;
#endif

			if (!pcb_box_is_good(&ans->inflated))
				goto dontexpand;
			nrb = CreateExpansionArea(&ans->inflated, e->rb->group, e->rb, pcb_true, e);
			pcb_r_insert_entry(rd->layergrouptree[nrb->group], &nrb->box);
			vector_append(area_vec, nrb);
			nrb->flags.homeless = 0;	/* not homeless any more */
			broken = BreakManyEdges(&s, targets, rd->layergrouptree[nrb->group], area_vec, ans, nrb, e);
			while (!vector_is_empty(broken)) {
				edge_t *ne = (edge_t *) vector_remove_last(broken);
				add_or_destroy_edge(&s, ne);
			}
			vector_destroy(&broken);

			/* add in possible via sites in nrb */
			if (AutoRouteParameters.use_vias && !e->rb->flags.is_via && e->cost + AutoRouteParameters.ViaCost < s.best_cost)
				add_via_sites(&s, &vss, rd->mtspace, nrb, NO_CONFLICT, e, targets, 0, pcb_false);
			goto dontexpand;
		}
	dontexpand:
		DestroyEdge(&e);
	}
	touch_conflicts(NULL, 1);
	pcb_heap_destroy(&s.workheap);
	pcb_r_destroy_tree(&targets);
	assert(vector_is_empty(edge_vec));
	vector_destroy(&edge_vec);

	/* we should have a path in best_path now */
	if (s.best_path) {
		routebox_t *rb;
#ifdef ROUTE_VERBOSE
		printf("%d:%d RC %.0f", ro++, seen, s.best_cost);
#endif
		result.found_route = pcb_true;
		result.best_route_cost = s.best_cost;
		/* determine if the best path had conflicts */
		result.route_had_conflicts = 0;
		if (AutoRouteParameters.with_conflicts && s.best_path->conflicts_with) {
			while (!vector_is_empty(s.best_path->conflicts_with)) {
				rb = (routebox_t *) vector_remove_last(s.best_path->conflicts_with);
				rb->flags.is_bad = 1;
				result.route_had_conflicts++;
			}
		}
#ifdef ROUTE_VERBOSE
		if (result.route_had_conflicts)
			printf(" (%d conflicts)", result.route_had_conflicts);
#endif
		if (result.route_had_conflicts < AutoRouteParameters.hi_conflict) {
			/* back-trace the path and add lines/vias to r-tree */
			TracePath(rd, s.best_path, s.best_target, from, result.route_had_conflicts);
			MergeNets(from, s.best_target, SUBNET);
		}
		else {
#ifdef ROUTE_VERBOSE
			printf(" (too many in fact)");
#endif
			result.found_route = pcb_false;
		}
#ifdef ROUTE_VERBOSE
		printf("\n");
#endif
	}
	else {
#ifdef ROUTE_VERBOSE
		printf("%d:%d NO PATH FOUND.\n", ro++, seen);
#endif
		result.best_route_cost = s.best_cost;
		result.found_route = pcb_false;
	}
	/* now remove all expansion areas from the r-tree. */
	while (!vector_is_empty(area_vec)) {
		routebox_t *rb = (routebox_t *) vector_remove_last(area_vec);
		assert(!rb->flags.homeless);
		if (rb->conflicts_with && rb->parent.expansion_area->conflicts_with != rb->conflicts_with)
			vector_destroy(&rb->conflicts_with);
		pcb_r_delete_entry_free_data(rd->layergrouptree[rb->group], &rb->box, free);
	}
	vector_destroy(&area_vec);
	/* clean up; remove all 'source', 'target', and 'nobloat' flags */
	LIST_LOOP(from, same_net, p);
	if (p->flags.source && p->conflicts_with)
		vector_destroy(&p->conflicts_with);
	p->flags.touched = p->flags.source = p->flags.target = p->flags.nobloat = 0;
	PCB_END_LOOP;

	vector_destroy(&vss.free_space_vec);
	vector_destroy(&vss.lo_conflict_space_vec);
	vector_destroy(&vss.hi_conflict_space_vec);

	return result;
}

static void InitAutoRouteParameters(int pass, pcb_route_style_t * style, pcb_bool with_conflicts, pcb_bool is_smoothing, pcb_bool lastpass)
{
	int i;
	/* routing style */
	AutoRouteParameters.style = style;
	AutoRouteParameters.bloat = style->Clearance + HALF_THICK(style->Thick);
	/* costs */
	AutoRouteParameters.ViaCost = PCB_INCH_TO_COORD(3.5) + style->Diameter * (is_smoothing ? 80 : 30);
	AutoRouteParameters.LastConflictPenalty = (400 * pass / passes + 2) / (pass + 1);
	AutoRouteParameters.ConflictPenalty = 4 * AutoRouteParameters.LastConflictPenalty;
	AutoRouteParameters.JogPenalty = 1000 * (is_smoothing ? 20 : 4);
	AutoRouteParameters.CongestionPenalty = 1e6;
	AutoRouteParameters.MinPenalty = EXPENSIVE;
	for (i = 0; i < pcb_max_group(PCB); i++) {
		if (is_layer_group_active[i]) {
			AutoRouteParameters.MinPenalty = MIN(x_cost[i], AutoRouteParameters.MinPenalty);
			AutoRouteParameters.MinPenalty = MIN(y_cost[i], AutoRouteParameters.MinPenalty);
		}
	}
	AutoRouteParameters.NewLayerPenalty = is_smoothing ? 0.5 * EXPENSIVE : 10 * AutoRouteParameters.ViaCost;
	/* other */
	AutoRouteParameters.hi_conflict = MAX(8 * (passes - pass + 1), 6);
	AutoRouteParameters.is_odd = (pass & 1);
	AutoRouteParameters.with_conflicts = with_conflicts;
	AutoRouteParameters.is_smoothing = is_smoothing;
	AutoRouteParameters.rip_always = is_smoothing;
	AutoRouteParameters.last_smooth = 0;	/*lastpass; */
	AutoRouteParameters.pass = pass + 1;
}

#ifndef NDEBUG
pcb_r_dir_t bad_boy(const pcb_box_t * b, void *cl)
{
	routebox_t *box = (routebox_t *) b;
	if (box->type == EXPANSION_AREA)
		return PCB_R_DIR_FOUND_CONTINUE;
	return PCB_R_DIR_NOT_FOUND;
}

pcb_bool no_expansion_boxes(routedata_t * rd)
{
	int i;
	pcb_box_t big;
	big.X1 = 0;
	big.X2 = PCB_MAX_COORD;
	big.Y1 = 0;
	big.Y2 = PCB_MAX_COORD;
	for (i = 0; i < pcb_max_group(PCB); i++) {
		if (pcb_r_search(rd->layergrouptree[i], &big, NULL, bad_boy, NULL, NULL))
			return pcb_false;
	}
	return pcb_true;
}
#endif

static void ripout_livedraw_obj(routebox_t * rb)
{
	if (rb->type == LINE && rb->livedraw_obj.line) {
		pcb_layer_t *layer = LAYER_PTR(PCB->LayerGroups.grp[rb->group].lid[0]);
		pcb_line_invalidate_erase(rb->livedraw_obj.line);
		pcb_destroy_object(PCB->Data, PCB_OBJ_LINE, layer, rb->livedraw_obj.line, NULL);
		rb->livedraw_obj.line = NULL;
	}

	if (rb->type == VIA && rb->livedraw_obj.via) {
		pcb_pstk_invalidate_erase(rb->livedraw_obj.via);
		pcb_destroy_object(PCB->Data, PCB_OBJ_PSTK, rb->livedraw_obj.via, NULL, NULL);
		rb->livedraw_obj.via = NULL;
	}
}

static pcb_r_dir_t ripout_livedraw_obj_cb(const pcb_box_t * b, void *cl)
{
	routebox_t *box = (routebox_t *) b;
	ripout_livedraw_obj(box);
	return PCB_R_DIR_NOT_FOUND;
}

struct routeall_status {
	/* --- for completion rate statistics ---- */
	int total_subnets;
	/* total subnets routed without conflicts */
	int routed_subnets;
	/* total subnets routed with conflicts */
	int conflict_subnets;
	/* net failted entirely */
	int failed;
	/* net was ripped */
	int ripped;
	int total_nets_routed;
};

static double calculate_progress(double this_heap_item, double this_heap_size, struct routeall_status *ras)
{
	double total_passes = passes + smoothes + 1;	/* + 1 is the refinement pass */
	double this_pass = AutoRouteParameters.pass - 1;	/* Number passes from zero */
	double heap_fraction = (double) (ras->routed_subnets + ras->conflict_subnets + ras->failed) / (double) ras->total_subnets;
	double pass_fraction = (this_heap_item + heap_fraction) / this_heap_size;
	double process_fraction = (this_pass + pass_fraction) / total_passes;

	return process_fraction;
}

struct routeall_status RouteAll(routedata_t * rd)
{
	struct routeall_status ras;
	routeone_status_t ros;
	pcb_bool rip;
	int request_cancel;
#ifdef NET_HEAP
	pcb_heap_t *net_heap;
#endif
	pcb_heap_t *this_pass, *next_pass, *tmp;
	routebox_t *net, *p, *pp;
	pcb_cost_t total_net_cost, last_cost = 0, this_cost = 0;
	int i;
	int this_heap_size;
	int this_heap_item;

	/* initialize heap for first pass;
	 * do smallest area first; that makes
	 * the subsequent costs more representative */
	this_pass = pcb_heap_create();
	next_pass = pcb_heap_create();
#ifdef NET_HEAP
	net_heap = pcb_heap_create();
#endif
	LIST_LOOP(rd->first_net, different_net, net);
	{
		double area;
		pcb_box_t bb = shrink_routebox(net);
		LIST_LOOP(net, same_net, p);
		{
			PCB_MAKE_MIN(bb.X1, p->sbox.X1);
			PCB_MAKE_MIN(bb.Y1, p->sbox.Y1);
			PCB_MAKE_MAX(bb.X2, p->sbox.X2);
			PCB_MAKE_MAX(bb.Y2, p->sbox.Y2);
		}
		PCB_END_LOOP;
		area = (double) (bb.X2 - bb.X1) * (bb.Y2 - bb.Y1);
		pcb_heap_insert(this_pass, area, net);
	}
	PCB_END_LOOP;

	ras.total_nets_routed = 0;
	/* refinement/finishing passes */
	for (i = 0; i <= passes + smoothes; i++) {
#ifdef ROUTE_VERBOSE
		if (i > 0 && i <= passes)
			printf("--------- STARTING REFINEMENT PASS %d ------------\n", i);
		else if (i > passes)
			printf("--------- STARTING SMOOTHING PASS %d -------------\n", i - passes);
#endif
		ras.total_subnets = ras.routed_subnets = ras.conflict_subnets = ras.failed = ras.ripped = 0;
		assert(pcb_heap_is_empty(next_pass));

		this_heap_size = pcb_heap_size(this_pass);
		for (this_heap_item = 0; !pcb_heap_is_empty(this_pass); this_heap_item++) {
#ifdef ROUTE_DEBUG
			if (aabort)
				break;
#endif
			net = (routebox_t *) pcb_heap_remove_smallest(this_pass);
			InitAutoRouteParameters(i, net->style, i < passes, i > passes, i == passes + smoothes);
			if (i > 0) {
				/* rip up all unfixed traces in this net ? */
				if (AutoRouteParameters.rip_always)
					rip = pcb_true;
				else {
					rip = pcb_false;
					LIST_LOOP(net, same_net, p);
					if (p->flags.is_bad) {
						rip = pcb_true;
						break;
					}
					PCB_END_LOOP;
				}

				LIST_LOOP(net, same_net, p);
				p->flags.is_bad = 0;
				if (!p->flags.fixed) {
#ifndef NDEBUG
					pcb_bool del;
#endif
					assert(!p->flags.homeless);
					if (rip) {
						RemoveFromNet(p, NET);
						RemoveFromNet(p, SUBNET);
					}
					if (AutoRouteParameters.use_vias && p->type != VIA_SHADOW && p->type != PLANE) {
						mtspace_remove(rd->mtspace, &p->box, p->flags.is_odd ? ODD : EVEN, p->style->Clearance);
						if (!rip)
							mtspace_add(rd->mtspace, &p->box, p->flags.is_odd ? EVEN : ODD, p->style->Clearance);
					}
					if (rip) {
						if (conf_core.editor.live_routing)
							ripout_livedraw_obj(p);
#ifndef NDEBUG
						del =
#endif
							pcb_r_delete_entry_free_data(rd->layergrouptree[p->group], &p->box, free);
#ifndef NDEBUG
						assert(del);
#endif
					}
					else {
						p->flags.is_odd = AutoRouteParameters.is_odd;
					}
				}
				PCB_END_LOOP;
				if (conf_core.editor.live_routing)
					pcb_draw();
				/* reset to original connectivity */
				if (rip) {
					ras.ripped++;
					ResetSubnet(net);
				}
				else {
					pcb_heap_insert(next_pass, 0, net);
					continue;
				}
			}
			/* count number of subnets */
			FOREACH_SUBNET(net, p);
			ras.total_subnets++;
			END_FOREACH(net, p);
			/* the first subnet doesn't require routing. */
			ras.total_subnets--;
			/* and re-route! */
			total_net_cost = 0;
			/* only route that which isn't fully routed */
#ifdef ROUTE_DEBUG
			if (ras.total_subnets == 0 || aabort)
#else
			if (ras.total_subnets == 0)
#endif
			{
				pcb_heap_insert(next_pass, 0, net);
				continue;
			}

			/* the loop here ensures that we get to all subnets even if
			 * some of them are unreachable from the first subnet. */
			LIST_LOOP(net, same_net, p);
			{
#ifdef NET_HEAP
				pcb_box_t b = shrink_routebox(p);
				/* using a heap allows us to start from smaller objects and
				 * end at bigger ones. also prefer to start at planes, then pads */
				pcb_heap_insert(net_heap, (float) (b.X2 - b.X1) *
#if defined(ROUTE_RANDOMIZED)
										(0.3 + pcb_rand() / (RAND_MAX + 1.0)) *
#endif
										(b.Y2 - b.Y1) * (p->type == PLANE ? -1 : ((p->type == TERM) ? 1 : 10)), p);
			}
			PCB_END_LOOP;
			ros.net_completely_routed = 0;
			while (!pcb_heap_is_empty(net_heap)) {
				p = (routebox_t *) pcb_heap_remove_smallest(net_heap);
#endif
				if (!p->flags.fixed || p->flags.subnet_processed || p->type == OTHER)
					continue;

				while (!ros.net_completely_routed) {
					double percent;

					assert(no_expansion_boxes(rd));
					/* FIX ME: the number of edges to examine should be in autoroute parameters
					 * i.e. the 2000 and 800 hard-coded below should be controllable by the user
					 */
					ros = RouteOne(rd, p, NULL, ((AutoRouteParameters.is_smoothing ? 2000 : 800) * (i + 1)) * routing_layers);
					total_net_cost += ros.best_route_cost;
					if (ros.found_route) {
						if (ros.route_had_conflicts)
							ras.conflict_subnets++;
						else {
							ras.routed_subnets++;
							ras.total_nets_routed++;
						}
					}
					else {
						if (!ros.net_completely_routed)
							ras.failed++;
						/* don't bother trying any other source in this subnet */
						LIST_LOOP(p, same_subnet, pp);
						pp->flags.subnet_processed = 1;
						PCB_END_LOOP;
						break;
					}
					/* note that we can infer nothing about ras.total_subnets based
					 * on the number of calls to RouteOne, because we may be unable
					 * to route a net from a particular starting point, but perfectly
					 * able to route it from some other. */
					percent = calculate_progress(this_heap_item, this_heap_size, &ras);
					request_cancel = pcb_hid_progress(percent * 100., 100, _("Autorouting tracks"));
					if (request_cancel) {
						ras.total_nets_routed = 0;
						ras.conflict_subnets = 0;
						pcb_message(PCB_MSG_INFO, "Autorouting cancelled\n");
						goto out;
					}
				}
			}
#ifndef NET_HEAP
			PCB_END_LOOP;
#endif
			if (!ros.net_completely_routed)
				net->flags.is_bad = 1;	/* don't skip this the next round */

			/* Route easiest nets from this pass first on next pass.
			 * This works best because it's likely that the hardest
			 * is the last one routed (since it has the most obstacles)
			 * but it will do no good to rip it up and try it again
			 * without first changing any of the other routes
			 */
			pcb_heap_insert(next_pass, total_net_cost, net);
			if (total_net_cost < EXPENSIVE)
				this_cost += total_net_cost;
			/* reset subnet_processed flags */
			LIST_LOOP(net, same_net, p);
			{
				p->flags.subnet_processed = 0;
			}
			PCB_END_LOOP;
		}
		/* swap this_pass and next_pass and do it all over again! */
		ro = 0;
		assert(pcb_heap_is_empty(this_pass));
		tmp = this_pass;
		this_pass = next_pass;
		next_pass = tmp;
#if defined(ROUTE_DEBUG) || defined (ROUTE_VERBOSE)
		printf
			("END OF PASS %d: %d/%d subnets routed without conflicts at cost %.0f, %d conflicts, %d failed %d ripped\n",
			 i, ras.routed_subnets, ras.total_subnets, this_cost, ras.conflict_subnets, ras.failed, ras.ripped);
#endif
#ifdef ROUTE_DEBUG
		if (aabort)
			break;
#endif
		/* if no conflicts found, skip directly to smoothing pass! */
		if (ras.conflict_subnets == 0 && ras.routed_subnets == ras.total_subnets && i <= passes)
			i = passes - (smoothes ? 0 : 1);
		/* if no changes in a smoothing round, then we're done */
		if (this_cost == last_cost && i > passes && i < passes + smoothes)
			i = passes + smoothes - 1;
		last_cost = this_cost;
		this_cost = 0;
	}

	pcb_message(PCB_MSG_INFO, "%d of %d nets successfully routed.\n", ras.routed_subnets, ras.total_subnets);

out:
	pcb_heap_destroy(&this_pass);
	pcb_heap_destroy(&next_pass);
#ifdef NET_HEAP
	pcb_heap_destroy(&net_heap);
#endif

	/* no conflicts should be left at the end of the process. */
	assert(ras.conflict_subnets == 0);

	return ras;
}

struct fpin_info {
	pcb_pstk_t *ps;
	pcb_coord_t x, y;
	jmp_buf env;
};

static pcb_r_dir_t fpstk_rect(const pcb_box_t * b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	struct fpin_info *info = (struct fpin_info *) cl;
	if (ps->x == info->x && ps->y == info->y) {
		info->ps = ps;
		longjmp(info->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

static int FindPin(const pcb_box_t *box, pcb_pstk_t **ps_out)
{
	struct fpin_info info;

	info.ps = NULL;
	info.x = box->X1;
	info.y = box->Y1;
	if (setjmp(info.env) == 0) {
		pcb_r_search(PCB->Data->padstack_tree, box, NULL, fpstk_rect, &info, NULL);
	}
	else {
		*ps_out = info.ps;
		return PCB_OBJ_PSTK;
	}

	*ps_out = NULL;
	return PCB_OBJ_VOID;
}


/* paths go on first 'on' layer in group */
/* returns 'pcb_true' if any paths were added. */
pcb_bool IronDownAllUnfixedPaths(routedata_t * rd)
{
	pcb_bool changed = pcb_false;
	pcb_layer_t *layer;
	routebox_t *net, *p;
	int i;
	LIST_LOOP(rd->first_net, different_net, net);
	{
		LIST_LOOP(net, same_net, p);
		{
			if (!p->flags.fixed) {
				/* find first on layer in this group */
				assert(PCB->LayerGroups.grp[p->group].len > 0);
				assert(is_layer_group_active[p->group]);
				for (i = 0, layer = NULL; i < PCB->LayerGroups.grp[p->group].len; i++) {
					layer = LAYER_PTR(PCB->LayerGroups.grp[p->group].lid[i]);
					if (layer->meta.real.vis)
						break;
				}
				assert(layer && layer->meta.real.vis);	/*at least one layer must be on in this group! */
				assert(p->type != EXPANSION_AREA);
				if (p->type == LINE) {
					pcb_coord_t halfwidth = HALF_THICK(p->style->Thick);
					double th = halfwidth * 2 + 1;
					pcb_box_t b;
					assert(p->parent.line == NULL);
					/* orthogonal; thickness is 2*halfwidth */
					/* flip coordinates, if bl_to_ur */
					b = p->sbox;
					total_wire_length += sqrt((b.X2 - b.X1 - th) * (b.X2 - b.X1 - th) + (b.Y2 - b.Y1 - th) * (b.Y2 - b.Y1 - th));
					b = pcb_shrink_box(&b, halfwidth);
					if (b.X2 == b.X1 + 1)
						b.X2 = b.X1;
					if (b.Y2 == b.Y1 + 1)
						b.Y2 = b.Y1;
					if (p->flags.bl_to_ur) {
						pcb_coord_t t;
						t = b.X1;
						b.X1 = b.X2;
						b.X2 = t;
					}
					/* using CreateDrawn instead of CreateNew concatenates sequential lines */
					p->parent.line = pcb_line_new_merge
						(layer, p->line.x1, p->line.y1, p->line.x2, p->line.y2,
						 p->style->Thick, p->style->Clearance * 2, pcb_flag_make(PCB_FLAG_AUTO | (conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)));

					if (p->parent.line) {
						pcb_undo_add_obj_to_create(PCB_OBJ_LINE, layer, p->parent.line, p->parent.line);
						changed = pcb_true;
					}
				}
				else if (p->type == VIA || p->type == VIA_SHADOW) {
					routebox_t *pp = (p->type == VIA_SHADOW) ? p->parent.via_shadow : p;
					pcb_coord_t radius = HALF_THICK(pp->style->Diameter);
					pcb_box_t b = shrink_routebox(p);
					total_via_count++;
					assert(pp->type == VIA);
					if (pp->parent.via == NULL) {
						assert(labs((b.X1 + radius) - (b.X2 - radius)) < 2);
						assert(labs((b.Y1 + radius) - (b.Y2 - radius)) < 2);
TODO("padstack: when style contains proto, remove this")
						pp->parent.via = pcb_pstk_new_compat_via(PCB->Data, -1,
							b.X1 + radius, b.Y1 + radius, 
							pp->style->Hole, pp->style->Diameter, 2 * pp->style->Clearance, 0, PCB_PSTK_COMPAT_ROUND, 1);
						assert(pp->parent.via);
						PCB_FLAG_SET(PCB_FLAG_AUTO, pp->parent.via);
						if (pp->parent.via) {
							pcb_undo_add_obj_to_create(PCB_OBJ_PSTK, pp->parent.via, pp->parent.via, pp->parent.via);
							changed = pcb_true;
						}
					}
					assert(pp->parent.via);
					if (p->type == VIA_SHADOW) {
						p->type = VIA;
						p->parent.via = pp->parent.via;
					}
				}
				else if (p->type == THERMAL)
					/* nothing to do because, the via might not be there yet */ ;
				else
					assert(0);
			}
		}
		PCB_END_LOOP;
		/* loop again to place all the thermals now that the vias are down */
		LIST_LOOP(net, same_net, p);
		{
			if (p->type == THERMAL) {
				pcb_pstk_t *pin = NULL;
				/* thermals are alread a single point search, no need to shrink */
				int type = FindPin(&p->box, &pin);
				if (pin) {
					pcb_undo_add_obj_to_clear_poly(type, pin->parent.data, pin, pin, pcb_false);
					pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_PSTK, LAYER_PTR(p->layer), pin);
					pcb_undo_add_obj_to_flag(pin);
					PCB_FLAG_THERM_ASSIGN(p->layer, autoroute_therm_style, pin);
					pcb_undo_add_obj_to_clear_poly(type, pin->parent.data, pin, pin, pcb_true);
					pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_PSTK, LAYER_PTR(p->layer), pin);
					changed = pcb_true;
				}
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
	return changed;
}

pcb_bool AutoRoute(pcb_bool selected)
{
	pcb_bool changed = pcb_false;
	routedata_t *rd;
	int i;

	total_wire_length = 0;
	total_via_count = 0;

#ifdef ROUTE_DEBUG
	ddraw = pcb_gui->request_debug_draw();
	if (ddraw != NULL) {
		ar_gc = ddraw->make_gc();
		ddraw->set_line_cap(ar_gc, pcb_cap_round);
	}
#endif

	for (i = 0; i < vtroutestyle_len(&PCB->RouteStyle); i++) {
		if (PCB->RouteStyle.array[i].Thick == 0 ||
				PCB->RouteStyle.array[i].Diameter == 0 || PCB->RouteStyle.array[i].Hole == 0 || PCB->RouteStyle.array[i].Clearance == 0) {
			pcb_message(PCB_MSG_ERROR, "You must define proper routing styles\n" "before auto-routing.\n");
			return pcb_false;
		}
	}
	if (ratlist_length(&PCB->Data->Rat) == 0)
		return pcb_false;
	rd = CreateRouteData();
	if (rd == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to initialize data; might be missing\n" "top or bottom copper layer.\n");
		return pcb_false;
	}

	if (1) {
		routebox_t *net, *rb, *last;
		int i = 0;
		/* count number of rats selected */
		PCB_RAT_LOOP(PCB->Data);
		{
			if (!selected || PCB_FLAG_TEST(PCB_FLAG_SELECTED, line))
				i++;
		}
		PCB_END_LOOP;
#ifdef ROUTE_VERBOSE
		printf("%d nets!\n", i);
#endif
		if (i == 0)
			goto donerouting;					/* nothing to do here */
		/* if only one rat selected, do things the quick way. =) */
		if (i == 1) {
			PCB_RAT_LOOP(PCB->Data);
			if (!selected || PCB_FLAG_TEST(PCB_FLAG_SELECTED, line)) {
				/* look up the end points of this rat line */
				routebox_t *a;
				routebox_t *b;
				a = FindRouteBoxOnLayerGroup(rd, line->Point1.X, line->Point1.Y, line->group1);
				b = FindRouteBoxOnLayerGroup(rd, line->Point2.X, line->Point2.Y, line->group2);
				assert(a != NULL && b != NULL);
				assert(a->style == b->style);
/*
	      if (a->type != PAD && b->type == PAD)
	        {
	          routebox_t *t = a;
		  a = b;
		  b = t;
	        }
*/
				/* route exactly one net, without allowing conflicts */
				InitAutoRouteParameters(0, a->style, pcb_false, pcb_true, pcb_true);
				/* hace planes work better as sources than targets */
				changed = RouteOne(rd, a, b, 150000).found_route || changed;
				goto donerouting;
			}
			PCB_END_LOOP;
		}
		/* otherwise, munge the netlists so that only the selected rats
		 * get connected. */
		/* first, separate all sub nets into separate nets */
		/* note that this code works because LIST_LOOP is clever enough not to
		 * be fooled when the list is changing out from under it. */
		last = NULL;
		LIST_LOOP(rd->first_net, different_net, net);
		{
			FOREACH_SUBNET(net, rb);
			{
				if (last) {
					last->different_net.next = rb;
					rb->different_net.prev = last;
				}
				last = rb;
			}
			END_FOREACH(net, rb);
			LIST_LOOP(net, same_net, rb);
			{
				rb->same_net = rb->same_subnet;
			}
			PCB_END_LOOP;
			/* at this point all nets are equal to their subnets */
		}
		PCB_END_LOOP;
		if (last) {
			last->different_net.next = rd->first_net;
			rd->first_net->different_net.prev = last;
		}

		/* now merge only those subnets connected by a rat line */
		PCB_RAT_LOOP(PCB->Data);
		if (!selected || PCB_FLAG_TEST(PCB_FLAG_SELECTED, line)) {
			/* look up the end points of this rat line */
			routebox_t *a;
			routebox_t *b;
			a = FindRouteBoxOnLayerGroup(rd, line->Point1.X, line->Point1.Y, line->group1);
			b = FindRouteBoxOnLayerGroup(rd, line->Point2.X, line->Point2.Y, line->group2);
			if (!a || !b) {
#ifdef DEBUG_STALE_RATS
				pcb_undo_add_obj_to_flag(line);
				PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, pcb_true, line);
				pcb_rat_invalidate_draw(line, 0);
#endif /* DEBUG_STALE_RATS */
				pcb_message(PCB_MSG_ERROR, "The rats nest is stale! Aborting autoroute...\n");
				goto donerouting;
			}
			/* merge subnets into a net! */
			MergeNets(a, b, NET);
		}
		PCB_END_LOOP;
		/* now 'different_net' may point to too many different nets.  Reset. */
		LIST_LOOP(rd->first_net, different_net, net);
		{
			if (!net->flags.touched) {
				LIST_LOOP(net, same_net, rb);
				rb->flags.touched = 1;
				PCB_END_LOOP;
			}
			else											/* this is not a "different net"! */
				RemoveFromNet(net, DIFFERENT_NET);
		}
		PCB_END_LOOP;
		/* reset "touched" flag */
		LIST_LOOP(rd->first_net, different_net, net);
		{
			LIST_LOOP(net, same_net, rb);
			{
				assert(rb->flags.touched);
				rb->flags.touched = 0;
			}
			PCB_END_LOOP;
		}
		PCB_END_LOOP;
	}
	/* okay, rd's idea of netlist now corresponds to what we want routed */
	/* auto-route all nets */
	changed = (RouteAll(rd).total_nets_routed > 0) || changed;
donerouting:
	pcb_hid_progress(0, 0, NULL);
	if (conf_core.editor.live_routing) {
		int i;
		pcb_box_t big = { 0, 0, PCB_MAX_COORD, PCB_MAX_COORD };
		for (i = 0; i < pcb_max_group(PCB); i++) {
			pcb_r_search(rd->layergrouptree[i], &big, NULL, ripout_livedraw_obj_cb, NULL, NULL);
		}
	}
#ifdef ROUTE_DEBUG
	if (ddraw != NULL)
		ddraw->finish_debug_draw();
#endif

	if (changed)
		changed = IronDownAllUnfixedPaths(rd);
	pcb_message(PCB_MSG_INFO, "Total added wire length = %$mS, %d vias added\n", (pcb_coord_t) total_wire_length, total_via_count);
	DestroyRouteData(&rd);
	if (changed) {
		pcb_undo_save_serial();

		/* optimize rats, we've changed connectivity a lot. */
		pcb_rats_destroy(pcb_false /*all rats */ );
		pcb_undo_restore_serial();
		pcb_rat_add_all(pcb_false /*all rats */);
		pcb_undo_restore_serial();

		pcb_undo_inc_serial();

		pcb_redraw();
	}
#if defined (ROUTE_DEBUG)
	aabort = 0;
#endif
	return changed;
}
