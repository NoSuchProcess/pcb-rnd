/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  altium pcbdoc ASCII plugin - high level read: convert tree to pcb-rnd
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"

#include <genht/htip.h>
#include <genht/htpi.h>
#include <genht/hash.h>
#include <genvector/vtd0.h>

#include <librnd/core/compat_misc.h>
#include <librnd/core/vtc0.h>
#include <librnd/core/rotate.h>
#include <librnd/poly/polygon1_gen.h>

#include "board.h"
#include "netlist.h"
#include "remove.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_compat_help/pstk_help.h"

#include "pcbdoc_ascii.h"
#include "pcbdoc_bin.h"
#include "htic.h"

#include "delay_postproc.c"

#define LY_CACHE_MAX 64

typedef struct postpone_hole_s postpone_hole_t;

struct postpone_hole_s {
	pcb_layer_t *ly;
	rnd_coord_t x1, y1, x2, y2;
	postpone_hole_t *next;
};

typedef struct {
	altium_tree_t tree;
	pcb_board_t *pcb;
	const char *filename;

	int has_bnd;  /* set to 3 after altium_parse_board() if board bbox picked up */

	/* caches */
	pcb_layer_t *lych[LY_CACHE_MAX];
	char lytoggle[128];
	int lytoggle_len;
	int ly_stack_new; /* 1 if we have the new layer stack model; the old one uses lytoggle */
	htip_t comps;
	htip_t nets;
	htic_t net_clr;
	htpi_t obj2netid;
	rnd_coord_t global_clr;
	pcb_layer_t *midly[64];
	postpone_hole_t *postholes;
	rnd_coord_t moved_x, moved_y; /* how much all objects got moved while normalizing the board */
	int bbvia_cache;
} rctx_t;

static rnd_coord_t altium_clearance(rctx_t *rctx, long netid)
{
	/* check if the net has its own */
	if (netid >= 0) {
		rnd_coord_t gap = htic_get(&rctx->net_clr, netid);
		if (gap > 0)
			return gap;
	}

	/* else fall back to global */
	return rctx->global_clr;
}

static rnd_coord_t conv_coord_field(altium_field_t *field)
{
	double res;
	rnd_bool succ;
	const char *s, *unit = NULL;

	switch(field->val_type) {
		case ALTIUM_FT_DBL: return RND_MIL_TO_COORD(field->val.dbl);
		case ALTIUM_FT_LNG: return RND_MIL_TO_COORD(field->val.lng);
		case ALTIUM_FT_CRD: return field->val.crd;
		case ALTIUM_FT_STR:
			/* look for unit (anything non-numeric) */
			for(s = field->val.str; *s != '\0' && !isalpha(*s); s++) ;
			if (*s == '\0')
				unit = "mil";

			res = rnd_get_value(field->val.str, unit, NULL, &succ);
			if (!succ) {
				rnd_message(RND_MSG_ERROR, "failed to convert coord value '%s'\n", field->val);
				return 0;
			}
			break;
	}
	return res;
}

static rnd_coord_t conv_coordx_field(rctx_t *rctx, altium_field_t *field)
{
	return conv_coord_field(field);
}

static rnd_coord_t conv_coordy_field(rctx_t *rctx, altium_field_t *field)
{
	return rctx->pcb->hidlib.dwg.Y2 - conv_coord_field(field);
}

static double conv_double_field(altium_field_t *field)
{
	char *end;
	double res;

	switch(field->val_type) {
		case ALTIUM_FT_DBL: return field->val.dbl;
		case ALTIUM_FT_LNG: return field->val.lng;
		case ALTIUM_FT_CRD: return field->val.crd;
		case ALTIUM_FT_STR:
			res = strtod(field->val.str, &end);
			if (*end != '\0') {
				rnd_message(RND_MSG_ERROR, "failed to convert floating point value '%s'\n", field->val);
				return 0;
			}
			return res;
	}
	abort();
}

static long conv_long_field(altium_field_t *field)
{
	char *end;
	long res;

	switch(field->val_type) {
		case ALTIUM_FT_DBL: return field->val.dbl;
		case ALTIUM_FT_LNG: return field->val.lng;
		case ALTIUM_FT_CRD: return field->val.crd;
		case ALTIUM_FT_STR:
			res = strtol(field->val.str, &end, 10);
			if (*end != '\0') {
				rnd_message(RND_MSG_ERROR, "failed to convert integer value '%s'\n", field->val.str);
				return 0;
			}
			return res;
	}
	abort();
}

static int conv_bool_field(altium_field_t *field)
{
	switch(field->val_type) {
		case ALTIUM_FT_LNG: return !!field->val.crd;
		case ALTIUM_FT_STR:
			if (rnd_strcasecmp(field->val.str, "true") == 0) return 1;
			if (rnd_strcasecmp(field->val.str, "false") == 0) return 0;
			rnd_message(RND_MSG_ERROR, "failed to convert bool value '%s'\n", field->val);
			return 0;
		default:;
			rnd_message(RND_MSG_ERROR, "failed to convert bool value from invalid type %d\n", field->val_type);
			return 0;
	}

	abort();
}

/* If te user routed field exists and is explicitly set to false, mark the
   object autorouted */
#define set_user_routed(obj, ur_field) \
do { \
	if ((obj != NULL) && (ur_field != NULL)) { \
		int __ur__ = conv_bool_field(ur_field); \
		if (!__ur__) \
			PCB_FLAG_SET(PCB_FLAG_AUTO, obj); \
	} \
} while(0)

static pcb_layer_t *conv_layer_(rctx_t *rctx, int cache_idx, pcb_layer_type_t lyt, const char *purpose)
{
	rnd_layer_id_t lid;
	pcb_layer_t *ly;

	if ((cache_idx >= 0) && (cache_idx < LY_CACHE_MAX) && (rctx->lych[cache_idx] != NULL))
		return rctx->lych[cache_idx];
	
	if (pcb_layer_listp(rctx->pcb, lyt, &lid, 1, -1, purpose) != 1)
		return NULL;

	ly = pcb_get_layer(rctx->pcb->Data, lid);
	if (ly == NULL)
		return NULL;

	if ((cache_idx >= 0) && (cache_idx < LY_CACHE_MAX))
		rctx->lych[cache_idx] = ly;

	return ly;
}

#define LYCH_ASSY_BOT 13
#define LYCH_ASSY_TOP 14

static pcb_layer_t *conv_layer_assy(rctx_t *rctx, int on_bottom)
{
	if (on_bottom)
		return conv_layer_(rctx, LYCH_ASSY_BOT, PCB_LYT_BOTTOM | PCB_LYT_DOC , "assy");
	return conv_layer_(rctx, LYCH_ASSY_TOP, PCB_LYT_TOP | PCB_LYT_DOC , "assy");
}

static pcb_layer_t *conv_layer_field_(rctx_t *rctx, altium_field_t *field, int deny_plane, int *is_plane)
{
	int kw;

	*is_plane = 0;

	if (field->val_type == ALTIUM_FT_LNG) {
		if ((field->val.lng >= 0) && (field->val.lng <= 82)) {
			switch(field->val.lng) {
				case 1:  return conv_layer_(rctx, 0,  PCB_LYT_COPPER | PCB_LYT_TOP, NULL);
				case 32: return conv_layer_(rctx, 1,  PCB_LYT_COPPER | PCB_LYT_BOTTOM, NULL);
				case 33: return conv_layer_(rctx, 2,  PCB_LYT_SILK | PCB_LYT_TOP, NULL);
				case 34: return conv_layer_(rctx, 3,  PCB_LYT_SILK | PCB_LYT_BOTTOM, NULL);
				case 35: return conv_layer_(rctx, 4,  PCB_LYT_PASTE | PCB_LYT_TOP, NULL);
				case 36: return conv_layer_(rctx, 5,  PCB_LYT_PASTE | PCB_LYT_BOTTOM, NULL);
				case 37: return conv_layer_(rctx, 6,  PCB_LYT_MASK | PCB_LYT_TOP, NULL);
				case 38: return conv_layer_(rctx, 7,  PCB_LYT_MASK | PCB_LYT_BOTTOM, NULL);
				case 55: return conv_layer_(rctx, 8,  PCB_LYT_DOC, "drill_guide");
				case 56: return conv_layer_(rctx, 9,  PCB_LYT_DOC, "altium.keepout");
				case 73: return conv_layer_(rctx, 10, PCB_LYT_DOC, "drill");
				case 74: return conv_layer_(rctx, 11, PCB_LYT_DOC, "multilayer");
				case 72: return conv_layer_(rctx, LYCH_ASSY_BOT, PCB_LYT_BOTTOM | PCB_LYT_DOC , "assy");
			}

			/* signal layer */
			if ((field->val.lng > 1) && (field->val.lng < 32))
				return rctx->midly[field->val.lng-2];

			/* mechanicals */
			if ((field->val.lng >= 57) && (field->val.lng < 72))
				return conv_layer_(rctx, 12, PCB_LYT_BOUNDARY, NULL);

			rnd_message(RND_MSG_ERROR, "Uknown binary layer ID: %ld\n", field->val.lng);
		}
		return NULL;
	}

	if (field->val_type != ALTIUM_FT_STR)
		return NULL;

	kw = altium_kw_sphash(field->val.str);
	switch(kw) {
		case altium_kw_layers_top:           return conv_layer_(rctx, 0,  PCB_LYT_COPPER | PCB_LYT_TOP, NULL);
		case altium_kw_layers_bottom:        return conv_layer_(rctx, 1,  PCB_LYT_COPPER | PCB_LYT_BOTTOM, NULL);
		case altium_kw_layers_topoverlay:    return conv_layer_(rctx, 2,  PCB_LYT_SILK | PCB_LYT_TOP, NULL);
		case altium_kw_layers_bottomoverlay: return conv_layer_(rctx, 3,  PCB_LYT_SILK | PCB_LYT_BOTTOM, NULL);
		case altium_kw_layers_toppaste:      return conv_layer_(rctx, 4,  PCB_LYT_PASTE | PCB_LYT_TOP, NULL);
		case altium_kw_layers_bottompaste:   return conv_layer_(rctx, 5,  PCB_LYT_PASTE | PCB_LYT_BOTTOM, NULL);
		case altium_kw_layers_topsolder:     return conv_layer_(rctx, 6,  PCB_LYT_MASK | PCB_LYT_TOP, NULL);
		case altium_kw_layers_bottomsolder:  return conv_layer_(rctx, 7,  PCB_LYT_MASK | PCB_LYT_BOTTOM, NULL);
		case altium_kw_layers_drillguide:    return conv_layer_(rctx, 8,  PCB_LYT_DOC, "drill_guide");
		case altium_kw_layers_keepout:       return conv_layer_(rctx, 9,  PCB_LYT_DOC, "altium.keepout");
		case altium_kw_layers_drilldrawing:  return conv_layer_(rctx, 10, PCB_LYT_DOC, "drill");
		case altium_kw_layers_multilayer:    return conv_layer_(rctx, 11, PCB_LYT_DOC, "multilayer");
	}

	if ((rnd_strcasecmp(field->val.str, "MECHANICAL1") == 0))
		return conv_layer_(rctx, 12, PCB_LYT_BOUNDARY, NULL);
	if ((rnd_strcasecmp(field->val.str, "MECHANICAL15") == 0))
		return conv_layer_(rctx, LYCH_ASSY_BOT, PCB_LYT_BOTTOM | PCB_LYT_DOC , "assy");

	if (rnd_strncasecmp(field->val.str, "MID", 3) == 0) { /* mid 1..30: intern copper signal */
		char *end;
		int idx = strtol(field->val.str+3, &end, 10);
		if (*end != '\0') {
			rnd_message(RND_MSG_ERROR, "Layer not found: '%s' - invalid integer for MID layer\n", field->val.str);
			return NULL;
		}
		return rctx->midly[idx-1];
	}

	if (rnd_strncasecmp(field->val.str, "PLANE", 5) == 0) {
		char *end;
		int idx;

		*is_plane = 1;

		if (deny_plane) {
			rnd_message(RND_MSG_ERROR, "Drawing on PLANE layer %s is not supported\n", field->val.str);
			return NULL;
		}

		idx = strtol(field->val.str+5, &end, 10);
		if (*end != '\0') {
			rnd_message(RND_MSG_ERROR, "Layer not found: '%s' - invalid integer for MID layer\n", field->val.str);
			return NULL;
		}
		return rctx->midly[idx-1+16+21];
	}
TODO("MECHANICAL2...MECHANICAL14: look up or create new doc?; use cache index from 15+16+16+mechanical");
	rnd_message(RND_MSG_ERROR, "Layer not found: '%s'\n", field->val.str);
	return NULL;
}

static pcb_layer_t *conv_layer_field(rctx_t *rctx, altium_field_t *field)
{
	int dummy;
	return conv_layer_field_(rctx, field, 1, &dummy);
}

static pcb_layer_t *bin_layer(rctx_t *rctx, long lyt, long offs)
{
	rnd_layer_id_t lid, lids[PCB_MAX_LAYER];

	if ((lyt & PCB_LYT_COPPER) && (lyt & PCB_LYT_INTERN)) {
		int len;

		len = pcb_layer_list(rctx->pcb, lyt, lids, PCB_MAX_LAYER);
		offs = offs - 1;
		if ((offs < 0) || (offs >= len))
			return NULL;
		lid = lids[offs];
	}
	else {
		if (pcb_layer_list(rctx->pcb, lyt, &lid, 1) != 1)
			return NULL;
	}

	return &rctx->pcb->Data->Layer[lid];
}

#define BUMP_COORD(dst, src) do { if (src > dst) dst = src; } while(0)

typedef struct {
	const char *name;
	int prev, next, seen;
} altium_layer_t;

static void make_int_cop(rctx_t *rctx, altium_layer_t *layers, int n)
{
	pcb_layergrp_t *g = pcb_get_grp_new_intern(rctx->pcb, n-2);
	const char *lyname = (layers == NULL) ? "anon" : layers[n].name;
	rnd_layer_id_t lid = pcb_layer_create(rctx->pcb, g - rctx->pcb->LayerGroups.grp, lyname, 0);

	rctx->midly[n-2] = pcb_get_layer(rctx->pcb->Data, lid);
	if ((n >= 39) && (n <= 54)) {
		/* internal plane layer - draw a poly at the end */
		pcb_attribute_put(&(rctx->midly[n-2]->Attributes), "altium::plane", "1");
	}

}

static int altium_layer_list(rctx_t *rctx, altium_layer_t *layers, int layers_max, int first)
{
	int timeout, cop = 0, n = first;

printf(" list:\n");
	for(timeout = 0; timeout < layers_max; timeout++) {
		if ((n <= 0) || (n >= layers_max) || layers[n].seen)
			break;

printf("  [%d] %s (idx=%d)\n", n, layers[n].name, n);

		/* The format uses hardwired layer indices; layer names are unreliable and there is no layer type */
		if ((n >= 1) && (n <= 54)) { /* plain copper signal layers */
			cop = 1;
			if ((n == 1) || (n == 32)) {
				/* do not do anything - top and bottom are already created */
			}
			else {
				/* internal copper layers for signals */
				make_int_cop(rctx, layers, n);
			}
		}

		layers[n].seen = 1;
		n = layers[n].next;
	}
	return cop;
}

static void poly_hole_from_rect(rctx_t *rctx, postpone_hole_t *ph)
{
	pcb_poly_t *poly = polylist_first(&ph->ly->Polygon), *npoly;
	rnd_polyarea_t *original, *new_hole, *result;

	if (poly == NULL) {
		rnd_message(RND_MSG_ERROR, "Internal error: can't find plane polygon for placing the fill within\n");
		return;
	}


	/* need to move the hole to compensate for board normalizaiton done earlier in postproc */
	ph->x1 += rctx->moved_x;
	ph->x2 += rctx->moved_x;
	ph->y1 += rctx->moved_y;
	ph->y2 += rctx->moved_y;


	original = pcb_poly_from_poly(poly);
	new_hole = rnd_poly_from_rect(ph->x1, ph->x2, ph->y1, ph->y2);

	rnd_polyarea_boolean_free(original, new_hole, &result, RND_PBO_SUB);

	npoly = pcb_poly_to_polygons_on_layer(PCB->Data, ph->ly, result, poly->Flags);
	memcpy(&npoly->Attributes, &poly->Attributes, sizeof(poly->Attributes));
	memset(&poly->Attributes, 0, sizeof(poly->Attributes));
	pcb_remove_object(PCB_OBJ_POLY, ph->ly, poly, poly);
}

/* Apply postponed poly holes */
static void altium_finalize_plane_holes(rctx_t *rctx)
{
	postpone_hole_t *ph, *next;
	for(ph = rctx->postholes; ph != NULL; ph = next) {
		next = ph->next;
		poly_hole_from_rect(rctx, ph);
		free(ph);
	}
}

/* Apply layer stack side effects */
static void altium_finalize_layers(rctx_t *rctx)
{
	int n;
	pcb_poly_t *plane[16];
	altium_record_t *rec;
	altium_field_t *field;


	/* create "plane" polygons */
	for(n = 37; n <= 52; n++) {
		if (rctx->midly[n] != NULL) {
			pcb_poly_t *poly = pcb_poly_new(rctx->midly[n], 0, pcb_flag_make(PCB_FLAG_CLEARPOLY));
			pcb_poly_point_new(poly, rctx->pcb->hidlib.dwg.X1, rctx->pcb->hidlib.dwg.Y1);
			pcb_poly_point_new(poly, rctx->pcb->hidlib.dwg.X2, rctx->pcb->hidlib.dwg.Y1);
			pcb_poly_point_new(poly, rctx->pcb->hidlib.dwg.X2, rctx->pcb->hidlib.dwg.Y2);
			pcb_poly_point_new(poly, rctx->pcb->hidlib.dwg.X1, rctx->pcb->hidlib.dwg.Y2);
			pcb_add_poly_on_layer(rctx->midly[n], poly);
			plane[n-37] = poly;
			pcb_attribute_put(&(poly->Attributes), "altium::plane", "yes");
		}
	}

	/* pick up plane net associativity */
	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_board]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_board], rec)) {
		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			assert(field->val_type == ALTIUM_FT_STR);
			if ((rnd_strncasecmp(field->key, "plane", 5) == 0) && (*field->val.str != '(')) {
				char *end;
				long idx = strtol(field->key+5, &end, 10), midx = idx-2+37+1;
				if ((rnd_strcasecmp(end, "netname") == 0) && (rctx->midly[midx] != NULL)) {
					pcb_attribute_put(&(rctx->midly[midx]->Attributes), "altium::net", field->val.str);
					TODO("make thermals on pcb_poly_t plane[idx-1] vs. any via on the same net");
				}
			}
		}
	}

	altium_finalize_plane_holes(rctx);
}

static int altium_parse_board(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;
	altium_layer_t layers[128] = {0};
	int seen_cop = 0, n, layers_max = (sizeof(layers)/sizeof(layers[0]));

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_board]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_board], rec)) {
		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_sheetheight: rctx->pcb->hidlib.dwg.X2 = conv_coord_field(field); break;
				case altium_kw_field_sheetwidth:  rctx->pcb->hidlib.dwg.Y2 = conv_coord_field(field); break;
				case altium_kw_field_togglelayers:
					{
						int len;
						assert(field->val_type == ALTIUM_FT_STR);
						len = strlen(field->val.str);
						if (len > sizeof(rctx->lytoggle)-1)
							len = sizeof(rctx->lytoggle)-1;
						memcpy(rctx->lytoggle, field->val.str, len);
						rctx->lytoggle[len] = '\0';
						rctx->lytoggle_len = len;
					}
					break;
				default:
					/* vx[0-4] and vy[0-4] */
					if ((tolower(field->key[0]) == 'v') && isdigit(field->key[2]) && (field->key[3] == 0)) {
						if (tolower(field->key[1]) == 'x') {
							BUMP_COORD(rctx->pcb->hidlib.dwg.X2, conv_coord_field(field));
							rctx->has_bnd |= 1;
						}
						if (tolower(field->key[1]) == 'y') {
							BUMP_COORD(rctx->pcb->hidlib.dwg.Y2, conv_coord_field(field));
							rctx->has_bnd |= 2;
						}
					}
					if (rnd_strncasecmp(field->key, "layer", 5) == 0) {
						char *end;
						long idx = strtol(field->key+5, &end, 10);
						int fid;

						if ((idx < 0) || (idx > layers_max))
							break;
						rctx->ly_stack_new = 1;
						fid = altium_kw_sphash(end);
						assert(field->val_type == ALTIUM_FT_STR);
						switch(fid) {
							case altium_kw_field_name: layers[idx].name = field->val.str; break;
							case altium_kw_field_prev: layers[idx].prev = atoi(field->val.str); break;
							case altium_kw_field_next: layers[idx].next = atoi(field->val.str); break;
						}
					}
				break;
			}
		}
	}

	if (rctx->has_bnd != 3)
		rctx->pcb->hidlib.dwg.X2 = rctx->pcb->hidlib.dwg.Y2 = 0;

	/*** create the layer stack (copper only) ***/

	if (!rctx->ly_stack_new) {
		int n;
		rnd_message(RND_MSG_WARNING, "Old pre-protel99 layer stack header detected;\nthe layer stack loaded may be broken\n");

		/* internal planes start at 22 */
		for(n = 22; (n < rctx->lytoggle_len) && (n < 27); n++) {
			if (rctx->lytoggle[n] == '1')
				make_int_cop(rctx, NULL, n-22+39);
		}
	}

printf("Layer stack:\n");

	/* figure the first (top) layer: prev == 0, next != 0 */
	for(n = 1; n < layers_max; n++)
		if ((layers[n].prev == 0) && (layers[n].next != 0))
			seen_cop |= altium_layer_list(rctx, layers, layers_max, n);

	if (!seen_cop && (rctx->lytoggle_len == 0))
		rnd_message(RND_MSG_ERROR, "Broken layer stack: no copper layers (falling back to stock 2 layer board)\n");

	return 0;
}

static int altium_parse_net(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;
	long auto_id = 0;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_net]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_net], rec), auto_id++) {
		altium_field_t *name = NULL;
		pcb_net_t *net;
		long id = -1;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_name: name = field; break;
				case altium_kw_field_id:   id = conv_long_field(field); break;
				default: break;
			}
		}
		if (id < 0)
			id = auto_id;
		if (name == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid net object: missing name (net not created)\n");
			continue;
		}

		assert(name->val_type == ALTIUM_FT_STR);
		net = pcb_net_get(rctx->pcb, &rctx->pcb->netlist[PCB_NETLIST_INPUT], name->val.str, PCB_NETA_ALLOC);
		htip_set(&rctx->nets, id, net);
	}

	return 0;
}

static int altium_parse_class(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_class]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_class], rec)) {
		const char *name = NULL;
		pcb_net_t *net;
		long id = -1;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_name: assert(field->val_type == ALTIUM_FT_STR); name = field->val.str; break;
				default: break;
			}
		}
		if (name == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid class: missing name\n");
			continue;
		}

		/* look up each net referenced from M[0-9]* fields and set class */
		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			if ((field->val_type == ALTIUM_FT_STR) && (tolower(field->key[0]) == 'm') && isdigit(field->key[1])) {
				pcb_net_t *net = pcb_net_get(rctx->pcb, &rctx->pcb->netlist[PCB_NETLIST_INPUT], field->val.str, 0);
				if (net == NULL) {
/*					rnd_message(RND_MSG_ERROR, "Class %s references non-existing net %s\n", name, field->val.str);*/
					continue;
				}
				pcb_attribute_put(&net->Attributes, "class", name);
			}
		}
	}

	return 0;
}

static int altium_parse_rule(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_rule]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_rule], rec)) {
		altium_field_t *scope_val[2] = {NULL, NULL}, *scval;
		const char *name = "";
		int kind = -1, netscope = -1, layerkind = -1, scope_kind[2] = {-1, -1}, sckind, enabled = 1;
		rnd_coord_t gap = RND_COORD_MAX;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			assert(field->val_type == ALTIUM_FT_STR);
			switch(field->type) {
				case altium_kw_field_name:           name = field->val.str; break;
				case altium_kw_field_rulekind:       kind = altium_kw_sphash(field->val.str); break;
				case altium_kw_field_netscope:       netscope = altium_kw_sphash(field->val.str); break;
				case altium_kw_field_layerkind:      layerkind = altium_kw_sphash(field->val.str); break;
				case altium_kw_field_scope1_0_kind:  assert(field->val_type == ALTIUM_FT_STR); scope_kind[0] = altium_kw_sphash(field->val.str); break;
				case altium_kw_field_scope2_0_kind:  assert(field->val_type == ALTIUM_FT_STR); scope_kind[1] = altium_kw_sphash(field->val.str); break;
				case altium_kw_field_scope1expression:  assert(field->val_type == ALTIUM_FT_STR); scope_kind[0] = altium_kw_sphash(field->val.str); break;
				case altium_kw_field_scope2expression:  assert(field->val_type == ALTIUM_FT_STR); scope_kind[1] = altium_kw_sphash(field->val.str); break;
				case altium_kw_field_scope1_0_value: scope_val[0] = field; break;
				case altium_kw_field_scope2_0_value: scope_val[1] = field; break;
				case altium_kw_field_gap:            gap = conv_coord_field(field); break;
				case altium_kw_field_enabled:        enabled = conv_bool_field(field); break;
				default: break;
			}
		}

		/* deal with samke layer net clearances only */
		if (!enabled) continue;
		if (kind != altium_kw_field_clearance) continue;
		if ((netscope != altium_kw_field_differentnets) && (netscope != altium_kw_field_anynet)) continue;
		if (layerkind != altium_kw_field_samelayer) continue;

		/* one of the scopes must be board */
		if (scope_kind[0] == altium_kw_field_board) {
			sckind = scope_kind[1];
			scval = scope_val[1];
		}
		else if (scope_kind[1] == altium_kw_field_board) {
			sckind = scope_kind[0];
			scval = scope_val[0];
		}
		else if (scope_kind[0] == altium_kw_field_all) {
			sckind = scope_kind[1];
			scval = scope_val[1];
		}
		else if (scope_kind[1] == altium_kw_field_all) {
			sckind = scope_kind[1];
			scval = scope_val[1];
		}
		else
			continue;

		if (scval != NULL) {
			assert(scval->val_type == ALTIUM_FT_STR);
		}

		/* one scope is board, the other is sckind:scval */
		if (gap == RND_COORD_MAX) {
			rnd_message(RND_MSG_ERROR, "Invalid clearance rule %s: no gap defined (ignoring rule)\n", name);
			continue;
		}

/*		rnd_printf("RULE %s sctype=%s scval=%s (%s %s) gap=%mm\n", name,
			((sckind == altium_kw_field_net) ? "net" : ((sckind == altium_kw_field_board) ? "board" : "class")),
			scval->val.str, scope_val[0]->val.str, scope_val[1]->val.str, gap);
*/

		switch(sckind) {
			case altium_kw_field_board:
			case altium_kw_field_all:
				rctx->global_clr = gap;
				break;
			case altium_kw_field_netclass:
				if (scval != NULL) {
					htip_entry_t *e;
					for(e = htip_first(&rctx->nets); e != NULL; e = htip_next(&rctx->nets, e)) {
						pcb_net_t *net = e->value;
						const char *nclass = pcb_attribute_get(&net->Attributes, "class");
						if ((nclass != NULL) && (strcmp(nclass, scval->val.str) == 0)) {
							long netid = e->key;
							htic_set(&rctx->net_clr, netid, gap);
						}
					}
				}
				break;
			case altium_kw_field_net:
				break;
		}
	}

	return 0;
}

static int altium_parse_components(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;
	long auto_id = 0;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_component]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_component], rec), auto_id++) {
		pcb_subc_t *sc;
		altium_field_t *ly = NULL, *refdes = NULL, *footprint = NULL;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX;
		double rot = 0;
		int on_bottom;
		long id = -1;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_layer:                ly = field; break;
				case altium_kw_field_x:                    x = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y:                    y = conv_coordy_field(rctx, field); break;
				case altium_kw_field_rotation:             rot = conv_double_field(field); break;
				case altium_kw_field_id:                   id = conv_long_field(field); break;
				case altium_kw_field_sourcedesignator:     refdes = field; break;
				case altium_kw_field_footprintdescription: footprint = field; break;
				default: break;
			}
		}
		if ((x == RND_COORD_MAX) || (y == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid component object: missing coordinate (component not created)\n");
			continue;
		}
		if (id < 0)
			id = auto_id;
		if (ly == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid component object: missing layer (component not created)\n");
			continue;
		}
		assert(ly->val_type == ALTIUM_FT_STR);
		if (rnd_strcasecmp(ly->val.str, "bottom") == 0) on_bottom = 1;
		else if (rnd_strcasecmp(ly->val.str, "top") == 0) on_bottom = 0;
		else {
			rnd_message(RND_MSG_ERROR, "Invalid component object: invalid layer '%s' (should be top or bottom; component not created)\n", ly->val.str);
			continue;
		}

		sc = pcb_subc_alloc();
		pcb_subc_create_aux(sc, x, y, rot, on_bottom);
		if (refdes != NULL) {
			assert(refdes->val_type == ALTIUM_FT_STR);
			pcb_attribute_put(&sc->Attributes, "refdes", refdes->val.str);
		}
		if (footprint != NULL) {
			assert(footprint->val_type == ALTIUM_FT_STR);
			pcb_attribute_put(&sc->Attributes, "footprint", footprint->val.str);
		}
		pcb_subc_reg(rctx->pcb->Data, sc);
		pcb_subc_bind_globals(rctx->pcb, sc);
		htip_set(&rctx->comps, id, sc);
	}

	return 0;
}

/* Decode expansion mode from text string or binary values and return a
   field keyword (or -1) */
static int get_expansion_mode(altium_field_t *field)
{
	switch(field->val_type) {
		case ALTIUM_FT_STR: return altium_kw_sphash(field->val.str);
		case ALTIUM_FT_LNG:
			switch(field->val.lng) {
				case 1: return altium_kw_field_rule;
				case 2: return altium_kw_field_manual;
				default:
					rnd_message(RND_MSG_ERROR, "internal error: io_altium get_expansion_mode(): unknown binary mode %d\n", field->val.lng);
			}
			break;
		default:
			rnd_message(RND_MSG_ERROR, "internal error: io_altium get_expansion_mode(): invalid field type %d\n", field->val_type);
	}
	return -1;
}

typedef enum {
	ALTIUM_SHAPE_RECTANGLE,
	ALTIUM_SHAPE_ROUND_RECTANGLE,
	ALTIUM_SHAPE_ROUND
} altium_pad_shape_t;

/* Return shape type of a field (text name from ASCII, long from bin) */
static altium_pad_shape_t get_shape(altium_field_t *field)
{
	switch(field->val_type) {
		case ALTIUM_FT_STR:
			if (rnd_strcasecmp(field->val.str, "rectangle") == 0) return ALTIUM_SHAPE_RECTANGLE;
			if (rnd_strcasecmp(field->val.str, "roundedrectangle") == 0) return ALTIUM_SHAPE_ROUND_RECTANGLE;
			if (rnd_strcasecmp(field->val.str, "round") == 0) return ALTIUM_SHAPE_ROUND;
			rnd_message(RND_MSG_ERROR, "io_altium get_shape(): invalid shape name '%s'\n", field->val.str);
			break;
		case ALTIUM_FT_LNG:
			switch(field->val.lng) {
				case 1: return ALTIUM_SHAPE_ROUND;
				case 2: return ALTIUM_SHAPE_RECTANGLE;
/*				case 3: return ALTIUM_SHAPE_OVAL?;*/
				case 9: return ALTIUM_SHAPE_ROUND_RECTANGLE;
				default:
					rnd_message(RND_MSG_ERROR, "io_altium get_shape(): invalid shape ID %ld\n", field->val.lng);
			}
			break;
		default:
			rnd_message(RND_MSG_ERROR, "internal error: io_altium get_shape(): invalid field type %d\n", field->val_type);
	}
	return -1;
}

/* Generate copper, mask and paste shapes for a given geometry */
static int gen_pad_shape(pcb_pstk_shape_t *copper_shape, int *copper_valid, pcb_pstk_shape_t *mask_shape, int *mask_valid, pcb_pstk_shape_t *paste_shape, int *paste_valid, altium_field_t *shapename, rnd_coord_t xsize, rnd_coord_t ysize, rnd_coord_t mask_fin, rnd_coord_t paste_fin)
{
	*mask_valid = 0;
	*mask_valid = 0;
	*paste_valid = 0;

	switch(get_shape(shapename)) {
		case ALTIUM_SHAPE_RECTANGLE:
		case ALTIUM_SHAPE_ROUND_RECTANGLE:
			pcb_shape_rect(copper_shape, xsize, ysize);
			*copper_valid = 1;
			if (((xsize + mask_fin*2) > 0) && ((ysize + mask_fin*2) > 0)) {
				pcb_shape_rect(mask_shape, xsize + mask_fin*2, ysize + mask_fin*2);
				*mask_valid = 1;
			}
			if (((xsize + paste_fin*2) > 0) && ((ysize + paste_fin*2) > 0)) {
				pcb_shape_rect(paste_shape, xsize + paste_fin*2, ysize + paste_fin*2);
				*paste_valid = 1;
			}
			break;
		case ALTIUM_SHAPE_ROUND:
			if (xsize == ysize) {
				copper_shape->shape = mask_shape->shape = paste_shape->shape = PCB_PSSH_CIRC;
				copper_shape->data.circ.x = copper_shape->data.circ.y = 0;
				copper_shape->data.circ.dia = xsize;
				*copper_valid = 1;
				mask_shape->data.circ.x = mask_shape->data.circ.y = 0;
				mask_shape->data.circ.dia = xsize + mask_fin*2;
				*mask_valid = (mask_shape->data.circ.dia > 0);
				paste_shape->data.circ.x = paste_shape->data.circ.y = 0;
TODO("check if mask is needed for paste:");
/*
				paste_shape->data.circ.dia = xsize + mask_fin*2;
				paste_shape->data.circ.dia += paste_fin*2;
*/
				paste_shape->data.circ.dia = xsize + paste_fin*2;
				*paste_valid = (paste_shape->data.circ.dia > 0);
			}
			else {
				copper_shape->shape = mask_shape->shape = paste_shape->shape = PCB_PSSH_LINE;
				if (xsize > ysize) {
					copper_shape->data.line.x1 = mask_shape->data.line.x1 = paste_shape->data.line.x1 = -xsize/2 + ysize/2;
					copper_shape->data.line.x2 = mask_shape->data.line.x2 = paste_shape->data.line.x2 = +xsize/2 - ysize/2;
					copper_shape->data.line.y1 = mask_shape->data.line.y1 = paste_shape->data.line.y1 = copper_shape->data.line.y2 = mask_shape->data.line.y2 = paste_shape->data.line.y2 = 0;
					copper_shape->data.line.thickness = ysize;
					*copper_valid = 1;
					mask_shape->data.line.thickness = ysize + mask_fin*2;
					*mask_valid = (mask_shape->data.line.thickness > 0);
					paste_shape->data.line.thickness = ysize + paste_fin*2;
					*paste_valid = (paste_shape->data.line.thickness > 0);
				}
				else {
					copper_shape->data.line.y1 = mask_shape->data.line.y1 = paste_shape->data.line.y1 = -ysize/2 + xsize/2;
					copper_shape->data.line.y2 = mask_shape->data.line.y2 = paste_shape->data.line.y2 = +ysize/2 - xsize/2;
					copper_shape->data.line.x1 = mask_shape->data.line.x1 = paste_shape->data.line.x1 = copper_shape->data.line.x2 = mask_shape->data.line.x2 = paste_shape->data.line.x2 = 0;
					copper_shape->data.line.thickness = xsize;
					*copper_valid = 1;
					mask_shape->data.line.thickness = xsize + mask_fin*2;
					*mask_valid = (mask_shape->data.line.thickness > 0);;
					paste_shape->data.line.thickness = xsize + paste_fin*2;
					*paste_valid = (paste_shape->data.line.thickness > 0);
				}
			}
			break;
		default:
			rnd_message(RND_MSG_ERROR, "Invalid pad object: invalid shape (pad not created)\n");
			return -1;
	}
	return 0;
}

/* copy copper/mask/paste shapes from level 0 (top) to level dst; this is done
   for multi-level padstacks where only top shape is specified (common in ASCII) */
static void copy_pad_shape0(int dst, pcb_pstk_shape_t *copper_shape, int *copper_valid, pcb_pstk_shape_t *mask_shape, int *mask_valid, pcb_pstk_shape_t *paste_shape, int *paste_valid)
{
	if (copper_valid[0]) {
		pcb_pstk_shape_copy(copper_shape+dst, copper_shape+0);
		copper_valid[dst] = 1;
	}
	else
		copper_valid[dst] = 0;

	if (mask_valid[0]) {
		pcb_pstk_shape_copy(mask_shape+dst, mask_shape+0);
		mask_valid[dst] = 1;
	}
	else
		mask_valid[dst] = 0;

	if (paste_valid[0]) {
		pcb_pstk_shape_copy(paste_shape+dst, paste_shape+0);
		paste_valid[dst] = 1;
	}
	else
		paste_valid[dst] = 0;
}

/* Returns whether level (0..2) has a valid shape definition */
static int has_shape_for(int level, altium_field_t **shapename, rnd_coord_t *xsize, rnd_coord_t *ysize)
{
	if ((xsize[level] == RND_COORD_MAX) || (ysize[level] == RND_COORD_MAX) || (shapename[level] == NULL))
		return 0;
	return 1;
}

static int altium_parse_pad(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_pad]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_pad], rec)) {
		pcb_pstk_t *ps;
		pcb_subc_t *sc = NULL;
		altium_field_t *ly = NULL, *term = NULL, *ur = NULL, *startly = NULL, *endly = NULL;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, hole = 0;
		altium_field_t *shapename[3] = {NULL, NULL, NULL}; /* top, mid, bottom */
		rnd_coord_t xsize[3] = {RND_COORD_MAX, RND_COORD_MAX, RND_COORD_MAX}, ysize[3] = {RND_COORD_MAX, RND_COORD_MAX, RND_COORD_MAX}; /* top, mid, bottom */
		rnd_coord_t mask_man = RND_COORD_MAX, paste_man = RND_COORD_MAX, mask_fin = PCB_PROTO_MASK_BLOAT, paste_fin = 0;
		pcb_pstk_shape_t shape[9] = {0}, copper_shape[3] = {0}, mask_shape[3] = {0}, paste_shape[3] = {0};
		long compid = -1, netid = -1;
		int on_all = 0, on_bottom = 0, plated = 0, mask_mode = -1, paste_mode = -1, copper_valid[3], mask_valid[3], paste_valid[3], n;
		double rot = 0, holerot = 0;
		rnd_coord_t cl, holewidth = -1;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_userrouted:ur = field; break;
				case altium_kw_field_name:      term = field; break;
				case altium_kw_field_layer:     ly = field; break;
				case altium_kw_field_startlayer:startly = field; break;
				case altium_kw_field_endlayer:  endly = field; break;
				case altium_kw_field_x:         x = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y:         y = conv_coordy_field(rctx, field); break;
				case altium_kw_field_holesize:  hole = conv_coord_field(field); break;
				case altium_kw_field_rotation:  rot = conv_double_field(field); break;
				case altium_kw_field_component: compid = conv_long_field(field); break;
				case altium_kw_field_net:       netid = conv_long_field(field); break;
				case altium_kw_field_plated:    plated = conv_bool_field(field); break;

				case altium_kw_field_shape:              shapename[0] = field; break;
				case altium_kw_field_xsize:              xsize[0] = conv_coord_field(field); break;
				case altium_kw_field_ysize:              ysize[0] = conv_coord_field(field); break;
				case altium_kw_field__bin_mid_shape:     shapename[1] = field; break;
				case altium_kw_field__bin_mid_xsize:     xsize[1] = conv_coord_field(field); break;
				case altium_kw_field__bin_mid_ysize:     ysize[1] = conv_coord_field(field); break;
				case altium_kw_field__bin_bottom_shape:  shapename[2] = field; break;
				case altium_kw_field__bin_bottom_xsize:  xsize[2] = conv_coord_field(field); break;
				case altium_kw_field__bin_bottom_ysize:  ysize[2] = conv_coord_field(field); break;

				case altium_kw_field_pastemaskexpansionmode:     paste_mode = get_expansion_mode(field); break;
				case altium_kw_field_soldermaskexpansionmode:    mask_mode = get_expansion_mode(field); break;
				case altium_kw_field_pastemaskexpansion_manual:  paste_man = conv_coord_field(field); break;
				case altium_kw_field_soldermaskexpansion_manual: mask_man = conv_coord_field(field); break;

				case altium_kw_field_holewidth:          holewidth = conv_coord_field(field); break;
				case altium_kw_field_holerotation:       holerot = conv_double_field(field); break;
TODO("HOLETYPE: 2 for slot?");
				default: break;
			}
		}
		if ((x == RND_COORD_MAX) || (y == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid pad object: missing coordinate (pad not created)\n");
			continue;
		}

		if (!has_shape_for(0, shapename, xsize, ysize)) {
			rnd_message(RND_MSG_ERROR, "Invalid pad object: missing top shape or size (pad not created)\n");
			continue;
		}

		if (compid >= 0) {
			sc = htip_get(&rctx->comps, compid);
			if (sc == NULL) {
				if (term != NULL)
					assert(term->val_type == ALTIUM_FT_STR);
				rnd_message(RND_MSG_ERROR, "Invalid pad object: can't find parent component (%ld) on term %s (pad not created)\n", compid, (term == NULL ? "<unspecified>" : term->val.str));
				continue;
			}
		}

		if (ly->val_type == ALTIUM_FT_STR) {
			if (rnd_strcasecmp(ly->val.str, "bottom") == 0) on_bottom = 1;
			else if (rnd_strcasecmp(ly->val.str, "top") == 0) on_bottom = 0;
			else if (rnd_strcasecmp(ly->val.str, "multilayer") == 0) on_all = 1;
			else {
				rnd_message(RND_MSG_ERROR, "Invalid object: invalid pad layer '%s' (should be top or bottom or multilayer; pad not created)\n", ly->val.str);
				continue;
			}
		}
		else if (ly->val_type == ALTIUM_FT_LNG) {
			switch(ly->val.lng) {
				case 1:  on_bottom = 0; break;
				case 32: on_bottom = 1; break;
				case 74: on_all = 1; break;
				default:
					rnd_message(RND_MSG_ERROR, "Invalid object: invalid pad layer %ld (should be 1=top or 32=bottom or 74=multilayer; pad not created)\n", ly->val.lng);
					continue;
			}
		}

		/* set final mask and paste offsets */
		if (mask_mode == altium_kw_field_manual) {
			if (mask_man != RND_COORD_MAX)
				mask_fin = mask_man;
			else
				rnd_message(RND_MSG_ERROR, "Invalid pad mask manual: no value (using default mask offset)\n");
		}
		if (paste_mode == altium_kw_field_manual) {
			if (paste_man != RND_COORD_MAX)
				paste_fin = paste_man;
			else
				rnd_message(RND_MSG_ERROR, "Invalid pad paste manual: no value (using default paste offset)\n");
		}

		/* create the abstract shapes */
		if (gen_pad_shape(&copper_shape[0], &copper_valid[0], &mask_shape[0], &mask_valid[0], &paste_shape[0], &paste_valid[0], shapename[0], xsize[0], ysize[0], mask_fin, paste_fin) != 0)
			continue;

		/* create shape stackup in shape[] */
		n = 0;
		if (on_all) {
			/* figure mid and bottom shapes */
			if (has_shape_for(1, shapename, xsize, ysize))
				gen_pad_shape(&copper_shape[1], &copper_valid[1], &mask_shape[1], &mask_valid[1], &paste_shape[1], &paste_valid[1], shapename[1], xsize[1], ysize[1], mask_fin, paste_fin);
			else
				copy_pad_shape0(1, copper_shape, copper_valid, mask_shape, mask_valid, paste_shape, paste_valid);
			if (has_shape_for(2, shapename, xsize, ysize))
				gen_pad_shape(&copper_shape[2], &copper_valid[2], &mask_shape[2], &mask_valid[2], &paste_shape[2], &paste_valid[2], shapename[2], xsize[2], ysize[2], mask_fin, paste_fin);
			else
				copy_pad_shape0(2, copper_shape, copper_valid, mask_shape, mask_valid, paste_shape, paste_valid);

			if (copper_valid[0]) {
				pcb_pstk_shape_copy(&shape[n], &copper_shape[0]); shape[n].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER; n++;
			}

			if (copper_valid[1]) {
				pcb_pstk_shape_copy(&shape[n], &copper_shape[1]); shape[n].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; n++;
			}

			if (copper_valid[2]) {
				pcb_pstk_shape_copy(&shape[n], &copper_shape[2]); shape[n].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; n++;
			}

			if (mask_valid[0]) {
				pcb_pstk_shape_copy(&shape[n], &mask_shape[0]);   shape[n].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK; shape[n].comb = PCB_LYC_AUTO | PCB_LYC_SUB; n++;
			}

			if (mask_valid[2]) {
				pcb_pstk_shape_copy(&shape[n], &mask_shape[2]);   shape[n].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK; shape[n].comb = PCB_LYC_AUTO | PCB_LYC_SUB; n++;
			}

			shape[n].layer_mask = 0;
		}
		else { /* work only from top shape */
			pcb_layer_type_t side = on_bottom ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
			if (copper_valid[0]) {
				pcb_pstk_shape_copy(&shape[n], &copper_shape[0]); shape[n].layer_mask = side | PCB_LYT_COPPER; n++;
			}
			if (mask_valid[0]) {
				pcb_pstk_shape_copy(&shape[n], &mask_shape[0]);   shape[n].layer_mask = side | PCB_LYT_MASK; shape[n].comb = PCB_LYC_AUTO | PCB_LYC_SUB; n++;
			}
			if (paste_valid[0]) {
				pcb_pstk_shape_copy(&shape[n], &paste_shape[0]);  shape[n].layer_mask = side | PCB_LYT_PASTE; shape[n].comb = PCB_LYC_AUTO; n++;
			}
			shape[n].layer_mask = 0;
		}

		if ((holewidth > 0) && (hole != holewidth)) {
			double xsize = holewidth, ysize = hole;
			rnd_printf("SLOT %mm %mm!\n", hole, holewidth);

			shape[n].shape = PCB_PSSH_LINE;
			shape[n].layer_mask = PCB_LYT_MECH;
			shape[n].comb = PCB_LYC_AUTO;
			if (xsize > ysize) {
				shape[n].data.line.x1 = -xsize/2 + ysize/2;
				shape[n].data.line.x2 = +xsize/2 - ysize/2;
				shape[n].data.line.y1 = shape[n].data.line.y2 = 0;
				shape[n].data.line.thickness = ysize;
			}
			else {
				shape[n].data.line.y1 = -ysize/2 + xsize/2;
				shape[n].data.line.y2 = +ysize/2 - xsize/2;
				shape[n].data.line.x1 = shape[n].data.line.x2 = 0;
				shape[n].data.line.thickness = xsize;
			}

			if (holerot != 0) {
				double hrot = holerot / RND_RAD_TO_DEG, cosa = cos(hrot), sina = sin(hrot);
printf("ROT: %f -> %f\n", holerot, hrot);
				rnd_rotate(&shape[n].data.line.x1, &shape[n].data.line.y1, 0, 0, cosa, sina);
				rnd_rotate(&shape[n].data.line.x2, &shape[n].data.line.y2, 0, 0, cosa, sina);
			}

			n++;
			hole = 0; /* turn off the orgiginal hole as slot will take over */
		}

		/* bbvia: start and end layers */
		if ((startly != NULL) || (endly != NULL))
			rnd_message(RND_MSG_ERROR, "Invalid pad: pads can't be blind or buried, making thru-hole\nIf you think you really have a blind/buried pad (not via), report this bug!");

		/* create the padstack */
		cl = altium_clearance(rctx, netid);
		ps = pcb_pstk_new_from_shape(((sc == NULL) ? rctx->pcb->Data : sc->data), x, y, hole, plated, cl * 2, shape);
		if (rot != 0)
			pcb_pstk_rotate(ps, x, y, cos(rot / RND_RAD_TO_DEG), sin(rot / RND_RAD_TO_DEG), rot);
		if (term != NULL) {
			assert(term->val_type == ALTIUM_FT_STR);
			pcb_attribute_put(&ps->Attributes, "term", term->val.str);
		}

		set_user_routed(ps, ur);

		/* free temporary pad shapes */
		for(n = 0; n < 3; n++) {
			pcb_pstk_shape_free(&copper_shape[n]);
			pcb_pstk_shape_free(&mask_shape[n]);
			pcb_pstk_shape_free(&paste_shape[n]);
		}
		for(n = 0; n < sizeof(shape)/sizeof(shape[0]); n++)
			pcb_pstk_shape_free(&shape[n]);

		/* assign net (netlist is stored on pad struct side) */
		if ((netid >= 0) && (sc != NULL)) {
			pcb_net_t *net = htip_get(&rctx->nets, netid);
			if (net != NULL) {
				if (sc->refdes == NULL)
					rnd_message(RND_MSG_ERROR, "Can't add pad to net %ld because parent subcircuit doesn't have a refdes (pad not assigned to net)\n", netid);
				else if (term == NULL)
					rnd_message(RND_MSG_ERROR, "Can't add pad to net %ld because it doesn't have a name (pad not assigned to net)\n", netid);
				else if (pcb_net_term_get(net, sc->refdes, term->val.str, PCB_NETA_ALLOC) == NULL)
					rnd_message(RND_MSG_ERROR, "Failed to add pad to net %ld (pad not assigned to net)\n", netid);
			}
			else
				rnd_message(RND_MSG_ERROR, "Invalid pad net ID: %ld (pad not assigned to net)\n", netid);
		}

		if (netid >= 0)
			htpi_set(&rctx->obj2netid, ps, netid);
	}

	return 0;
}

static int altium_finalize_subcs(rctx_t *rctx)
{
	htip_entry_t *e;

	if (rctx->pcb->Data->subc_tree == NULL)
		rnd_rtree_init(rctx->pcb->Data->subc_tree = malloc(sizeof(rnd_rtree_t)));

	for(e = htip_first(&rctx->comps); e != NULL; e = htip_next(&rctx->comps, e)) {
		pcb_subc_t *sc = e->value;
		pcb_subc_bbox(sc);
		rnd_rtree_insert(rctx->pcb->Data->subc_tree, sc, (rnd_rtree_box_t *)sc);
		pcb_subc_rebind(rctx->pcb, sc);
	}

	return 0;
}

static pcb_layer_t *altium_comp_layer(rctx_t *rctx, pcb_layer_t *ly, long compid, const char *otype, pcb_subc_t **sc_out)
{
	rnd_layer_id_t lid;
	pcb_subc_t *sc;

	sc = htip_get(&rctx->comps, compid);
	if (sc_out != NULL)
		*sc_out = sc;

	if (ly == NULL) {
		rnd_message(RND_MSG_ERROR, "Invalid %s object: no/unknown layer (%s not created)\n", otype, otype);
		return NULL;
	}

	if (compid < 0)
		return ly;

	if (sc == NULL) {
		rnd_message(RND_MSG_ERROR, "Invalid %s object: invalid parent subc (%s not created)\n", otype, otype);
		return NULL;
	}

	lid = pcb_layer_by_name(sc->data, ly->name);
	if (lid < 0)
		return pcb_subc_alloc_layer_like(sc, ly);
	return &sc->data->Layer[lid];
}

static int altium_parse_track(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_track]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_track], rec)) {
		altium_field_t *ur = NULL;
		pcb_layer_t *ly = NULL;
		rnd_coord_t x1 = RND_COORD_MAX, y1 = RND_COORD_MAX, x2 = RND_COORD_MAX, y2 = RND_COORD_MAX, w = RND_COORD_MAX;
		long compid = -1, netid = -1;
		rnd_coord_t cl;
		pcb_line_t *line;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_userrouted: ur = field; break;
				case altium_kw_field_net:   netid = conv_long_field(field); break;
				case altium_kw_field_layer: ly = conv_layer_field(rctx, field); break;
				case altium_kw_field_x1:    x1 = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y1:    y1 = conv_coordy_field(rctx, field); break;
				case altium_kw_field_x2:    x2 = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y2:    y2 = conv_coordy_field(rctx, field); break;
				case altium_kw_field_width: w = conv_coord_field(field); break;
				case altium_kw_field_component: compid = conv_long_field(field); break;
				default: break;
			}
		}
		if ((x1 == RND_COORD_MAX) || (y1 == RND_COORD_MAX) || (x2 == RND_COORD_MAX) || (y2 == RND_COORD_MAX) || (w == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid track object: missing coordinate or width (line not created)\n");
			continue;
		}
		if ((ly = altium_comp_layer(rctx, ly, compid, "line", NULL)) == NULL)
			continue;

		cl = altium_clearance(rctx, netid);
		line = pcb_line_new(ly, x1, y1, x2, y2, w, cl * 2, pcb_flag_make(PCB_FLAG_CLEARLINE));
		set_user_routed(line, ur);
	}

	return 0;
}

		/* convert arc start and end angles to pcb-rnd coord system */
#define ARC_CONV_ANGLES(sa, ea) \
	do { \
		sa = sa - 180; \
		ea = ea - 180; \
	} while(0)


static int altium_parse_arc(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_arc]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_arc], rec)) {
		altium_field_t *ur = NULL;
		pcb_layer_t *ly = NULL;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, r = RND_COORD_MAX, w = RND_COORD_MAX;
		double sa = -10000, ea = -10000;
		long compid = -1, netid = -1;
		rnd_coord_t cl;
		pcb_arc_t *arc;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_userrouted:ur = field; break;
				case altium_kw_field_net:         netid = conv_long_field(field); break;
				case altium_kw_field_layer:       ly = conv_layer_field(rctx, field); break;
				case altium_kw_field_location_x:  x = conv_coordx_field(rctx, field); break;
				case altium_kw_field_location_y:  y = conv_coordy_field(rctx, field); break;
				case altium_kw_field_width:       w = conv_coord_field(field); break;
				case altium_kw_field_radius:      r = conv_coord_field(field); break;
				case altium_kw_field_startangle:  sa = conv_double_field(field); break;
				case altium_kw_field_endangle:    ea = conv_double_field(field); break;
				case altium_kw_field_component:   compid = conv_long_field(field); break;
				default: break;
			}
		}
		if ((x == RND_COORD_MAX) || (y == RND_COORD_MAX) || (r == RND_COORD_MAX) || (w == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid arc object: missing coordinate or radius or width (arc not created)\n");
			continue;
		}
		if ((sa <= -10000) || (ea <= -10000)) {
			rnd_message(RND_MSG_ERROR, "Invalid arc object: missing angles (arc not created)\n");
			continue;
		}

		if ((ly = altium_comp_layer(rctx, ly, compid, "arc", NULL)) == NULL)
			continue;

		ARC_CONV_ANGLES(sa, ea);

		cl = altium_clearance(rctx, netid);
		arc = pcb_arc_new(ly, x, y, r, r, sa, ea-sa, w, cl * 2, pcb_flag_make(PCB_FLAG_CLEARLINE), 0);
		set_user_routed(arc, ur);
	}

	return 0;
}

static int altium_parse_text(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_text]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_text], rec)) {
		altium_field_t *ur = NULL;
		pcb_layer_t *ly = NULL;
		pcb_subc_t *sc;
		altium_field_t *ly_ascii = NULL, *text = NULL;
		pcb_text_t *t;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, x1 = RND_COORD_MAX, y1 = RND_COORD_MAX, x2 = RND_COORD_MAX, y2 = RND_COORD_MAX, w = RND_COORD_MAX;
		double rot = 0;
		int mir = 0, designator = 0, comment = 0;
		long compid = -1;
		long binly_lyt = 0, binly_offs = 0;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_userrouted:ur = field; break;
				case altium_kw_field__bin_layer_offs: binly_offs = conv_long_field(field); break;
				case altium_kw_field__bin_layer_type: binly_lyt = conv_long_field(field); break;
				case altium_kw_field_layer:       ly_ascii = field; break;
				case altium_kw_field_text:        text = field; break;
				case altium_kw_field_x:           x = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y:           y = conv_coordy_field(rctx, field); break;
				case altium_kw_field_x1:          x1 = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y1:          y1 = conv_coordy_field(rctx, field); break;
				case altium_kw_field_x2:          x2 = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y2:          y2 = conv_coordy_field(rctx, field); break;
				case altium_kw_field_width:       w = conv_coord_field(field); break;
				case altium_kw_field_mirror:      mir = conv_bool_field(field); break;
				case altium_kw_field_rotation:    rot = conv_double_field(field); break;
				case altium_kw_field_component:   compid = conv_long_field(field); break;
				case altium_kw_field_designator:  designator = conv_bool_field(field); break;
				case altium_kw_field_comment:     comment = conv_bool_field(field); break;
				default: break;
			}
		}

		if (binly_lyt != 0)
			ly = bin_layer(rctx, binly_lyt, binly_offs);
		else if (ly_ascii != NULL)
			ly = conv_layer_field(rctx, ly_ascii);
		else {
			rnd_message(RND_MSG_ERROR, "Invalid text object: missing layer field (text not created)\n");
			continue;
		}

		if ((x1 == RND_COORD_MAX) || (y1 == RND_COORD_MAX) || (x2 == RND_COORD_MAX) || (y2 == RND_COORD_MAX) || (w == RND_COORD_MAX)) {
			if ((x == RND_COORD_MAX) || (y == RND_COORD_MAX)) {
				rnd_message(RND_MSG_ERROR, "Invalid text object: missing coordinate or width (text not created)\n");
				continue;
			}
			else {
				TODO("estimate text size");
				x1 = x; y1 = y;
				if (text != NULL) {
					assert(text->val_type == ALTIUM_FT_STR);
					x2 = x + strlen(text->val.str) * RND_MM_TO_COORD(1.2);
					y2 = y + RND_MM_TO_COORD(1.8);
				}
			}
		}
		if (!designator && (text == NULL)) {
			rnd_message(RND_MSG_ERROR, "Invalid text object: missing text string (text not created)\n");
			continue;
		}


		if (comment && (ly != NULL)) /* move comments to assy */
			ly = conv_layer_assy(rctx, pcb_layer_flags_(ly) & PCB_LYT_BOTTOM);

		if ((ly = altium_comp_layer(rctx, ly, compid, "text", &sc)) == NULL)
			continue;

		if (designator && (text != NULL) && (sc != NULL) && (sc->refdes == NULL)) {
			/* special case: component name (refdes) is specified by designator text object (happens in protel99) */
			assert(text->val_type == ALTIUM_FT_STR);
			pcb_attribute_put(&sc->Attributes, "refdes", text->val.str);
		}

		if (x2 < x1) {
			rnd_coord_t tmp = x2;
			x2 = x1;
			x1 = tmp;
		}
		if (y2 < y1) {
			rnd_coord_t tmp = y2;
			y2 = y1;
			y1 = tmp;
		}

		if (text != NULL) {
			assert(text->val_type == ALTIUM_FT_STR);
			t = pcb_text_new_by_bbox(ly, pcb_font(rctx->pcb, 1, 1),
				x1, y1, x2-x1, y2-y1, 
				mir ? x2-x1 : 0, 0, 
				1.0, (mir ? PCB_TXT_MIRROR_X : 0), rot, w,
				(designator ? "%a.parent.refdes%" : text->val.str),
				pcb_flag_make(PCB_FLAG_CLEARLINE | (designator ? PCB_FLAG_DYNTEXT|PCB_FLAG_FLOATER : 0)));
			set_user_routed(t, ur);
		}
	}

	return 0;
}

#define POLY_VERT(field, dst, conv) \
do { \
	char *end; \
	rnd_coord_t c; \
	long idx = strtol(field->key+2, &end, 10); \
	if ((*end != '\0') || (idx < 0)) \
		break; \
	c = conv(rctx, field); \
	vtc0_set(dst, idx, c); \
} while(0)

#define POLY_ANG(field, dst) \
do { \
	char *end; \
	double d; \
	long idx = strtol(field->key+2, &end, 10); \
	if ((*end != '\0') || (idx < 0)) \
		break; \
	d = conv_double_field(field); \
	vtd0_set(dst, idx, d); \
} while(0)

static int add_poly_point_cb(void *uctx, rnd_coord_t x, rnd_coord_t y)
{
	pcb_poly_t *poly = uctx;
	pcb_poly_point_new(poly, x, y);
	return 0;
}

static int altium_parse_poly(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;
	vtc0_t vx = {0}, vy = {0}, cx = {0}, cy = {0};
	vtd0_t sa = {0}, ea = {0};

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_polygon]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_polygon], rec)) {
		altium_field_t *ur = NULL;
		pcb_poly_t *poly;
		pcb_layer_t *ly = NULL;
		long compid = -1, netid = -1, n;
		rnd_coord_t cl;

		vx.used = vy.used = cx.used = cy.used = 0;
		sa.used = ea.used = 0;
		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_userrouted:  ur = field; break;
				case altium_kw_field_net:         netid = conv_long_field(field); break;
				case altium_kw_field_layer:       ly = conv_layer_field(rctx, field); break;
				case altium_kw_field_component:   compid = conv_long_field(field); break;
				default:
					if (tolower(field->key[0]) == 'v') {
						if (tolower(field->key[1]) == 'x') POLY_VERT(field, &vx, conv_coordx_field);
						if (tolower(field->key[1]) == 'y') POLY_VERT(field, &vy, conv_coordy_field);
					}
					if (tolower(field->key[0]) == 'c') {
						if (tolower(field->key[1]) == 'x') POLY_VERT(field, &cx, conv_coordx_field);
						if (tolower(field->key[1]) == 'y') POLY_VERT(field, &cy, conv_coordy_field);
					}
					if ((tolower(field->key[0]) == 's') && (tolower(field->key[1]) == 'a')) POLY_ANG(field, &sa);
					if ((tolower(field->key[0]) == 'e') && (tolower(field->key[1]) == 'a')) POLY_ANG(field, &ea);
					break;
			}
		}

		if ((vx.used < 3) || (vy.used < 3) || (vx.used != vy.used)) {
			rnd_message(RND_MSG_ERROR, "Invalid polygon object: wrong number of vertices (polygon not created)\n");
			continue;
		}

		if ((ly = altium_comp_layer(rctx, ly, compid, "polygon", NULL)) == NULL)
			continue;

		cl = altium_clearance(rctx, netid);
		poly = pcb_poly_new(ly, cl * 2, pcb_flag_make(PCB_FLAG_CLEARPOLYPOLY | PCB_FLAG_CLEARPOLY));
		for(n = 0; n < vx.used; n++) {
			if ((sa.array[n] > 0) || (ea.array[n] > 0)) { /* arc - approximate with line */
				double r = rnd_distance(vx.array[n], vy.array[n], cx.array[n], cy.array[n]);
				double sa_tmp = sa.array[n], ea_tmp = ea.array[n];
				rnd_coord_t ex0, ey0, ex1, ey1;
				int rev;
				pcb_arc_t arc;

				/* create a dummy arc for the approximator */
				ARC_CONV_ANGLES(sa_tmp, ea_tmp);
				arc.X = cx.array[n];
				arc.Y = cy.array[n];
				arc.Width = arc.Height = r;
				arc.StartAngle = sa_tmp;
				arc.Delta = ea_tmp - sa_tmp;
				if (arc.Delta < 0)
					arc.Delta = 360+arc.Delta;

				/* figure which end matches our target and set 'reverse' approximation accordingly */
				pcb_arc_get_end(&arc, 0, &ex0, &ey0);
				pcb_arc_get_end(&arc, 1, &ex1, &ey1);
				rev = rnd_distance2(vx.array[n], vy.array[n], ex0, ey0) > rnd_distance2(vx.array[n], vy.array[n], ex1, ey1);

				/*printf(" angles: %f -> %f (%f %d)\n", sa_tmp, ea_tmp, arc.Delta, rev);*/
				pcb_arc_approx(&arc, -RND_MM_TO_COORD(0.2), rev, poly, add_poly_point_cb);
			}

			/* else plain line segment to [n]; in either case go to [n] exactly */
			pcb_poly_point_new(poly, vx.array[n], vy.array[n]);
		}
		pcb_add_poly_on_layer(ly, poly);
		set_user_routed(poly, ur);
		if (netid >= 0)
			htpi_set(&rctx->obj2netid, poly, netid);
	}

	vtc0_uninit(&vx);
	vtc0_uninit(&vy);
	vtc0_uninit(&cx);
	vtc0_uninit(&cy);
	vtd0_uninit(&sa);
	vtd0_uninit(&ea);
	return 0;
}

static int altium_parse_fill(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_fill]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_fill], rec)) {
		altium_field_t *ur = NULL;
		pcb_poly_t *poly;
		pcb_layer_t *ly = NULL;
		long compid = -1, netid = -1;
		rnd_coord_t cl, x1 = RND_COORD_MAX, y1 = RND_COORD_MAX, x2 = RND_COORD_MAX, y2 = RND_COORD_MAX;
		double rot = 0;
		int is_plane = 0;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_userrouted:  ur = field; break;
				case altium_kw_field_net:         netid = conv_long_field(field); break;
				case altium_kw_field_layer:       ly = conv_layer_field_(rctx, field, 0, &is_plane); break;
				case altium_kw_field_component:   compid = conv_long_field(field); break;
				case altium_kw_field_x1:          x1 = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y1:          y1 = conv_coordy_field(rctx, field); break;
				case altium_kw_field_x2:          x2 = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y2:          y2 = conv_coordy_field(rctx, field); break;
				case altium_kw_field_rotation:    rot = conv_double_field(field); break;
				default:
					break;
			}
		}

		if ((x1 == RND_COORD_MAX) || (y1 == RND_COORD_MAX) || (x2 == RND_COORD_MAX) || (y2 == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid fill object: missing coordinates (fill not created)\n");
			continue;
		}

		if ((ly = altium_comp_layer(rctx, ly, compid, "fill", NULL)) == NULL)
			continue;


		if (is_plane) { /* remember this as the plane hole doesn't yet exist */
			postpone_hole_t *ph = malloc(sizeof(postpone_hole_t));

			ph->ly = ly;
			ph->x1 = x1; ph->y1 = y2;
			ph->x2 = x2; ph->y2 = y1;
			ph->next = rctx->postholes;
			rctx->postholes = ph;
		}
		else {
			cl = altium_clearance(rctx, netid);
			poly = pcb_poly_new(ly, cl * 2, pcb_flag_make(PCB_FLAG_CLEARPOLYPOLY | PCB_FLAG_CLEARPOLY));
			pcb_poly_point_new(poly, x1, y1);
			pcb_poly_point_new(poly, x2, y1);
			pcb_poly_point_new(poly, x2, y2);
			pcb_poly_point_new(poly, x1, y2);
			pcb_add_poly_on_layer(ly, poly);
			set_user_routed(poly, ur);
		}
		if (netid >= 0)
			htpi_set(&rctx->obj2netid, poly, netid);
	}
	return 0;
}

static pcb_pstk_t *bb_via_new(rctx_t *rctx,pcb_data_t *data, long int id, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Thickness, rnd_coord_t Clearance, rnd_coord_t Mask, rnd_coord_t DrillingHole, pcb_flag_t Flags, altium_field_t *startly, altium_field_t *endly)
{
	pcb_pstk_t *p;
	pcb_pstk_compshape_t shp = PCB_PSTK_COMPAT_ROUND;
	int bb_top, bb_bottom, dummy;
	static rnd_layergrp_id_t top, bottom;
	rnd_layergrp_id_t bb_top_gid, bb_bottom_gid;
	pcb_layer_t *ly;

	/* resolve top and bottom copper for reference; cache them */
	if (rctx->bbvia_cache == 0) {
		if (pcb_layergrp_list(rctx->pcb, PCB_LYT_COPPER | PCB_LYT_TOP, &top, 1) != 1)
			return NULL;

		if (pcb_layergrp_list(rctx->pcb, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &bottom, 1) != 1)
			return NULL;

		rctx->bbvia_cache = 1;
	}

	/* resolve layer group id of hole top and bottom */
	ly = conv_layer_field_(rctx, startly, 0, &dummy);
	if (ly == NULL)
		return NULL;
	bb_top_gid = pcb_layer_get_group_(ly);

	ly = conv_layer_field_(rctx, endly, 0, &dummy);
	if (ly == NULL)
		return NULL;
	bb_bottom_gid = pcb_layer_get_group_(ly);

	/* calc hole distances from top and bottom */
	if (pcb_layergrp_dist(rctx->pcb, bb_top_gid, top, PCB_LYT_COPPER, &bb_top) != 0)
		return NULL;
	if (pcb_layergrp_dist(rctx->pcb, bottom, bb_bottom_gid, PCB_LYT_COPPER, &bb_bottom) != 0)
		return NULL;

	p = pcb_pstk_new_compat_bbvia(data, id, X, Y, DrillingHole, Thickness, Clearance/2, Mask, shp, !(Flags.f & PCB_FLAG_HOLE), bb_top, bb_bottom);
	p->Flags.f |= Flags.f & PCB_PSTK_VIA_COMPAT_FLAGS;

	return p;
}


static int altium_parse_via(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_via]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_via], rec)) {
		altium_field_t *startly = NULL, *endly = NULL;
		altium_field_t *ur = NULL;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, dia = RND_COORD_MAX, hole = RND_COORD_MAX;
		rnd_coord_t cl, mask = 0;
		long netid = -1, compid = -1;
		pcb_pstk_t *ps;
		pcb_subc_t *sc = NULL;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_startlayer:startly = field; break;
				case altium_kw_field_endlayer:  endly = field; break;
				case altium_kw_field_userrouted:ur = field; break;
				case altium_kw_field_net:      netid = conv_long_field(field); break;
				case altium_kw_field_component:compid = conv_long_field(field); break;
				case altium_kw_field_x:        x = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y:        y = conv_coordy_field(rctx, field); break;
				case altium_kw_field_diameter: dia = conv_coord_field(field); break;
				case altium_kw_field_holesize: hole = conv_coord_field(field); break;
TODO("TENTINGTOP and TENTINGBOTTOM");
				default: break;
			}
		}
		if ((x == RND_COORD_MAX) || (y == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid via object: missing coordinate (via not created)\n");
			continue;
		}
		if ((dia == RND_COORD_MAX) || (hole == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid via object: missing geometry (via not created)\n");
			continue;
		}

		if (compid >= 0) {
			sc = htip_get(&rctx->comps, compid);
			if (sc == NULL)
				rnd_message(RND_MSG_ERROR, "Invalid via object: can't find parent component (%ld); (via not created in subc but in board context)\n", compid);
		}

		cl = altium_clearance(rctx, netid);

		if ((startly != NULL) || (endly != NULL)) { /* bbvia */
			if ((startly == NULL) || (endly == NULL)) {
				rnd_message(RND_MSG_ERROR, "Invalid via: both start and end layer is required for bbvias - via is omitted\n");
				continue;
			}
			ps = bb_via_new(rctx, ((sc == NULL) ? rctx->pcb->Data : sc->data), -1, x, y, dia, cl * 2, mask, hole, pcb_flag_make(PCB_FLAG_CLEARLINE), startly, endly);
			if (ps == NULL) {
				rnd_message(RND_MSG_ERROR, "Internal error: failed to create bbvia\n");
				continue;
			}
		}
		else /* normal thru-hole via */
			ps = pcb_old_via_new(((sc == NULL) ? rctx->pcb->Data : sc->data), -1, x, y, dia, cl * 2, mask, hole, NULL, pcb_flag_make(PCB_FLAG_CLEARLINE));

		set_user_routed(ps, ur);
		if (netid >= 0)
			htpi_set(&rctx->obj2netid, ps, netid);
	}

	return 0;
}

long io_altium_obj2netid(void *ctx, void *obj)
{
	rctx_t *rctx = ctx;
	htpi_entry_t *e = htpi_getentry(&rctx->obj2netid, obj);

	if (e == NULL)
		return -1;
	return e->value;
}

static void altium_postproc(rctx_t *rctx)
{
	PCB_POLY_COPPER_LOOP(rctx->pcb->Data) {
		long netid = io_altium_obj2netid(rctx, polygon);
		if (netid < 0)
			continue;
		altium_post_poly_thermal_netname(rctx, rctx->pcb, polygon, netid, PCB_THERMAL_ROUND | PCB_THERMAL_ON, NULL);
	}
	PCB_ENDALL_LOOP;

}

static int io_altium_parse_pcbdoc_any(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest, int is_bin)
{
	rctx_t rctx = {0};
	int res = 0;

	static const pcb_dflgmap_t altium_dflgmap[] = {
		{"top_paste",           PCB_LYT_TOP | PCB_LYT_PASTE,     NULL, PCB_LYC_AUTO, 0},
		{"top_silk",            PCB_LYT_TOP | PCB_LYT_SILK,      NULL, PCB_LYC_AUTO, 0},
		{"top_mask",            PCB_LYT_TOP | PCB_LYT_MASK,      NULL, PCB_LYC_SUB | PCB_LYC_AUTO, 0},
		{"top_copper",          PCB_LYT_TOP | PCB_LYT_COPPER,    NULL, 0, 0},
		{"bottom_copper",       PCB_LYT_BOTTOM | PCB_LYT_COPPER, NULL, 0, 0},
		{"bottom_mask",         PCB_LYT_BOTTOM | PCB_LYT_MASK,   NULL, PCB_LYC_SUB | PCB_LYC_AUTO, PCB_DFLGMAP_FORCE_END},
		{"bottom_silk",         PCB_LYT_BOTTOM | PCB_LYT_SILK,   NULL, PCB_LYC_AUTO, PCB_DFLGMAP_FORCE_END},
		{"bottom_paste",        PCB_LYT_BOTTOM | PCB_LYT_PASTE,  NULL, PCB_LYC_AUTO, PCB_DFLGMAP_FORCE_END},

		{"outline",             PCB_LYT_BOUNDARY,                "uroute", 0, 0},
		{"pmech",               PCB_LYT_MECH,                    "proute", PCB_LYC_AUTO, 0},
		{"umech",               PCB_LYT_MECH,                    "uroute", PCB_LYC_AUTO, 0},
		{"keepout",             PCB_LYT_DOC,                     "altium.keepout", 0, 0},

		{NULL, 0}
	};


	rctx.pcb = pcb;
	rctx.filename = filename;

	if (is_bin)
		res = pcbdoc_bin_parse_file(&pcb->hidlib, &rctx.tree, filename);
	else
		res = pcbdoc_ascii_parse_file(&pcb->hidlib, &rctx.tree, filename);

	if (res != 0) {
		altium_tree_free(&rctx.tree);
		rnd_message(RND_MSG_ERROR, "failed to parse '%s' (%s mode)\n", filename, (is_bin ? "binary" : "ASCII"));
		return -1;
	}

	htip_init(&rctx.comps, longhash, longkeyeq);
	htip_init(&rctx.nets, longhash, longkeyeq);
	htic_init(&rctx.net_clr, longhash, longkeyeq);
	htpi_init(&rctx.obj2netid, ptrhash, ptrkeyeq);
	pcb_data_clip_inhibit_inc(rctx.pcb->Data);

	pcb_layergrp_upgrade_by_map(pcb, altium_dflgmap);
	pcb_layergrp_upgrade_by_map(pcb, pcb_dflgmap_doc);

	res |= altium_parse_board(&rctx);
	res |= altium_parse_net(&rctx);
	res |= altium_parse_class(&rctx);
	res |= altium_parse_rule(&rctx);
	res |= altium_parse_components(&rctx);
	res |= altium_parse_text(&rctx); /* have to read text right after components because designator may be coming from text objects */
	res |= altium_parse_pad(&rctx);
	res |= altium_parse_track(&rctx);
	res |= altium_parse_arc(&rctx);
	res |= altium_parse_via(&rctx);
	res |= altium_parse_poly(&rctx);
	res |= altium_parse_fill(&rctx);

	/* componentbody is not loaded: looks like 3d model with a floorplan, height and texture */

	altium_finalize_subcs(&rctx);

	/* if the board subtree didn't specify a board body polygon: use the bbox of all data read */
	if (rctx.has_bnd != 3) {
		rnd_box_t b;
		pcb_data_bbox(&b, rctx.pcb->Data, 0);
		rctx.pcb->hidlib.dwg.X1 = b.X1;
		rctx.pcb->hidlib.dwg.Y1 = b.Y1;
		rctx.pcb->hidlib.dwg.X2 = b.X2;
		rctx.pcb->hidlib.dwg.Y2 = b.Y2;
/* autocrop-like move, pre-librnd4:
		pcb_data_move(rctx.pcb->Data, -b.X1, -b.Y1, 0);
		rctx.moved_x = -b.X1;
		rctx.moved_y = -b.Y1;*/
		rnd_message(RND_MSG_ERROR, "Board without contour or body - can not determine real size\n");
	}

	altium_finalize_layers(&rctx); /* depends on board size figured */

	pcb_data_clip_inhibit_dec(rctx.pcb->Data, 1);

	altium_postproc(&rctx);

	htpi_uninit(&rctx.obj2netid);
	htic_uninit(&rctx.net_clr);
	htip_uninit(&rctx.nets);
	htip_uninit(&rctx.comps);
	altium_tree_free(&rctx.tree);
	return res;
}

int io_altium_parse_pcbdoc_ascii(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	return io_altium_parse_pcbdoc_any(ctx, pcb, filename, settings_dest, 0);
}

int io_altium_parse_pcbdoc_bin(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	return io_altium_parse_pcbdoc_any(ctx, pcb, filename, settings_dest, 1);
}

