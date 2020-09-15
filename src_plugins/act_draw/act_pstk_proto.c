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

/* included from act_draw.c */

#include "obj_pstk_inlines.h"

static const char *PCB_PTR_DOMAIN_PSTK_PROTO = "fgw_ptr_domain_pstk_proto";

static const char pcb_acts_PstkProtoTmp[] =
	"PstkProto([noundo,] new)\n"
	"PstkProto([noundo,] dup, idpath)\n"
	"PstkProto([noundo,] dup, data, src_proto_id)\n"
	"PstkProto([noundo,] insert, idpath|data, proto)\n"
	"PstkProto([noundo,] insert_dup, idpath|data, proto)\n"
	"PstkProto([noundo,] free, proto)";
static const char pcb_acth_PstkProtoTmp[] = "Allocate, insert or free a temporary padstack prototype";

static int get_obj_and_data_from_idp(int argc, fgw_arg_t *argv, int aidx, pcb_any_obj_t **obj_out, pcb_data_t **data_out)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	pcb_any_obj_t *obj;
	pcb_idpath_t *idp;

	if (((argv[aidx].type & FGW_STR) == FGW_STR) && (argv[aidx].val.str[0] == 'b') && ((argv[aidx].val.str[1] == 'u') || (argv[aidx].val.str[1] == 'o'))) {
		RND_ACT_CONVARG(aidx, FGW_DATA, PstkProtoTmp, *data_out = fgw_data(&argv[aidx]));
		*obj_out = NULL;
		return 0;
	}

	RND_ACT_CONVARG(aidx, FGW_IDPATH, PstkProtoTmp, idp = fgw_idpath(&argv[aidx]));
	if ((idp == NULL) || !fgw_ptr_in_domain(&rnd_fgw, &argv[aidx], RND_PTR_DOMAIN_IDPATH))
		return FGW_ERR_PTR_DOMAIN;
	obj = pcb_idpath2obj(pcb, idp);
	if ((obj == NULL) || (obj->type != PCB_OBJ_PSTK) || (obj->parent_type != PCB_PARENT_DATA))
		return -1;
	*obj_out = obj;
	*data_out = obj->parent.data;
	return 0;
}

static fgw_error_t pcb_act_PstkProtoTmp(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *cmd;
	int cmdi;
	pcb_pstk_proto_t *proto, *src;
	pcb_data_t *data;
	pcb_any_obj_t *obj;
	long src_id;
	DRAWOPTARG;

	res->type = FGW_PTR | FGW_STRUCT;
	res->val.ptr_void = NULL;

	RND_ACT_CONVARG(1+ao, FGW_STR, PstkProtoTmp, cmd = argv[1+ao].val.str);

	cmdi = act_draw_keywords_sphash(cmd);
	switch(cmdi) {
		case act_draw_keywords_new:
			proto = calloc(sizeof(pcb_pstk_proto_t), 1);
			fgw_ptr_reg(&rnd_fgw, res, PCB_PTR_DOMAIN_PSTK_PROTO, FGW_PTR | FGW_STRUCT, proto);
			res->val.ptr_void = proto;
			pcb_vtpadstack_tshape_init(&proto->tr);
			pcb_vtpadstack_tshape_alloc_append(&proto->tr, 1);
			return 0;

		case act_draw_keywords_dup:
			if (argc < 4) {
				if (get_obj_and_data_from_idp(argc, argv, 2+ao, &obj, &data) != 0) {
					RND_ACT_IRES(-1);
					return 0;
				}
				src = pcb_pstk_get_proto((pcb_pstk_t *)obj);
			}
			else {
				RND_ACT_CONVARG(2+ao, FGW_DATA, PstkProtoTmp, data = fgw_data(&argv[2+ao]));
				RND_ACT_CONVARG(3+ao, FGW_LONG, PstkProtoTmp, src_id = argv[3+ao].val.nat_long);
				if (data == NULL)
					return 0;
				src = pcb_pstk_get_proto_(data, src_id);
			}
			if (src == NULL)
				return 0;
			proto = calloc(sizeof(pcb_pstk_proto_t), 1);
			fgw_ptr_reg(&rnd_fgw, res, PCB_PTR_DOMAIN_PSTK_PROTO, FGW_PTR | FGW_STRUCT, proto);
			pcb_pstk_proto_copy(proto, src);
			res->val.ptr_void = proto;
			return 0;

		case act_draw_keywords_insert:
		case act_draw_keywords_insert_dup:
			if (argc < 3)
				RND_ACT_FAIL(PstkProtoTmp);
			if (get_obj_and_data_from_idp(argc, argv, 2+ao, &obj, &data) != 0) {
				RND_ACT_IRES(-1);
				return 0;
			}
			RND_ACT_CONVARG(3+ao, FGW_PTR, PstkProtoTmp, proto = argv[3+ao].val.ptr_void);
			if (!fgw_ptr_in_domain(&rnd_fgw, &argv[3+ao], PCB_PTR_DOMAIN_PSTK_PROTO) || (proto == NULL)) {
				rnd_message(RND_MSG_ERROR, "PstkProtoTmp: invalid proto pointer\n");
				RND_ACT_IRES(-1);
				return 0;
			}
			res->type = FGW_LONG;
			if (cmdi == act_draw_keywords_insert_dup)
				res->val.nat_long = pcb_pstk_proto_insert_forcedup(data, proto, 1, !noundo);
			else
				res->val.nat_long = pcb_pstk_proto_insert_dup(data, proto, 1, !noundo);
			return 0;

		case act_draw_keywords_free:
			TODO("implement this");

		case act_draw_keywords_SPHASH_INVALID:
		default:
			return FGW_ERR_ARG_CONV;
	}

	return 0;
}

static const char pcb_acts_PstkProtoEdit[] =
	"PstkProto([noundo,] proto, remove, layer_type)\n"
	"PstkProto([noundo,] proto, copy, dst_layer_type, src_layer_type)\n"
	"PstkProto([noundo,] proto, hdia, dia)\n"
	"PstkProto([noundo,] proto, shape:line, layer_type, x1, y1, x2, y2, th, [square])\n"
	;
static const char pcb_acth_PstkProtoEdit[] = "Edit a padstack prototype specified by its pointer.";
static fgw_error_t pcb_act_PstkProtoEdit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *tmp;
	pcb_pstk_proto_t *proto;
	pcb_layer_type_t slyt, dlyt;
	pcb_layer_combining_t slyc, dlyc;
	int src_idx, dst_idx, n;
	pcb_pstk_tshape_t *ts;
	rnd_coord_t crd;
	DRAWOPTARG;

	RND_ACT_CONVARG(1+ao, FGW_PTR, PstkProtoEdit, proto = argv[1+ao].val.ptr_void);
	RND_ACT_CONVARG(2+ao, FGW_STR, PstkProtoEdit, cmd = argv[2+ao].val.str);

	if (!fgw_ptr_in_domain(&rnd_fgw, &argv[1+ao], PCB_PTR_DOMAIN_PSTK_PROTO) || (proto == NULL)) {
		rnd_message(RND_MSG_ERROR, "PstkProtoEdit: invalid proto pointer\n");
		RND_ACT_IRES(-1);
		return 0;
	}
	ts = &proto->tr.array[0];

	switch(act_draw_keywords_sphash(cmd)) {
		case act_draw_keywords_remove:
			RND_ACT_CONVARG(3+ao, FGW_STR, PstkProtoEdit, tmp = argv[3+ao].val.str);
			if (pcb_layer_typecomb_str2bits(tmp, &dlyt, &dlyc, 1) != 0)
				return FGW_ERR_ARG_CONV;
			pcb_pstk_proto_del_shape(proto, dlyt, dlyc);
			break;

		case act_draw_keywords_copy:
			RND_ACT_CONVARG(3+ao, FGW_STR, PstkProtoEdit, tmp = argv[3+ao].val.str);
			if (pcb_layer_typecomb_str2bits(tmp, &dlyt, &dlyc, 1) != 0)
				return FGW_ERR_ARG_CONV;
			RND_ACT_CONVARG(4+ao, FGW_STR, PstkProtoEdit, tmp = argv[4+ao].val.str);
			if (pcb_layer_typecomb_str2bits(tmp, &slyt, &slyc, 1) != 0)
				return FGW_ERR_ARG_CONV;

			src_idx = pcb_pstk_get_shape_idx(ts, slyt, slyc);
			if (src_idx < 0) {
				RND_ACT_IRES(-1);
				return 0;
			}
			dst_idx = pcb_pstk_get_shape_idx(ts, dlyt, dlyc);
			for(n = 0; n < proto->tr.used; n++) {
				if (dst_idx < 0)
					dst_idx = pcb_pstk_alloc_shape_idx(proto, n);
				else
					pcb_pstk_shape_free(&proto->tr.array[n].shape[dst_idx]);
				pcb_pstk_shape_copy(&proto->tr.array[n].shape[dst_idx], &proto->tr.array[n].shape[src_idx]);
				proto->tr.array[n].shape[dst_idx].layer_mask = dlyt;
				proto->tr.array[n].shape[dst_idx].comb = dlyc;
				if (proto->tr.array[n].smirror)
					pcb_pstk_shape_smirror(&proto->tr.array[n].shape[dst_idx]);
			}
			pcb_pstk_proto_update(proto);
			RND_ACT_IRES(0);
			return 0;

		case act_draw_keywords_hdia:

			RND_ACT_CONVARG(3+ao, FGW_COORD, PstkProtoEdit, crd = fgw_coord(&argv[3+ao]));
			proto->hdia = crd;
			pcb_pstk_proto_update(proto);
			RND_ACT_IRES(0);
			return 0;

		case act_draw_keywords_shape_line:
			{
				rnd_coord_t x1, y1, x2, y2, th;
				int sq = 0;

				RND_ACT_CONVARG(3+ao, FGW_STR, PstkProtoEdit, tmp = argv[3+ao].val.str);
				RND_ACT_CONVARG(4+ao, FGW_COORD, PstkProtoEdit, x1 = fgw_coord(&argv[4+ao]));
				RND_ACT_CONVARG(5+ao, FGW_COORD, PstkProtoEdit, y1 = fgw_coord(&argv[5+ao]));
				RND_ACT_CONVARG(6+ao, FGW_COORD, PstkProtoEdit, x2 = fgw_coord(&argv[6+ao]));
				RND_ACT_CONVARG(7+ao, FGW_COORD, PstkProtoEdit, y2 = fgw_coord(&argv[7+ao]));
				RND_ACT_CONVARG(8+ao, FGW_COORD, PstkProtoEdit, th = fgw_coord(&argv[8+ao]));
				if (pcb_layer_typecomb_str2bits(tmp, &dlyt, &dlyc, 1) != 0)
					return FGW_ERR_ARG_CONV;

				tmp = NULL;
				RND_ACT_MAY_CONVARG(9+ao, FGW_STR, PstkProtoEdit, tmp = argv[9+ao].val.str);
				if ((tmp != NULL) && (*tmp == 's'))
					sq = 1;

				pcb_pstk_proto_del_shape(proto, dlyt, dlyc);
				for(n = 0; n < proto->tr.used; n++) {
					pcb_pstk_shape_t *sh = pcb_pstk_alloc_append_shape(&proto->tr.array[n]);
					memset(sh, 0, sizeof(pcb_pstk_shape_t));
					sh->layer_mask = dlyt;
					sh->comb = dlyc;
					sh->shape = PCB_PSSH_LINE;
					sh->data.line.x1 = x1;
					sh->data.line.y1 = y1;
					sh->data.line.x2 = x2;
					sh->data.line.y2 = y2;
					sh->data.line.thickness = th;
					sh->data.line.square = sq;
				}
			}
			break;

		default:
			return FGW_ERR_ARG_CONV;
	}

	RND_ACT_IRES(-1);
	return 0;
}
