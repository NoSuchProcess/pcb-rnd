/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
typedef enum { NONE = 0x1000, TYPE, NET, NETMARGIN } layer_setup_target_t;

typedef struct {
	/* for a composite check */
	pcb_layer_type_t require_lyt[LSL_max];
	pcb_layer_type_t refuse_lyt[LSL_max];
	pcb_net_t *require_net[LSL_max];
	rnd_coord_t net_margin[LSL_max];

	/* for an atomic query: loc|target */
	unsigned long loc_target;
} layer_setup_t;

void pcb_qry_uninit_layer_setup(pcb_qry_exec_t *ectx)
{
	if (!ectx->layer_setup_inited)
		return;

	genht_uninit_deep(htpp, &ectx->layer_setup_precomp, {
		free(htent->value);
	});

	vtp0_uninit(&ectx->layer_setup_netobjs);
	ectx->layer_setup_inited = 0;
}

static void pcb_qry_init_layer_setup(pcb_qry_exec_t *ectx)
{
	htpp_init(&ectx->layer_setup_precomp, ptrhash, ptrkeyeq);
	ectx->layer_setup_inited = 1;
	vtp0_init(&ectx->layer_setup_netobjs);
}

#define LSC_RESET() \
do { \
	val = NULL; \
	loc = LSL_ON; \
	target = NONE; \
} while(0)

#define LSC_GET_VAL(err_stmt) \
do { \
	len = s-val; \
	if ((val == NULL) || (len >= sizeof(tmp))) { err_stmt; } \
	strncpy(tmp, val, len); \
	tmp[len] = '\0'; \
} while(0)

static long layer_setup_compile_(pcb_qry_exec_t *ectx, layer_setup_t *ls, const char *s_, rnd_bool composite)
{
	const char *val, *lys;
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
			ls->loc_target = loc | target;

			if (*s == ':') {
				s++;
				val = s;
				while((*s != '\0') && (*s != ',') && (*s != ';')) s++;
			}

			if (!composite)
				return 0;

			/* close current rule */
			switch(target) {
				case TYPE:
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

				case NET:
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

				case NETMARGIN:
					LSC_GET_VAL(goto err_coord);
					ls->net_margin[loc] = rnd_get_value(tmp, NULL, NULL, &succ);
					if (!succ) {
						err_coord:;
						rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid netmargin value '%s'\n", val);
						return -1;
					}
					break;
				case NONE:
					rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: missing target in '%s'\n", s_);
					return -1;
			}

			/* continue with the next rule */
			LSC_RESET();
			if (*s == '\0')
				break;
			s++;
			continue;
		}



		if (strncmp(s, "above", 5) == 0)           { s += 5; loc = LSL_ABOVE; }
		else if (strncmp(s, "below", 5) == 0)      { s += 5; loc = LSL_BELOW; }
		else if (strncmp(s, "netmargin", 9) == 0)  { s += 9; target = NETMARGIN; }
		else if (strncmp(s, "type", 4) == 0)       { s += 4; target = TYPE; }
		else if (strncmp(s, "net", 3) == 0)        { s += 3; target = NET; }
		else if (strncmp(s, "on", 2) == 0)         { s += 2; loc = LSL_ON; }
		else {
			rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid target or location in '%s'\n", s);
			return -1;
		}
	}

	return 0;
}

/* Cache access wrapper around compile - assume the same string will have the same pointer (e.g. net attribute) */
static const layer_setup_t *layer_setup_compile(pcb_qry_exec_t *ectx, const char *s, rnd_bool composite)
{
	layer_setup_t *ls;
	ls = htpp_get(&ectx->layer_setup_precomp, s);
	if (ls != NULL)
		return ls;

	ls = calloc(sizeof(layer_setup_t), 1);
	layer_setup_compile_(ectx, ls, s, composite);
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

/* retruns 1 if the adjacent copper layer group in direction dir has objects
   of net fully covering obj bloated by bloat */
static rnd_bool lse_fully_covered(pcb_qry_exec_t *ectx, pcb_any_obj_t *obj, rnd_layergrp_id_t ogid, pcb_net_t *net, int dir, rnd_coord_t bloat)
{
	rnd_layergrp_id_t ngid = pcb_layergrp_step(ectx->pcb, ogid, dir, PCB_LYT_COPPER);
	pcb_layergrp_t *grp;
	vtp0_t *objs = &ectx->layer_setup_netobjs;
	rnd_rtree_it_t it;
	pcb_any_obj_t *o;
	rnd_polyarea_t *iceberg, *ptmp;
	rnd_bool dummy1;
	static rnd_coord_t zero = 0;
	long n;
	int res;

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

rnd_trace("objects on the right net: %d\n", objs->used);

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
rnd_trace(" sub! %p %$mm\n", iceberg, (rnd_coord_t)sqrt(iceberg->contours->area));
			rnd_polyarea_boolean_free(iceberg, heat, &ptmp, RND_PBO_SUB);
			iceberg = ptmp;
		}
	}

rnd_trace(" res=%$mm\n", (rnd_coord_t)sqrt(iceberg->contours->area));

	rnd_polyarea_free(&iceberg);

	/* by now iceberg contains 'exposed' parts, anything not covered by objects
	   found; if it is not empty, the cover was not full */
	return 1;
}

/* execute ls on obj, assuming 'above' is in layer stack direction 'above_dir';
   returns true if ls matches obj's current setup */
static rnd_bool layer_setup_exec(pcb_qry_exec_t *ectx, pcb_any_obj_t *obj, const layer_setup_t *ls, int above_dir)
{
	pcb_layer_t *ly;
	pcb_layergrp_t *grp;
	rnd_layergrp_id_t gid;

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
	if ((ls->require_net[LSL_ABOVE] != NULL) && (!lse_fully_covered(ectx, obj, gid, ls->require_net[LSL_ABOVE], above_dir, ls->net_margin[LSL_ABOVE])))
		return 0;
	if ((ls->require_net[LSL_BELOW] != NULL) && (!lse_fully_covered(ectx, obj, gid, ls->require_net[LSL_BELOW], -above_dir, ls->net_margin[LSL_BELOW])))
		return 0;

	/* if nothing failed, we have a match */
	return 1;
}

static int fnc_layer_setup(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	pcb_any_obj_t *obj;
	const char *lss;
	const layer_setup_t *ls;

	if ((argc != 2) || (argv[0].type != PCBQ_VT_OBJ) || (argv[1].type != PCBQ_VT_STRING))
		return -1;

	lss = argv[1].data.str;

	if (!ectx->layer_setup_inited)
		pcb_qry_init_layer_setup(ectx);

	obj = (pcb_any_obj_t *)argv[0].data.obj;
	ls = layer_setup_compile(ectx, lss, 1);

rnd_trace("layer_setup: %p/'%s' -> %p\n", lss, lss, ls);

TODO("cache result: obj-ls -> direction (+1 or -1) where it was true or 0 if it was false");

	/* try above=-1, below=+1 (directions matching normal layer stack ordering) */
	if (layer_setup_exec(ectx, obj, ls, -1))
		PCB_QRY_RET_INT(res, 1);

	/* try above=+1, below=-1 (opposite directions to normal layer stack ordering)  */
	if (layer_setup_exec(ectx, obj, ls, 1))
		PCB_QRY_RET_INT(res, 1);

	/* both failed -> this layer setup is invalid in any direction */
	PCB_QRY_RET_INT(res, 0);
}

