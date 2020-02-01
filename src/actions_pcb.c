/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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

/* fungw type extensions for pcb-rnd-only actions (not part of the hidlib) */

#include "config.h"

#include <stdlib.h>

#include <libfungw/fungw_conv.h>

#include <librnd/core/actions.h>
#include "board.h"
#include "data.h"
#include <librnd/core/compat_misc.h>
#include "layer.h"
#include "layer_addr.h"
#include "undo.h"
#include "change.h"
#include <librnd/core/pcb-printf.h>
#include "conf_core.h"

const char *PCB_PTR_DOMAIN_LAYER = "pcb_fgw_ptr_domain_layer";
const char *PCB_PTR_DOMAIN_LAYERGRP = "pcb_fgw_ptr_domain_layergrp";


#define conv_str2layerid(dst, src) \
do { \
	pcb_layer_id_t lid = pcb_layer_str2id(PCB->Data, src); \
	if (lid < 0) \
		return -1; \
	dst = lid; \
} while(0)

static int layerid_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (target == FGW_LAYERID) { /* convert to layer id */
		pcb_layer_id_t tmp;
		switch(FGW_BASE_TYPE(arg->type)) {
			ARG_CONV_CASE_LONG(tmp, conv_assign)
			ARG_CONV_CASE_LLONG(tmp, conv_assign)
			ARG_CONV_CASE_DOUBLE(tmp, conv_assign)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_assign)
			ARG_CONV_CASE_STR(tmp, conv_str2layerid)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
		}
		arg->type = FGW_LAYERID;
		fgw_layerid(arg) = tmp;
		return 0;
	}
	if (arg->type == FGW_LAYERID) { /* convert from layer id */
		pcb_layer_id_t tmp = fgw_layerid(arg);
		switch(target) {
			ARG_CONV_CASE_LONG(tmp, conv_rev_assign)
			ARG_CONV_CASE_LLONG(tmp, conv_rev_assign)
			ARG_CONV_CASE_DOUBLE(tmp, conv_rev_assign)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_rev_assign)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
			case FGW_STR:
				arg->val.str = (char *)pcb_strdup_printf("#%ld", (long)tmp);
				arg->type = FGW_STR | FGW_DYN;
				return 0;
		}
		arg->type = target;
		return 0;
	}
	fprintf(stderr, "Neither side of the conversion is layer id\n");
	abort();
}

static int layer_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (target == FGW_LAYER) { /* convert to layer */
		pcb_layer_id_t lid;
		if (layerid_arg_conv(ctx, arg, FGW_LAYERID) != 0)
			return -1;
		lid = fgw_layerid(arg);
		arg->val.ptr_void = pcb_get_layer(PCB->Data, lid);
		if (arg->val.ptr_void == NULL) {
			arg->type = FGW_INVALID;
			return -1;
		}
		arg->type = FGW_LAYER;
		return 0;
	}
	if (arg->type == FGW_LAYER) { /* convert from layer */
		pcb_layer_id_t lid;
		pcb_layer_t *ly = arg->val.ptr_void;
		pcb_data_t *data;
		if (ly == NULL)
			return -1;
		data = ly->parent.data;
		lid = ly - data->Layer;
		if ((lid >= 0) && (lid < data->LayerN)) {
			arg->type = FGW_LAYERID;
			arg->val.nat_long = lid;
			if (layerid_arg_conv(ctx, arg, target) != 0)
				return -1;
			return 0;
		}
		return -1;
	}
	fprintf(stderr, "Neither side of the conversion is layer\n");
	abort();
}

#define conv_str2layergrpid(dst, src) \
do { \
	pcb_layergrp_id_t gid = pcb_layergrp_str2id(PCB, src); \
	if (gid < 0) \
		return -1; \
	dst = gid; \
} while(0)

static int layergrpid_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (target == FGW_LAYERGRPID) { /* convert to layergrp id */
		pcb_layergrp_id_t tmp;
		switch(FGW_BASE_TYPE(arg->type)) {
			ARG_CONV_CASE_LONG(tmp, conv_assign)
			ARG_CONV_CASE_LLONG(tmp, conv_assign)
			ARG_CONV_CASE_DOUBLE(tmp, conv_assign)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_assign)
			ARG_CONV_CASE_STR(tmp, conv_str2layergrpid)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
		}
		arg->type = FGW_LAYERGRPID;
		fgw_layergrpid(arg) = tmp;
		return 0;
	}
	if (arg->type == FGW_LAYERGRPID) { /* convert from layergrp id */
		pcb_layergrp_id_t tmp = fgw_layergrpid(arg);
		switch(target) {
			ARG_CONV_CASE_LONG(tmp, conv_rev_assign)
			ARG_CONV_CASE_LLONG(tmp, conv_rev_assign)
			ARG_CONV_CASE_DOUBLE(tmp, conv_rev_assign)
			ARG_CONV_CASE_LDOUBLE(tmp, conv_rev_assign)
			ARG_CONV_CASE_PTR(tmp, conv_err)
			ARG_CONV_CASE_CLASS(tmp, conv_err)
			ARG_CONV_CASE_INVALID(tmp, conv_err)
			case FGW_STR:
				arg->val.str = (char *)pcb_strdup_printf("#%ld", (long)tmp);
				arg->type = FGW_STR | FGW_DYN;
				return 0;
		}
		arg->type = target;
		return 0;
	}
	fprintf(stderr, "Neither side of the conversion is layer group id\n");
	abort();
}

static int layergrp_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (target == FGW_LAYERGRP) { /* convert to layer group */
		pcb_layergrp_id_t gid;
		if (layergrpid_arg_conv(ctx, arg, FGW_LAYERGRPID) != 0)
			return -1;
		gid = fgw_layergrpid(arg);
		arg->val.ptr_void = pcb_get_layergrp(PCB, gid);
		if (arg->val.ptr_void == NULL) {
			arg->type = FGW_INVALID;
			return -1;
		}
		arg->type = FGW_LAYERGRP;
		return 0;
	}
	if (arg->type == FGW_LAYERGRP) { /* convert from layer group */
		pcb_layer_id_t gid;
		pcb_layergrp_t *grp = arg->val.ptr_void;
		pcb_board_t *pcb;
		if (grp == NULL)
			return -1;
		pcb = grp->parent.board;
		gid = grp - pcb->LayerGroups.grp;
		if ((gid >= 0) && (gid < pcb->LayerGroups.len)) {
			arg->type = FGW_LAYERGRPID;
			arg->val.nat_long = gid;
			if (layergrpid_arg_conv(ctx, arg, target) != 0)
				return -1;
			return 0;
		}
		return -1;
	}
	fprintf(stderr, "Neither side of the conversion is layer group\n");
	abort();
}

static int data_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (target == FGW_DATA) { /* convert to data */
		if (FGW_BASE_TYPE(arg->type) == FGW_STR) {
			const char *str = arg->val.str;
			pcb_data_t *data = pcb_data_by_name(PCB, &str);
			if (data != NULL) {
				arg->val.ptr_void = data;
				arg->type = FGW_DATA;
				return 0;
			}
		}
		if (FGW_BASE_TYPE(arg->type) == (FGW_PTR | FGW_STRUCT)) {
TODO("Dangerous: rather support subc's data with IDPATH");
			arg->val.ptr_void = arg->val.ptr_void;
			arg->type = FGW_DATA;
			return 0;
		}
		arg->type = FGW_INVALID;
		return -1;
	}
	if (arg->type == FGW_DATA) { /* convert from data */
		char buf[32];
		const char *name = pcb_data_to_name(PCB, arg->val.ptr_void, buf, sizeof(buf));

		if (name != NULL) {
			if (name == buf) {
				arg->val.str = pcb_strdup(name);
				arg->type = FGW_STR | FGW_DYN;
			}
			else { /* static string */
				arg->val.cstr = name;
				arg->type = FGW_STR;
			}
			return 0;
		}

TODO("Dangerous: rather support subc's data with IDPATH");
		arg->type = FGW_PTR | FGW_DYN;
		return 0;
	}
	fprintf(stderr, "Neither side of the conversion is data\n");
	abort();
}

void pcb_actions_init_pcb_only(void)
{
	if (fgw_reg_custom_type(&pcb_fgw, FGW_LAYERID, "layerid", layerid_arg_conv, NULL) != FGW_LAYERID) {
		fprintf(stderr, "pcb_actions_init: failed to register FGW_LAYERID\n");
		abort();
	}
	if (fgw_reg_custom_type(&pcb_fgw, FGW_LAYER, "layer", layer_arg_conv, NULL) != FGW_LAYER) {
		fprintf(stderr, "pcb_actions_init: failed to register FGW_LAYER\n");
		abort();
	}
	if (fgw_reg_custom_type(&pcb_fgw, FGW_LAYERGRPID, "layergrpid", layergrpid_arg_conv, NULL) != FGW_LAYERGRPID) {
		fprintf(stderr, "pcb_actions_init: failed to register FGW_LAYERGRPID\n");
		abort();
	}
	if (fgw_reg_custom_type(&pcb_fgw, FGW_LAYERGRP, "layergrp", layergrp_arg_conv, NULL) != FGW_LAYERGRP) {
		fprintf(stderr, "pcb_actions_init: failed to register FGW_LAYERGRP\n");
		abort();
	}
	if (fgw_reg_custom_type(&pcb_fgw, FGW_DATA, "data", data_arg_conv, NULL) != FGW_DATA) {
		fprintf(stderr, "pcb_actions_init: failed to register FGW_DATA\n");
		abort();
	}
}

int pcb_act_execute_file(pcb_hidlib_t *hidlib, const char *fn)
{
	int res;

	defer_updates = 1;
	defer_needs_update = 0;

	res = rnd_act_execute_file(hidlib, fn);

	defer_updates = 0;
	if (defer_needs_update) {
		pcb_undo_inc_serial();
		pcb_gui->invalidate_all(pcb_gui);
	}

	return res;
}
