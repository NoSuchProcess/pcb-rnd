/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

/* included from act_draw.c */

#include "obj_pstk_inlines.h"

static const char *PCB_PTR_DOMAIN_PSTK_RPOTO = "fgw_ptr_domain_pstk_proto";

static const char pcb_acts_PstkProtoTmp[] =
	"PstkProto([noundo,] new)\n"
	"PstkProto([noundo,] dup, data, src_proto_id)\n"
	"PstkProto([noundo,] insert, data, proto)\n"
	"PstkProto([noundo,] insert_dup, data, proto)\n"
	"PstkProto([noundo,] free, proto)";
static const char pcb_acth_PstkProtoTmp[] = "Allocate, insert or free a temporary padstack prototype";
static fgw_error_t pcb_act_PstkProtoTmp(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *cmd;
	pcb_pstk_proto_t *proto, *src;
	pcb_data_t *data;
	long src_id;
	DRAWOPTARG;

	res->type = FGW_PTR | FGW_STRUCT;
	res->val.ptr_void = NULL;

	PCB_ACT_CONVARG(1+ao, FGW_STR, PstkProtoTmp, cmd = argv[1+ao].val.str);

	switch(act_draw_keywords_sphash(cmd)) {
		case act_draw_keywords_new:
			proto = calloc(sizeof(pcb_pstk_proto_t), 1);
			fgw_ptr_reg(&pcb_fgw, res, PCB_PTR_DOMAIN_PSTK_RPOTO, FGW_PTR | FGW_STRUCT, proto);
			res->val.ptr_void = proto;
			return 0;

		case act_draw_keywords_dup:
			PCB_ACT_CONVARG(2+ao, FGW_DATA, PstkProtoTmp, data = fgw_data(&argv[2+ao]));
			PCB_ACT_CONVARG(3+ao, FGW_LONG, PstkProtoTmp, src_id = argv[3+ao].val.nat_long);
			if (data == NULL)
				return 0;
			src = pcb_pstk_get_proto_(data, src_id);
			if (src == NULL)
				return 0;
			proto = calloc(sizeof(pcb_pstk_proto_t), 1);
			fgw_ptr_reg(&pcb_fgw, res, PCB_PTR_DOMAIN_PSTK_RPOTO, FGW_PTR | FGW_STRUCT, proto);
			pcb_pstk_proto_copy(proto, src);
			res->val.ptr_void = proto;
			return 0;

		case act_draw_keywords_insert:
		case act_draw_keywords_insert_dup:
		case act_draw_keywords_free:
		case act_draw_keywords_SPHASH_INVALID:
			break;
	}

	return 0;
}
