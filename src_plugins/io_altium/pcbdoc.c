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
#include <genht/hash.h>
#include <genvector/vtd0.h>

#include <librnd/core/compat_misc.h>
#include <librnd/core/vtc0.h>

#include "board.h"
#include "netlist.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_compat_help/pstk_help.h"

#include "pcbdoc_ascii.h"
#include "pcbdoc_bin.h"
#include "htic.h"

#define LY_CACHE_MAX 64

typedef struct {
	altium_tree_t tree;
	pcb_board_t *pcb;
	const char *filename;

	int has_bnd;  /* set to 3 after altium_parse_board() if board bbox picked up */

	/* caches */
	pcb_layer_t *lych[LY_CACHE_MAX];
	htip_t comps;
	htip_t nets;
	htic_t net_clr;
	rnd_coord_t global_clr;
	pcb_layer_t *midly[64];
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
	return rctx->pcb->hidlib.size_y - conv_coord_field(field);
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
	}

	abort();
}

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

static pcb_layer_t *conv_layer_field(rctx_t *rctx, altium_field_t *field)
{
	int kw;

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
		rnd_message(RND_MSG_ERROR, "Drawing on PLANE layer %s is not supported\n", field->val.str);
		return NULL;
	}
TODO("MECHANICAL2...MECHANICAL14: look up or create new doc?; use cache index from 15+16+16+mechanical");
	rnd_message(RND_MSG_ERROR, "Layer not found: '%s'\n", field->val.str);
	return NULL;
}

#define BUMP_COORD(dst, src) do { if (src > dst) dst = src; } while(0)

typedef struct {
	const char *name;
	int prev, next, seen;
} altium_layer_t;

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
				pcb_layergrp_t *g = pcb_get_grp_new_intern(rctx->pcb, n-2);
				rnd_layer_id_t lid = pcb_layer_create(rctx->pcb, g - rctx->pcb->LayerGroups.grp, layers[n].name, 0);
				rctx->midly[n-2] = pcb_get_layer(rctx->pcb->Data, lid);
				if ((n >= 39) || (n <= 54)) {
					/* internal plane layer - draw a poly at the end */
					pcb_attribute_put(&(rctx->midly[n-2]->Attributes), "altium::plane", "1");
				}
			}
		}

		layers[n].seen = 1;
		n = layers[n].next;
	}
	return cop;
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
			pcb_poly_point_new(poly, 0, 0);
			pcb_poly_point_new(poly, rctx->pcb->hidlib.size_x, 0);
			pcb_poly_point_new(poly, rctx->pcb->hidlib.size_x, rctx->pcb->hidlib.size_y);
			pcb_poly_point_new(poly, 0, rctx->pcb->hidlib.size_y);
			pcb_add_poly_on_layer(rctx->midly[n], poly);
			plane[n-37] = poly;
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
				case altium_kw_field_sheetheight: rctx->pcb->hidlib.size_x = conv_coord_field(field); break;
				case altium_kw_field_sheetwidth:  rctx->pcb->hidlib.size_y = conv_coord_field(field); break;
				default:
					/* vx[0-4] and vy[0-4] */
					if ((tolower(field->key[0]) == 'v') && isdigit(field->key[2]) && (field->key[3] == 0)) {
						if (tolower(field->key[1]) == 'x') {
							BUMP_COORD(rctx->pcb->hidlib.size_x, conv_coord_field(field));
							rctx->has_bnd |= 1;
						}
						if (tolower(field->key[1]) == 'y') {
							BUMP_COORD(rctx->pcb->hidlib.size_y, conv_coord_field(field));
							rctx->has_bnd |= 2;
						}
					}
					if (rnd_strncasecmp(field->key, "layer", 5) == 0) {
						char *end;
						long idx = strtol(field->key+5, &end, 10);
						int fid;

						if ((idx < 0) || (idx > layers_max))
							break;
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
		rctx->pcb->hidlib.size_x = rctx->pcb->hidlib.size_y = 0;

	/*** create the layer stack (copper only) ***/

printf("Layer stack:\n");

	/* figure the first (top) layer: prev == 0, next != 0 */
	for(n = 1; n < layers_max; n++)
		if ((layers[n].prev == 0) && (layers[n].next != 0))
			seen_cop |= altium_layer_list(rctx, layers, layers_max, n);

	if (!seen_cop)
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
		else
			continue;

		assert(scval->val_type == ALTIUM_FT_STR);

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
				rctx->global_clr = gap;
				break;
			case altium_kw_field_netclass:
				{
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

static int altium_parse_pad(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_pad]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_pad], rec)) {
		pcb_pstk_t *ps;
		pcb_subc_t *sc = NULL;
		altium_field_t *ly = NULL, *term = NULL, *shapename = NULL;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, xsize = RND_COORD_MAX, ysize = RND_COORD_MAX, hole = 0;
		rnd_coord_t mask_man = RND_COORD_MAX, paste_man = RND_COORD_MAX, mask_fin = PCB_PROTO_MASK_BLOAT, paste_fin = 0;
		pcb_pstk_shape_t shape[8] = {0}, copper_shape = {0}, mask_shape = {0}, paste_shape = {0};
		long compid = -1, netid = -1;
		int on_all = 0, on_bottom = 0, plated = 0, mask_mode = -1, paste_mode = -1, copper_valid = 0, mask_valid = 0, paste_valid = 0, n;
		double rot = 0;
		rnd_coord_t cl;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_name:      term = field; break;
				case altium_kw_field_layer:     ly = field; break;
				case altium_kw_field_shape:     shapename = field; break;
				case altium_kw_field_x:         x = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y:         y = conv_coordy_field(rctx, field); break;
				case altium_kw_field_xsize:     xsize = conv_coord_field(field); break;
				case altium_kw_field_ysize:     ysize = conv_coord_field(field); break;
				case altium_kw_field_holesize:  hole = conv_coord_field(field); break;
				case altium_kw_field_rotation:  rot = conv_double_field(field); break;
				case altium_kw_field_component: compid = conv_long_field(field); break;
				case altium_kw_field_net:       netid = conv_long_field(field); break;
				case altium_kw_field_plated:    plated = conv_bool_field(field); break;

				case altium_kw_field_pastemaskexpansionmode:     paste_mode = get_expansion_mode(field); break;
				case altium_kw_field_soldermaskexpansionmode:    mask_mode = get_expansion_mode(field); break;
				case altium_kw_field_pastemaskexpansion_manual:  paste_man = conv_coord_field(field); break;
				case altium_kw_field_soldermaskexpansion_manual: mask_man = conv_coord_field(field); break;

TODO("HOLEWIDTH, HOLEROTATION");
TODO("STARTLAYER and ENDLAYER (for bbvias)");
				default: break;
			}
		}
		if ((x == RND_COORD_MAX) || (y == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid pad object: missing coordinate (pad not created)\n");
			continue;
		}
		if ((xsize == RND_COORD_MAX) || (ysize == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid pad object: missing size (pad not created)\n");
			continue;
		}
		if (shapename == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid pad object: missing shape (pad not created)\n");
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
		switch(get_shape(shapename)) {
			case ALTIUM_SHAPE_RECTANGLE:
			case ALTIUM_SHAPE_ROUND_RECTANGLE:
				pcb_shape_rect(&copper_shape, xsize, ysize);
				copper_valid = 1;
				if (((xsize + mask_fin*2) > 0) && ((ysize + mask_fin*2) > 0)) {
					pcb_shape_rect(&mask_shape, xsize + mask_fin*2, ysize + mask_fin*2);
					mask_valid = 1;
				}
				if (((xsize + paste_fin*2) > 0) && ((ysize + paste_fin*2) > 0)) {
					pcb_shape_rect(&paste_shape, xsize + paste_fin*2, ysize + paste_fin*2);
					paste_valid = 1;
				}
				break;
			case ALTIUM_SHAPE_ROUND:
				if (xsize == ysize) {
					copper_shape.shape = mask_shape.shape = PCB_PSSH_CIRC;
					copper_shape.data.circ.x = copper_shape.data.circ.y = 0;
					copper_shape.data.circ.dia = xsize;
					copper_valid = 1;
					mask_shape.data.circ.x = mask_shape.data.circ.y = 0;
					mask_shape.data.circ.dia = xsize + mask_fin*2;
					mask_valid = (mask_shape.data.circ.dia > 0);
					paste_shape = copper_shape;
					paste_shape.data.circ.dia += paste_fin*2;
					paste_valid = (paste_shape.data.circ.dia > 0);
				}
				else {
					copper_shape.shape = mask_shape.shape = PCB_PSSH_LINE;
					if (xsize > ysize) {
						copper_shape.data.line.x1 = mask_shape.data.line.x1 = -xsize/2 + ysize/2;
						copper_shape.data.line.x2 = mask_shape.data.line.x2 = +xsize/2 - ysize/2;
						copper_shape.data.line.y1 = mask_shape.data.line.y1 = copper_shape.data.line.y2 = mask_shape.data.line.y2 = 0;
						copper_shape.data.line.thickness = ysize;
						copper_valid = 1;
						mask_shape.data.line.thickness = ysize + mask_fin*2;
						mask_valid = (mask_shape.data.line.thickness > 0);
						paste_shape = copper_shape;
						paste_shape.data.line.thickness += paste_fin*2;
						paste_valid = (paste_shape.data.line.thickness > 0);
					}
					else {
						copper_shape.data.line.y1 = mask_shape.data.line.y1 = -ysize/2 + xsize/2;
						copper_shape.data.line.y2 = mask_shape.data.line.y2 = +ysize/2 - xsize/2;
						copper_shape.data.line.x1 = mask_shape.data.line.x1 = copper_shape.data.line.x2 = mask_shape.data.line.x2 = 0;
						copper_shape.data.line.thickness = xsize;
						copper_valid = 1;
						mask_shape.data.line.thickness = xsize + mask_fin*2;
						mask_valid = (mask_shape.data.line.thickness > 0);;
						paste_shape = copper_shape;
						paste_shape.data.line.thickness += paste_fin*2;
						paste_valid = (paste_shape.data.line.thickness > 0);
					}
				}
				break;
			default:
				rnd_message(RND_MSG_ERROR, "Invalid pad object: invalid shape (pad not created)\n");
				continue;
		}

		/* create shape stackup in shape[] */
		n = 0;
		if (on_all) {
			if (copper_valid) {
				pcb_pstk_shape_copy(&shape[n], &copper_shape); shape[n].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER; n++;
				pcb_pstk_shape_copy(&shape[n], &copper_shape); shape[n].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; n++;
				pcb_pstk_shape_copy(&shape[n], &copper_shape); shape[n].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; n++;
			}
			if (mask_valid) {
				pcb_pstk_shape_copy(&shape[n], &mask_shape);   shape[n].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK; shape[n].comb = PCB_LYC_AUTO | PCB_LYC_SUB; n++;
				pcb_pstk_shape_copy(&shape[n], &mask_shape);   shape[n].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK; shape[n].comb = PCB_LYC_AUTO | PCB_LYC_SUB; n++;
			}
			shape[n].layer_mask = 0;
		}
		else {
			pcb_layer_type_t side = on_bottom ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
			if (copper_valid) {
				pcb_pstk_shape_copy(&shape[n], &copper_shape); shape[n].layer_mask = side | PCB_LYT_COPPER; n++;
			}
			if (mask_valid) {
				pcb_pstk_shape_copy(&shape[n], &mask_shape);   shape[n].layer_mask = side | PCB_LYT_MASK; shape[n].comb = PCB_LYC_AUTO | PCB_LYC_SUB; n++;
			}
			if (paste_valid) {
				pcb_pstk_shape_copy(&shape[n], &paste_shape);  shape[n].layer_mask = side | PCB_LYT_PASTE; shape[n].comb = PCB_LYC_AUTO; n++;
			}
			shape[n].layer_mask = 0;
		}

		/* create the padstack */
		cl = altium_clearance(rctx, netid);
		ps = pcb_pstk_new_from_shape(((sc == NULL) ? rctx->pcb->Data : sc->data), x, y, hole, plated, cl * 2, shape);
		if (rot != 0)
			pcb_pstk_rotate(ps, x, y, cos(rot / RND_RAD_TO_DEG), sin(rot / RND_RAD_TO_DEG), rot);
		if (term != NULL) {
			assert(term->val_type == ALTIUM_FT_STR);
			pcb_attribute_put(&ps->Attributes, "term", term->val.str);
		}

		/* free temporary pad shapes */
		pcb_pstk_shape_free(&copper_shape);
		pcb_pstk_shape_free(&mask_shape);
		pcb_pstk_shape_free(&paste_shape);
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
	}

	return 0;
}

static int altium_finalize_subcs(rctx_t *rctx)
{
	htip_entry_t *e;

	if (rctx->pcb->Data->subc_tree == NULL)
		rctx->pcb->Data->subc_tree = rnd_r_create_tree();

	for(e = htip_first(&rctx->comps); e != NULL; e = htip_next(&rctx->comps, e)) {
		pcb_subc_t *sc = e->value;
		pcb_subc_bbox(sc);
		rnd_r_insert_entry(rctx->pcb->Data->subc_tree, (rnd_box_t *)sc);
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
		rnd_message(RND_MSG_ERROR, "Invalid track object: invalid parent subc (line not created)\n");
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
		pcb_layer_t *ly = NULL;
		rnd_coord_t x1 = RND_COORD_MAX, y1 = RND_COORD_MAX, x2 = RND_COORD_MAX, y2 = RND_COORD_MAX, w = RND_COORD_MAX;
		long compid = -1, netid = -1;
		rnd_coord_t cl;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
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
		pcb_line_new(ly, x1, y1, x2, y2, w, cl * 2, pcb_flag_make(PCB_FLAG_CLEARLINE));
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
		pcb_layer_t *ly = NULL;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, r = RND_COORD_MAX, w = RND_COORD_MAX;
		double sa = -10000, ea = -10000;
		long compid = -1, netid = -1;
		rnd_coord_t cl;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
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
		pcb_arc_new(ly, x, y, r, r, sa, ea-sa, w, cl * 2, pcb_flag_make(PCB_FLAG_CLEARLINE), 0);
	}

	return 0;
}

static int altium_parse_text(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_text]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_text], rec)) {
		pcb_layer_t *ly = NULL;
		pcb_subc_t *sc;
		altium_field_t *text = NULL;
		pcb_text_t *t;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, x1 = RND_COORD_MAX, y1 = RND_COORD_MAX, x2 = RND_COORD_MAX, y2 = RND_COORD_MAX, w = RND_COORD_MAX;
		double rot = 0;
		int mir = 0, designator = 0, comment = 0;
		long compid = -1;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_layer:       ly = conv_layer_field(rctx, field); break;
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

		assert(text->val_type == ALTIUM_FT_STR);
		t = pcb_text_new_by_bbox(ly, pcb_font(rctx->pcb, 1, 1), x1, y1, x2-x1, y2-y1, 0, 0, 1.0, mir, rot, w, (designator ? "%a.parent.refdes%" : text->val.str), pcb_flag_make(PCB_FLAG_CLEARLINE | (designator ? PCB_FLAG_DYNTEXT|PCB_FLAG_FLOATER : 0)));
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
		pcb_poly_t *poly;
		pcb_layer_t *ly = NULL;
		long compid = -1, netid = -1, n;
		rnd_coord_t cl;

		vx.used = vy.used = cx.used = cy.used = 0;
		sa.used = ea.used = 0;
		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
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
	}

	vtc0_uninit(&vx);
	vtc0_uninit(&vy);
	vtc0_uninit(&cx);
	vtc0_uninit(&cy);
	vtd0_uninit(&sa);
	vtd0_uninit(&ea);
	return 0;
}

static int altium_parse_via(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_via]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_via], rec)) {
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, dia = RND_COORD_MAX, hole = RND_COORD_MAX;
		rnd_coord_t cl, mask = 0;
		long netid = -1;

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_net:      netid = conv_long_field(field); break;
				case altium_kw_field_x:        x = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y:        y = conv_coordy_field(rctx, field); break;
				case altium_kw_field_diameter: dia = conv_coord_field(field); break;
				case altium_kw_field_holesize: hole = conv_coord_field(field); break;
TODO("TENTINGTOP and TENTINGBOTTOM");
TODO("STARTLAYER and ENDLAYER (for bbvias)");
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
		cl = altium_clearance(rctx, netid);
		pcb_old_via_new(rctx->pcb->Data, -1, x, y, dia, cl * 2, mask, hole, NULL, pcb_flag_make(PCB_FLAG_CLEARLINE));
	}

	return 0;
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

	/* componentbody is not loaded: looks like 3d model with a floorplan, height and texture */

	altium_finalize_subcs(&rctx);

	/* if the board subtree didn't specify a board body polygon: use the bbox of all data read */
	if (rctx.has_bnd != 3) {
		rnd_box_t b;
		pcb_data_bbox(&b, rctx.pcb->Data, 0);
		rctx.pcb->hidlib.size_x = b.X2-b.X1;
		rctx.pcb->hidlib.size_y = b.Y2-b.Y1;
		pcb_data_move(rctx.pcb->Data, -b.X1, -b.Y1, 0);
		rnd_message(RND_MSG_ERROR, "Board without contour or body - can not determine real size\n");
	}

	altium_finalize_layers(&rctx); /* depends on board size figured */

	pcb_data_clip_inhibit_dec(rctx.pcb->Data, 1);
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

