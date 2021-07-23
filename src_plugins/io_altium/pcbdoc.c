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

#include <librnd/core/compat_misc.h>

#include "board.h"
#include "netlist.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_compat_help/pstk_help.h"

#include "pcbdoc_ascii.h"

#define LY_CACHE_MAX 64

typedef struct {
	altium_tree_t tree;
	pcb_board_t *pcb;
	const char *filename;

	/* caches */
	pcb_layer_t *lych[LY_CACHE_MAX];
	htip_t comps;
	htip_t nets;
} rctx_t;

static rnd_coord_t conv_coord_field(altium_field_t *field)
{
	double res;
	rnd_bool succ;
	const char *s, *unit = NULL;

	/* look for unit (anything non-numeric) */
	for(s = field->val; *s != '\0' && !isalpha(*s); s++) ;
	if (*s == '\0')
		unit = "mil";

	res = rnd_get_value(field->val, unit, NULL, &succ);
	if (!succ) {
		rnd_message(RND_MSG_ERROR, "failed to convert coord value '%s'\n", field->val);
		return 0;
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
	double res = strtod(field->val, &end);
	if (*end != '\0') {
		rnd_message(RND_MSG_ERROR, "failed to convert floating point value '%s'\n", field->val);
		return 0;
	}
	return res;
}

static long conv_long_field(altium_field_t *field)
{
	char *end;
	long res = strtol(field->val, &end, 10);
	if (*end != '\0') {
		rnd_message(RND_MSG_ERROR, "failed to convert integer value '%s'\n", field->val);
		return 0;
	}
	return res;
}

static int conv_bool_field(altium_field_t *field)
{
	if (rnd_strcasecmp(field->val, "true") == 0) return 1;
	if (rnd_strcasecmp(field->val, "false") == 0) return 0;
	rnd_message(RND_MSG_ERROR, "failed to convert bool value '%s'\n", field->val);
	return 0;
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

static pcb_layer_t *conv_layer_field(rctx_t *rctx, altium_field_t *field)
{
	int kw = altium_kw_sphash(field->val);

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
		case altium_kw_layers_keepout:       return conv_layer_(rctx, 9,  PCB_LYT_DOC, "keepout");
		case altium_kw_layers_drilldrawing:  return conv_layer_(rctx, 10, PCB_LYT_DOC, "drill");
		case altium_kw_layers_multilayer:    return conv_layer_(rctx, 11, PCB_LYT_DOC, "multilayer");
	}

	if ((rnd_strcasecmp(field->val, "MECHANICAL1") == 0))
		return conv_layer_(rctx, 12, PCB_LYT_BOUNDARY, NULL);
	if ((rnd_strcasecmp(field->val, "MECHANICAL15") == 0))
		return conv_layer_(rctx, 13, PCB_LYT_BOTTOM | PCB_LYT_DOC , "assy");

TODO("MID1...MID16: look up or create new intern copper; use cache index from 15+mid");
TODO("PLANE1...PLANE16: look up or create new intern copper; use cache index from 15+16+plane");
TODO("MECHANICAL2...MECHANICAL14: look up or create new doc?; use cache index from 15+16+16+mechanical");
	rnd_message(RND_MSG_ERROR, "Layer not found: '%s'\n", field->val);
	return NULL;
}

#define BUMP_COORD(dst, src) do { if (src > dst) dst = src; } while(0)

static int altium_parse_board(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_board]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_board], rec)) {
		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_sheetheight: rctx->pcb->hidlib.size_x = conv_coord_field(field); break;
				case altium_kw_field_sheetwidth:  rctx->pcb->hidlib.size_y = conv_coord_field(field); break;
				default:
					/* vx[0-4] and vy[0-4] */
					if ((tolower(field->key[0]) == 'v') && isdigit(field->key[2]) && (field->key[3] == 0)) {
						if (tolower(field->key[1]) == 'x')
							BUMP_COORD(rctx->pcb->hidlib.size_x, conv_coord_field(field));
						if (tolower(field->key[1]) == 'y')
							BUMP_COORD(rctx->pcb->hidlib.size_y, conv_coord_field(field));
					}
				break;
			}
		}
	}

	return 0;
}

static int altium_parse_net(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_net]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_net], rec)) {
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
		if (id < 0) {
			rnd_message(RND_MSG_ERROR, "Invalid net object: missing ID (net not created)\n");
			continue;
		}
		if (name == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid net object: missing name (net not created)\n");
			continue;
		}

		net = pcb_net_get(rctx->pcb, &rctx->pcb->netlist[PCB_NETLIST_INPUT], name->val, PCB_NETA_ALLOC);
		htip_set(&rctx->nets, id, net);
	}

	return 0;
}

static int altium_parse_components(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_component]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_component], rec)) {
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
		if (id < 0) {
			rnd_message(RND_MSG_ERROR, "Invalid component object: missing ID (component not created)\n");
			continue;
		}
		if (ly == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid component object: missing layer (component not created)\n");
			continue;
		}
		if (rnd_strcasecmp(ly->val, "bottom") == 0) on_bottom = 1;
		else if (rnd_strcasecmp(ly->val, "top") == 0) on_bottom = 0;
		else {
			rnd_message(RND_MSG_ERROR, "Invalid component object: invalid layer '%s' (should be top or bottom; component not created)\n", ly->val);
			continue;
		}

		sc = pcb_subc_alloc();
		pcb_subc_create_aux(sc, x, y, rot, on_bottom);
		if (refdes != NULL)
			pcb_attribute_put(&sc->Attributes, "refdes", refdes->val);
		if (footprint != NULL)
			pcb_attribute_put(&sc->Attributes, "footprint", footprint->val);
		pcb_subc_reg(rctx->pcb->Data, sc);
		pcb_subc_bind_globals(rctx->pcb, sc);
		htip_set(&rctx->comps, id, sc);
	}

	return 0;
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
		pcb_pstk_shape_t shape[8], master_shape = {0}, mask_shape = {0};
		long compid = -1, netid = -1;
		int on_all = 0, on_bottom = 0, plated = 0;
		double rot = 0;
		rnd_coord_t cl = 0;
		TODO("figure clearance for cl");

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

		if (compid >= 0)
			sc = htip_get(&rctx->comps, compid);
		if (sc == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid pad object: can't find parent component (pad not created)\n");
			continue;
		}

		if (rnd_strcasecmp(ly->val, "bottom") == 0) on_bottom = 1;
		else if (rnd_strcasecmp(ly->val, "top") == 0) on_bottom = 0;
		else if (rnd_strcasecmp(ly->val, "multilayer") == 0) on_all = 1;
		else {
			rnd_message(RND_MSG_ERROR, "Invalid pad object: invalid layer '%s' (should be top or bottom or multilayer; pad not created)\n", ly->val);
			continue;
		}

		if ((rnd_strcasecmp(shapename->val, "rectangle") == 0) || (rnd_strcasecmp(shapename->val, "roundedrectangle") == 0)) {
			pcb_shape_rect(&master_shape, xsize, ysize);
			pcb_shape_rect(&mask_shape, xsize + 2*PCB_PROTO_MASK_BLOAT, ysize + 2*PCB_PROTO_MASK_BLOAT);
		}
		else if (rnd_strcasecmp(shapename->val, "round") == 0) {
			if (xsize == ysize) {
				master_shape.shape = mask_shape.shape = PCB_PSSH_CIRC;
				master_shape.data.circ.x = master_shape.data.circ.y = 0;
				master_shape.data.circ.dia = xsize;
				mask_shape.data.circ.x = mask_shape.data.circ.y = 0;
				mask_shape.data.circ.dia = xsize + 2*PCB_PROTO_MASK_BLOAT;
			}
			else {
				master_shape.shape = mask_shape.shape = PCB_PSSH_LINE;
				if (xsize > ysize) {
					master_shape.data.line.x1 = mask_shape.data.line.x1 = -xsize/2 + ysize/2;
					master_shape.data.line.x2 = mask_shape.data.line.x2 = +xsize/2 - ysize/2;
					master_shape.data.line.y1 = mask_shape.data.line.y1 = master_shape.data.line.y2 = mask_shape.data.line.y2 = 0;
					master_shape.data.line.thickness = ysize;
					mask_shape.data.line.thickness = ysize + 2*PCB_PROTO_MASK_BLOAT;
				}
				else {
					master_shape.data.line.y1 = mask_shape.data.line.y1 = -ysize/2 + xsize/2;
					master_shape.data.line.y2 = mask_shape.data.line.y2 = +ysize/2 - xsize/2;
					master_shape.data.line.x1 = mask_shape.data.line.x1 = master_shape.data.line.x2 = mask_shape.data.line.x2 = 0;
					master_shape.data.line.thickness = xsize;
					mask_shape.data.line.thickness = xsize + 2*PCB_PROTO_MASK_BLOAT;
				}
			}
		}
		else {
			rnd_message(RND_MSG_ERROR, "Invalid pad object: invalid shape '%s' (pad not created)\n", shapename->val);
			continue;
		}


		if (on_all) {
			shape[0] = master_shape; shape[0].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER;
			shape[1] = master_shape; shape[1].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER;
			shape[2] = master_shape; shape[2].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER;
			shape[3] = mask_shape;   shape[3].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK;
			shape[4] = mask_shape;   shape[4].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK;
			shape[5].layer_mask = 0;
		}
		else {
			pcb_layer_type_t side = on_bottom ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
			shape[0] = master_shape; shape[0].layer_mask = side | PCB_LYT_COPPER;
			shape[1] = mask_shape;   shape[1].layer_mask = side | PCB_LYT_MASK;
			shape[2] = master_shape; shape[2].layer_mask = side | PCB_LYT_PASTE;
			shape[3].layer_mask = 0;
		}

		ps = pcb_pstk_new_from_shape(sc->data, x, y, hole, plated, cl, shape);
		if (rot != 0)
			pcb_pstk_rotate(ps, x, y, cos(rot / RND_RAD_TO_DEG), sin(rot / RND_RAD_TO_DEG), rot);
		if (term != NULL)
			pcb_attribute_put(&ps->Attributes, "term", term->val);

		if (netid >= 0) {
			pcb_net_t *net = htip_get(&rctx->nets, netid);
			if (net != NULL) {
				if (sc->refdes == NULL)
					rnd_message(RND_MSG_ERROR, "Can't add pad to net %ld because parent subcircuit doesn't have a refdes (pad not assigned to net)\n", netid);
				else if (term == NULL)
					rnd_message(RND_MSG_ERROR, "Can't add pad to net %ld because it doesn't have a name (pad not assigned to net)\n", netid);
				else if (pcb_net_term_get(net, sc->refdes, term->val, PCB_NETA_ALLOC) == NULL)
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

static pcb_layer_t *altium_comp_layer(rctx_t *rctx, pcb_layer_t *ly, long compid, const char *otype)
{
	rnd_layer_id_t lid;
	pcb_subc_t *sc;

	if (ly == NULL) {
		rnd_message(RND_MSG_ERROR, "Invalid %s object: no/unknown layer (%s not created)\n", otype, otype);
		return NULL;
	}

	if (compid < 0)
		return ly;

	sc = htip_get(&rctx->comps, compid);

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
		long compid = -1;
		rnd_coord_t cl = 0;
		TODO("figure clearance for cl");

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
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
		if ((ly = altium_comp_layer(rctx, ly, compid, "line")) == NULL)
			continue;

		pcb_line_new(ly, x1, y1, x2, y2, w, cl, pcb_flag_make(PCB_FLAG_CLEARLINE));
	}

	return 0;
}

static int altium_parse_arc(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_arc]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_arc], rec)) {
		pcb_layer_t *ly = NULL;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, r = RND_COORD_MAX, w = RND_COORD_MAX;
		double sa = -10000, ea = -10000;
		long compid = -1;
		rnd_coord_t cl = 0;
		TODO("figure clearance for cl");

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
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

		if ((ly = altium_comp_layer(rctx, ly, compid, "arc")) == NULL)
			continue;

		/* convert to pcb-rnd coord system */
		sa = sa - 180;
		ea = ea - 180;

		pcb_arc_new(ly, x, y, r, r, sa, ea-sa, w, cl, pcb_flag_make(PCB_FLAG_CLEARLINE), 0);
	}

	return 0;
}

static int altium_parse_via(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_via]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_via], rec)) {
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, dia = RND_COORD_MAX, hole = RND_COORD_MAX;
		rnd_coord_t cl = 0, mask = 0;
		TODO("figure clearance for cl");

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
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
		pcb_old_via_new(rctx->pcb->Data, -1, x, y, dia, cl, mask, hole, NULL, pcb_flag_make(PCB_FLAG_CLEARLINE));
	}

	return 0;
}

int io_altium_parse_pcbdoc_ascii(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	rctx_t rctx = {0};
	int res = 0;

	rctx.pcb = pcb;
	rctx.filename = filename;

	if (pcbdoc_ascii_parse_file(&pcb->hidlib, &rctx.tree, filename) != 0) {
		altium_tree_free(&rctx.tree);
		rnd_message(RND_MSG_ERROR, "failed to parse '%s'\n", filename);
		return -1;
	}

	htip_init(&rctx.comps, longhash, longkeyeq);
	htip_init(&rctx.nets, longhash, longkeyeq);

	pcb_layergrp_upgrade_by_map(pcb, pcb_dflgmap);
	pcb_layergrp_upgrade_by_map(pcb, pcb_dflgmap_doc);

	res |= altium_parse_board(&rctx);
	res |= altium_parse_net(&rctx);
	res |= altium_parse_components(&rctx);
	res |= altium_parse_pad(&rctx);
	res |= altium_parse_track(&rctx);
	res |= altium_parse_arc(&rctx);
	res |= altium_parse_via(&rctx);

	altium_finalize_subcs(&rctx);

	htip_uninit(&rctx.nets);
	htip_uninit(&rctx.comps);
	altium_tree_free(&rctx.tree);
	return res;
}

