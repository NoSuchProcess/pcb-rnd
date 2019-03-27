/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "actions.h"
#include "board.h"
#include "data.h"
#include "compat_misc.h"
#include "layer.h"
#include "pcb-printf.h"
#include "conf_core.h"

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

static int data_arg_conv(fgw_ctx_t *ctx, fgw_arg_t *arg, fgw_type_t target)
{
	if (target == FGW_DATA) { /* convert to data */
		if (FGW_BASE_TYPE(arg->type) == FGW_STR) {
			if (pcb_strcasecmp(arg->val.str, "pcb") == 0) {
				arg->val.ptr_void = PCB->Data;
				arg->type = FGW_DATA;
				return 0;
			}
			else if (pcb_strncasecmp(arg->val.str, "buffer#", 7) == 0) {
				char *end;
				long idx = strtol(arg->val.str+7, &end, 10);
				if ((*end == '\0') && (idx >= 0) && (idx < PCB_MAX_BUFFER)) {
					arg->val.ptr_void = pcb_buffers[idx].Data;
					arg->type = FGW_DATA;
					return 0;
				}
			}
			else if (pcb_strcasecmp(arg->val.str, "buffer") == 0) {
				arg->val.ptr_void = PCB_PASTEBUFFER->Data;
				arg->type = FGW_DATA;
				return 0;
			}
		}
		arg->type = FGW_INVALID;
		return -1;
	}
	if (arg->type == FGW_DATA) { /* convert from layer */
		int n;
		if (arg->val.ptr_void == PCB->Data) {
			arg->val.str = "pcb";
			arg->type = FGW_STR;
			return 0;
		}
		for(n = 0; n < PCB_MAX_BUFFER; n++) {
			if (arg->val.ptr_void == pcb_buffers[n].Data) {
				arg->val.str = pcb_strdup_printf("buffer#%d", n);
				arg->type = FGW_STR | FGW_DYN;
				return 0;
			}
		}
		arg->type = FGW_INVALID;
		return -1;
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
	if (fgw_reg_custom_type(&pcb_fgw, FGW_DATA, "data", data_arg_conv, NULL) != FGW_DATA) {
		fprintf(stderr, "pcb_actions_init: failed to register FGW_DATA\n");
		abort();
	}
}
