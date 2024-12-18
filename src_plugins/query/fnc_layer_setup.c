/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020,2024 Tibor 'Igor2' Palinkas
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

/* Query language - layer setup checks */

typedef enum { LSL_ON, LSL_BELOW, LSL_ABOVE, LSL_max } layer_setup_loc_t;
typedef enum { LST_NONE, LST_TYPE, LST_NET, LST_NETMARGIN, LST_RESULT, LST_UNCOVERED, LST_SUBSTRATE } layer_setup_target_t;

typedef struct {
	/* condition/check */
	pcb_layer_type_t require_lyt[LSL_max];
	pcb_layer_type_t refuse_lyt[LSL_max];
	pcb_net_t *require_net[LSL_max];
	rnd_coord_t net_margin[LSL_max];


	/* query result request: loc and target */
	layer_setup_loc_t res_loc;
	layer_setup_target_t res_target;

	unsigned valid:1; /* 0 on syntax error */
} layer_setup_t;

/* measurements saved during the condition/check phase */
typedef struct {
	int matched;                /* 1 if condition matched, 0 otherwise */
	double uncovered[LSL_max];  /* area of uncovered portion in nm^2, after a require_net check */
	pcb_layergrp_t *substrate[LSL_max];
} layer_setup_res_t;

static layer_setup_res_t ls_invalid;

typedef struct {
	pcb_any_obj_t *obj;
	const layer_setup_t *ls;
	int above_dir;
} layer_setup_req_t;


typedef layer_setup_req_t htls_key_t;
typedef layer_setup_res_t htls_value_t;
#define HT_INVALID_VALUE ls_invalid
#define HT(x) htls_ ## x
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT_INVALID_VALUE
#undef HT

static int lsreqkeyeq(layer_setup_req_t a, layer_setup_req_t b)
{
	if (a.obj != b.obj) return 0;
	if (a.ls != b.ls) return 0;
	if (a.above_dir != b.above_dir) return 0;
	return 1;
}

static unsigned lsreqhash(layer_setup_req_t k)
{
	return ptrhash(k.obj) ^ ptrhash(k.ls) ^ k.above_dir;
}

void pcb_qry_uninit_layer_setup(pcb_qry_exec_t *ectx)
{
	if (!ectx->layer_setup_inited)
		return;

	genht_uninit_deep(htpp, &ectx->layer_setup_precomp, {
		free(htent->value);
	});

	vtp0_uninit(&ectx->layer_setup_netobjs);
	ectx->layer_setup_inited = 0;
	htls_free(ectx->layer_setup_res_cache);
}

static void pcb_qry_init_layer_setup(pcb_qry_exec_t *ectx)
{
	htpp_init(&ectx->layer_setup_precomp, ptrhash, ptrkeyeq);
	ectx->layer_setup_inited = 1;
	vtp0_init(&ectx->layer_setup_netobjs);
	ectx->layer_setup_res_cache = htls_alloc(lsreqhash, lsreqkeyeq);
}

#define LSC_RESET() \
do { \
	val = NULL; \
	loc = LSL_ON; \
	target = LST_NONE; \
} while(0)

#define LSC_GET_VAL(err_stmt) \
do { \
	len = s-val; \
	if ((val == NULL) || (len >= sizeof(tmp))) { err_stmt; } \
	strncpy(tmp, val, len); \
	tmp[len] = '\0'; \
} while(0)

#define LSC_PARSE_LOCTARGET(s) \
do { \
		if (strncmp(s, "above", 5) == 0)           { s += 5; loc = LSL_ABOVE; } \
		else if (strncmp(s, "below", 5) == 0)      { s += 5; loc = LSL_BELOW; } \
		else if (strncmp(s, "result", 6) == 0)     { s += 6; target = LST_RESULT; } \
		else if (strncmp(s, "substrate", 9) == 0)  { s += 9; target = LST_SUBSTRATE; } \
		else if (strncmp(s, "netmargin", 9) == 0)  { s += 9; target = LST_NETMARGIN; } \
		else if (strncmp(s, "uncovered", 9) == 0)  { s += 9; target = LST_UNCOVERED; } \
		else if (strncmp(s, "type", 4) == 0)       { s += 4; target = LST_TYPE; } \
		else if (strncmp(s, "net", 3) == 0)        { s += 3; target = LST_NET; } \
		else if (strncmp(s, "on", 2) == 0)         { s += 2; loc = LSL_ON; } \
		else { \
			rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid target or location in '%s'\n", s); \
			return -1; \
		} \
} while(0)

static long layer_setup_compile_(pcb_qry_exec_t *ectx, layer_setup_t *ls, const char *s_)
{
	const char *val, *lys, *rest;
	layer_setup_loc_t loc;
	layer_setup_target_t target;
	const char *s;
	char tmp[1024];
	int len;
	rnd_bool succ;
	static const pcb_layer_type_t lyt_allow = PCB_LYT_COPPER | PCB_LYT_SILK | PCB_LYT_MASK | PCB_LYT_PASTE;
	pcb_net_t *net;

	LSC_RESET();

	/* above-type:!copper,below-net:gnd;below-netmargin:10mil */
	for(s = s_;;) {
		while(isspace(*s) || (*s == '-')) s++;
		if ((*s == '\0') || (*s == ',') || (*s == ';') || (*s == ':')) {

			if (*s == ':') {
				s++;
				val = s;
				if (target != LST_RESULT) /* result has its own parser, don't change s */
					while((*s != '\0') && (*s != ',') && (*s != ';')) s++;
			}

			/* close current rule */
			switch(target) {
				case LST_RESULT:
					/* parse the result part on the right of ':' */
					rest = s;
					LSC_RESET();
					
					for(;;) {
						while(isspace(*s) || (*s == '-')) s++;
						if ((*s == '\0') || (*s == ',') || (*s == ';'))
							break;
						LSC_PARSE_LOCTARGET(s);
					}
					ls->res_loc = loc;
					ls->res_target = target;
					switch(target) {
						case LST_TYPE: case LST_NET: case LST_NETMARGIN:
							rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid result in '%s'\n", rest);
							return -1;
						default:;
					}
					break;
				case LST_TYPE:
					{
						int invert = 0;
						pcb_layer_type_t lyt;

						LSC_GET_VAL({
							rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid layer type value '%s'\n", val == NULL ? s_ : val);
							return -1;
						});

						lys = tmp;
						if (*lys == '!') {
							invert = 1;
							lys++;
						}
						lyt = pcb_layer_type_str2bit(lys);
						if ((lyt == 0) || ((lyt & lyt_allow) != lyt)) {
							rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid netmargin value '%s'\n", val);
							return -1;
						}
						if (invert)
							ls->refuse_lyt[loc] = lyt;
						else
							ls->require_lyt[loc] = lyt;
					}
					break;

				case LST_NET:
					LSC_GET_VAL({
						rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid net name '%s'\n", val == NULL ? s_ : val);
						return -1;
					});
					net = pcb_net_get(ectx->pcb, &ectx->pcb->netlist[PCB_NETLIST_EDITED], tmp, 0);
					if (net == NULL) {
						rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: net not found: '%s'\n", val);
						return -1;
					}
					ls->require_net[loc] = net;
					break;

				case LST_NETMARGIN:
					LSC_GET_VAL(goto err_coord);
					ls->net_margin[loc] = rnd_get_value(tmp, NULL, NULL, &succ);
					if (!succ) {
						err_coord:;
						rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid netmargin value '%s'\n", val);
						return -1;
					}
					break;
				case LST_NONE:
					rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: missing target in '%s'\n", s_);
					return -1;
				default:
					rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: wrong target for check in '%s'\n", s_);
					return -1;
			}

			/* continue with the next rule */
			LSC_RESET();
			if (*s == '\0')
				break;
			s++;
			continue;
		}

		LSC_PARSE_LOCTARGET(s);

	}

	return 0;
}

/* Cache access wrapper around compile - assume the same string will have the same pointer (e.g. net attribute) */
static const layer_setup_t *layer_setup_compile(pcb_qry_exec_t *ectx, const char *s)
{
	layer_setup_t *ls;
	ls = htpp_get(&ectx->layer_setup_precomp, s);
	if (ls != NULL)
		return ls;

	ls = calloc(sizeof(layer_setup_t), 1);
	if (layer_setup_compile_(ectx, ls, s) == 0)
		ls->valid = 1;
	htpp_set(&ectx->layer_setup_precomp, (void *)s, ls);
	return ls;
}

/* retruns 1 if there is an "adjacent" layer group of lyt in direction dir */
static rnd_bool lse_next_layer_type(pcb_qry_exec_t *ectx, pcb_any_obj_t *obj, pcb_layergrp_t *grp, pcb_layer_type_t lyt, int dir)
{
	pcb_layer_type_t nextloc = (dir == -1) ? PCB_LYT_TOP : PCB_LYT_BOTTOM;
	rnd_layergrp_id_t tmp;

	/* assuming nextloc=top; want to have mask/silk/whatever over us;
	   only top copper/silk/mask/etc will have a chance to be above */
	if ((lyt & (PCB_LYT_MASK | PCB_LYT_PASTE | PCB_LYT_SILK)) && (!(grp->ltype & nextloc)))
			return 0;

	/* assuming nextloc=top; want to have copper over us;
	   only top copper should fail*/
	if ((lyt & PCB_LYT_COPPER) && (grp->ltype & nextloc))
		return 0;

	/* make sure the referenced layers do exist */
	if ((lyt & PCB_LYT_MASK) && (pcb_layergrp_list(ectx->pcb, PCB_LYT_MASK | nextloc, &tmp, 1) < 1))
			return 0;
	if ((lyt & PCB_LYT_SILK) && (pcb_layergrp_list(ectx->pcb, PCB_LYT_SILK | nextloc, &tmp, 1) < 1))
			return 0;
	if ((lyt & PCB_LYT_PASTE) && (pcb_layergrp_list(ectx->pcb, PCB_LYT_PASTE | nextloc, &tmp, 1) < 1))
			return 0;

	return 1;
}

/* append obj to objs if obj is on net */
static void lse_netcover_add_obj(pcb_qry_exec_t *ectx, vtp0_t *objs, pcb_any_obj_t *obj, pcb_net_t *net)
{
	pcb_any_obj_t *term = pcb_qry_parent_net_term(ectx, obj);

	if ((term == NULL) || (term->type != PCB_OBJ_NET_TERM))
		return;
	if (term->parent.net != net)
		return;

	vtp0_append(objs, obj);
}

/* returns 1 if object is fully covered on adjacent copper layer group in direction
   dir by objects of net (obj bloated by bloat first); tolerance for full cover is 0.1 mm^2 */
static int lse_non_covered(pcb_qry_exec_t *ectx, pcb_any_obj_t *obj, rnd_layergrp_id_t ogid, pcb_net_t *net, int dir, rnd_coord_t bloat, double *area_out)
{
	rnd_layergrp_id_t ngid = pcb_layergrp_step(ectx->pcb, ogid, dir, PCB_LYT_COPPER);
	pcb_layergrp_t *grp;
	vtp0_t *objs = &ectx->layer_setup_netobjs;
	rnd_rtree_it_t it;
	pcb_any_obj_t *o;
	rnd_polyarea_t *iceberg, *ptmp;
	static rnd_coord_t zero = 0;
	long n;
	double final_area2;

	if (ngid == -1)
		return 0;

	/* handle only the simple cases for now */
	if ((obj->type != PCB_OBJ_LINE) && (obj->type != PCB_OBJ_ARC) && (obj->type != PCB_OBJ_POLY))
		return 0;

	grp = &ectx->pcb->LayerGroups.grp[ngid];
	objs->used = 0;

	/* gather objects that might be overlapping the target object and are on
	   the target net */
	for(n = 0; n < grp->len; n++) {
		pcb_layer_t *ly = pcb_get_layer(ectx->pcb->Data, grp->lid[n]);
		if (ly->arc_tree != NULL)
			for(o = rnd_rtree_first(&it, ly->arc_tree, (rnd_rtree_box_t *)&obj->bbox_naked); o != NULL; o = rnd_rtree_next(&it))
				lse_netcover_add_obj(ectx, objs, o, net);
		if (ly->line_tree != NULL)
			for(o = rnd_rtree_first(&it, ly->line_tree, (rnd_rtree_box_t *)&obj->bbox_naked); o != NULL; o = rnd_rtree_next(&it))
				lse_netcover_add_obj(ectx, objs, o, net);
		if (ly->polygon_tree != NULL)
			for(o = rnd_rtree_first(&it, ly->polygon_tree, (rnd_rtree_box_t *)&obj->bbox_naked); o != NULL; o = rnd_rtree_next(&it))
				lse_netcover_add_obj(ectx, objs, o, net);
	}

/*rnd_trace("objects on the right net: %d\n", objs->used);*/

	if (objs->used == 0) /* no object found */
		return 0;

	/* create the bloated polygon of the initial object; use the resulting
	   polyarea as the 'iceberg, then...' */
	switch(obj->type) {
		case PCB_OBJ_LINE: iceberg = pcb_poly_from_pcb_line((pcb_line_t *)obj, ((pcb_line_t *)obj)->Thickness + 2*bloat); break;
		case PCB_OBJ_ARC:  iceberg = pcb_poly_from_pcb_arc((pcb_arc_t *)obj, ((pcb_arc_t *)obj)->Thickness + 2*bloat); break;
		case PCB_OBJ_POLY: iceberg = pcb_poly_clearance_construct((pcb_poly_t *)obj, &bloat, NULL); break;
		default: return 0; /* shouldn't get here because input sanity check */
	}

	/* ...subtract each object found from the 'iceberg', melting it down... */
	for(n = 0; n < objs->used; n++) {
		pcb_any_obj_t *o = objs->array[n];
		rnd_polyarea_t *heat;
		switch(o->type) {
			case PCB_OBJ_LINE: heat = pcb_poly_from_pcb_line((pcb_line_t *)o, ((pcb_line_t *)o)->Thickness); break;
			case PCB_OBJ_ARC:  heat = pcb_poly_from_pcb_arc((pcb_arc_t *)o, ((pcb_arc_t *)o)->Thickness); break;
			case PCB_OBJ_POLY: heat = pcb_poly_clearance_construct((pcb_poly_t *)o, &zero, NULL); break;
			default: heat = 0;
		}
		if (heat != 0) {
/*rnd_trace(" sub! %p %$mm\n", iceberg, (rnd_coord_t)sqrt(iceberg->contours->area));*/
			rnd_polyarea_boolean_free(iceberg, heat, &ptmp, RND_PBO_SUB);
			iceberg = ptmp;
			if (iceberg == NULL)
				break;
		}
	}

	if (iceberg != NULL) {
		final_area2 = iceberg->contours->area;
		*area_out = sqrt(final_area2);
		rnd_polyarea_free(&iceberg);
	}
	else
		*area_out = final_area2 = 0;

	/* by now iceberg contains 'exposed' parts, anything not covered by objects
	   found; if it is not zero, the cover was not full */
	return final_area2 < RND_MM_TO_COORD(0.01);
}

/* execute ls on obj, assuming 'above' is in layer stack direction 'above_dir';
   returns true if ls matches obj's current setup */
static rnd_bool layer_setup_exec(pcb_qry_exec_t *ectx, pcb_any_obj_t *obj, const layer_setup_t *ls, const layer_setup_t *ls_res, int above_dir, layer_setup_res_t *lsr)
{
	pcb_layer_t *ly;
	pcb_layergrp_t *grp;
	rnd_layergrp_id_t gid, sgid;
	layer_setup_req_t req;
	int need_set = 0;
	htls_entry_t *he;

	/* look up the result in the cache */
	req.obj = obj;
	req.ls = ls;
	req.above_dir = above_dir;
	he = htls_getentry(ectx->layer_setup_res_cache, req);
#ifdef CACHE_DEBUG
	rnd_trace("cache %s: %p:%p:%d\n", he == NULL ? "miss":"hit ", obj, ls, above_dir);
#endif
	if (he != NULL) { /* cache hit */
		*lsr = he->value;
		if (!he->value.matched) /* mismatch -> quick return */
			return 0;
		goto cache_hit; /* some parts of the result may not have been calculated on the first insertion, check those again */
	}

	/* not cached, need to apply the condition... */
	need_set = 1;
	lsr->matched = 0;
	lsr->uncovered[LSL_ABOVE] = lsr->uncovered[LSL_BELOW] = -1;

	/* only layer objects may match */
	if (obj->parent_type != PCB_PARENT_LAYER)
		return 0;

	ly = pcb_layer_get_real(obj->parent.layer);
	if (ly == NULL)
		return 0;
	gid = pcb_layer_get_group_(ly);
	if (gid == -1)
		return 0;
	grp = &ectx->pcb->LayerGroups.grp[gid];

	/*** find any rule we fail to match and return false ***/

	/* require object's layer type */
	if ((ls->require_lyt[LSL_ON] != 0) && (((ls->require_lyt[LSL_ON] & pcb_layer_flags_(ly)) == 0)))
		return 0;
	if ((ls->refuse_lyt[LSL_ON] != 0) && (((ls->refuse_lyt[LSL_ON] & pcb_layer_flags_(ly)) != 0)))
		return 0;


	/* require above/below layer's type */
	if ((ls->require_lyt[LSL_ABOVE] != 0) && (!lse_next_layer_type(ectx, obj, grp, ls->require_lyt[LSL_ABOVE], above_dir)))
		return 0;
	if ((ls->refuse_lyt[LSL_ABOVE] != 0) && (lse_next_layer_type(ectx, obj, grp, ls->refuse_lyt[LSL_ABOVE], above_dir)))
		return 0;
	if ((ls->require_lyt[LSL_BELOW] != 0) && (!lse_next_layer_type(ectx, obj, grp, ls->require_lyt[LSL_BELOW], -above_dir)))
		return 0;
	if ((ls->refuse_lyt[LSL_BELOW] != 0) && (lse_next_layer_type(ectx, obj, grp, ls->refuse_lyt[LSL_BELOW], -above_dir)))
		return 0;


	/* check for net coverage on adjacent layer */
	if ((ls->require_net[LSL_ABOVE] != NULL) && (!lse_non_covered(ectx, obj, gid, ls->require_net[LSL_ABOVE], above_dir, ls->net_margin[LSL_ABOVE], &lsr->uncovered[LSL_ABOVE])))
		return 0;
	if ((ls->require_net[LSL_BELOW] != NULL) && (!lse_non_covered(ectx, obj, gid, ls->require_net[LSL_BELOW], -above_dir, ls->net_margin[LSL_BELOW], &lsr->uncovered[LSL_BELOW])))
		return 0;

	/* if nothing failed, we have a match */
	lsr->matched = 1;

	/* store above/below substrate */
	sgid = pcb_layergrp_step(ectx->pcb, gid, above_dir, PCB_LYT_SUBSTRATE);
	lsr->substrate[LSL_ABOVE] = (sgid != -1) ? &ectx->pcb->LayerGroups.grp[sgid] : NULL;
	sgid = pcb_layergrp_step(ectx->pcb, gid, -above_dir, PCB_LYT_SUBSTRATE);
	lsr->substrate[LSL_BELOW] = (sgid != -1) ? &ectx->pcb->LayerGroups.grp[sgid] : NULL;

	cache_hit:;

	/* calculate the return value; use ->res_* fields from ls_res */
	/* if condition didn't calculate the result, do it now */
	if ((ls_res->res_target == LST_UNCOVERED) && ((ls_res->res_loc == LSL_BELOW) || (ls_res->res_loc == LSL_ABOVE))) {
		if (lsr->uncovered[ls_res->res_loc] < 0) {
			int dir = (ls_res->res_loc == LSL_ABOVE) ? above_dir : -above_dir;
			lse_non_covered(ectx, obj, gid, ls->require_net[ls_res->res_loc], dir, ls->net_margin[ls_res->res_loc], &lsr->uncovered[ls_res->res_loc]);
			need_set = 1;
		}
	}

	if (need_set) /* not cached yet, need to remember this result */
		htls_set(ectx->layer_setup_res_cache, req, *lsr);

	return 1;
}

static int fnc_layer_setup(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	pcb_any_obj_t *obj;
	const char *lss, *lss_res = NULL;
	const layer_setup_t *ls, *ls_res = NULL;
	layer_setup_res_t lsr;

	if ((argc < 2) || (argc > 3) || (argv[0].type != PCBQ_VT_OBJ) || (argv[1].type != PCBQ_VT_STRING))
		return -1;

	if (argc == 3) {
		if (argv[2].type != PCBQ_VT_STRING)
			return -1;
		lss_res = argv[2].data.str;
	}

	lss = argv[1].data.str;

	if (!ectx->layer_setup_inited)
		pcb_qry_init_layer_setup(ectx);

	obj = (pcb_any_obj_t *)argv[0].data.obj;
	ls = layer_setup_compile(ectx, lss);

	if (!ls->valid)
		return -1;

	if (lss_res != NULL) {
		ls_res = layer_setup_compile(ectx, lss_res);
		if (!ls->valid)
			return -1;
	}
	else
		ls_res = ls;

/*rnd_trace("layer_setup: %p/'%s' -> %p (ls_res=%p)\n", lss, lss, ls, ls_res);*/

	/* try above=-1, below=+1 (directions matching normal layer stack ordering) */
	if (layer_setup_exec(ectx, obj, ls, ls_res, -1, &lsr))
		goto done;

	/* try above=+1, below=-1 (opposite directions to normal layer stack ordering)  */
	if (layer_setup_exec(ectx, obj, ls, ls_res, 1, &lsr))
		goto done;

	/* both failed -> this layer setup is invalid in any direction */

	/* generate the result according to ls_res */
	done:;
	switch(ls_res->res_target) {
		case LST_NONE:
			PCB_QRY_RET_INT(res, lsr.matched);

		case LST_UNCOVERED:
			if (!lsr.matched) PCB_QRY_RET_INV(res);
			PCB_QRY_RET_DBL(res, lsr.uncovered[ls_res->res_loc]);

		case LST_SUBSTRATE:
			if (!lsr.matched) PCB_QRY_RET_INV(res);
			PCB_QRY_RET_OBJ(res, (pcb_any_obj_t *)lsr.substrate[ls_res->res_loc]);

		case LST_TYPE:
		case LST_NET:
		case LST_NETMARGIN:
		case LST_RESULT:
			PCB_QRY_RET_INV(res);
	}

	return -1;
}


/*** ko match ***/

#include "htko.h"

void pcb_qry_uninit_layer_ko_match(pcb_qry_exec_t *ectx)
{
	if (!ectx->layer_ko_match_inited)
		return;

	htko_free((htko_t *)ectx->layer_ko_match);
	ectx->layer_ko_match_inited = 0;
}

static void pcb_qry_init_layer_ko_match(pcb_qry_exec_t *ectx)
{
	ectx->layer_ko_match = htko_alloc(htko_keyhash, htko_keyeq);
	ectx->layer_ko_match_inited = 1;
}


RND_INLINE int ko_match(pcb_layer_t *ly, pcb_layer_t *koly)
{
	const char *koprp = NULL, *stype;
	pcb_layer_type_t loc, typ, flg;

	pcb_layer_purpose_(koly, &koprp);
	if ((koprp == NULL) || (koprp[0] != 'k') || (koprp[1] != 'o') || (koprp[2] != '@'))
		return 0;

	koprp += 3;

	if ((*koprp == 't') && (strncmp(koprp, "top-", 4) == 0)) {
		stype = koprp+4;
		loc = PCB_LYT_TOP;
	}
	else if ((*koprp == 'b') && (strncmp(koprp, "bottom-", 7) == 0)) {
		stype = koprp+7;
		loc = PCB_LYT_BOTTOM;
	}
	else if ((*koprp == 'i') && (strncmp(koprp, "intern-", 7) == 0)) {
		stype = koprp+7;
		loc = PCB_LYT_INTERN;
	}
	else if ((*koprp == 'l') && (strncmp(koprp, "logical-", 8) == 0)) {
		stype = koprp+8;
		loc = PCB_LYT_LOGICAL;
	}
	else if ((*koprp == 'a') && (strncmp(koprp, "any-", 4) == 0)) {
		stype = koprp+4;
		loc = PCB_LYT_ANYWHERE;
	}
	else {
		rnd_message(RND_MSG_ERROR, "query layer_ko_match(): invalid ko@ layer type: '%s' (invalid location before the dash)\n", koprp);
		return 0;
	}

	if ((*stype == 'a') && (strcmp(stype, "any") == 0))
		typ = PCB_LYT_ANYTHING;
	else
		typ = pcb_layer_type_str2bit(stype);

	if (typ == 0) {
		rnd_message(RND_MSG_ERROR, "query layer_ko_match(): invalid ko@ layer type: '%s' (invalid type after the dash)\n", koprp);
		return 0;
	}

	flg = pcb_layer_flags_(ly);
	return (flg & loc) && (flg & typ);
}

static int fnc_layer_ko_match(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	pcb_any_obj_t *obj, *koobj;
	pcb_layer_t *ly, *koly;
	rnd_layer_id_t lid, kolid;
	htko_key_t key;
	htko_entry_t *e;
	int m;

	if ((argc != 2) || (argv[0].type != PCBQ_VT_OBJ) || (argv[1].type != PCBQ_VT_OBJ))
		return -1;

	/* init cache on-demand */
	if (!ectx->layer_ko_match_inited)
		pcb_qry_init_layer_ko_match(ectx);

	/* figure the two layers */
	obj = (pcb_any_obj_t *)argv[0].data.obj;
	koobj = (pcb_any_obj_t *)argv[1].data.obj;
	ly = pcb_layer_get_real(obj->parent.layer);
	koly = pcb_layer_get_real(koobj->parent.layer);

	lid = pcb_layer2id(ectx->pcb->Data, ly);
	kolid = pcb_layer2id(ectx->pcb->Data, koly);

	if ((lid == -1) || (kolid == -1))
		PCB_QRY_RET_INT(res, 0); /* invalid layers can't match */

	/* look up lid-kolid pair in cache */
	key = htko_mkkey(lid, kolid);
	e = htko_getentry((htko_t *)ectx->layer_ko_match, key);
	if (e != NULL) {
/*		rnd_trace("KO cache HIT %ld-%ld: %d\n", lid, kolid, e->value);*/
		PCB_QRY_RET_INT(res, e->value);
	}

	/* not in cache: compute and remember */
	m = ko_match(ly, koly);
/*	rnd_trace("KO cache set %ld-%ld: %d\n", lid, kolid, m);*/
	htko_insert((htko_t *)ectx->layer_ko_match, key, m);

	PCB_QRY_RET_INT(res, m);
}
