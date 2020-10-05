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

/* Query language - glue functions to other infra */

#define PCB dontuse

static int fnc_action(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	int n, retv = 0;
	fgw_arg_t tmp, resa, arga[PCB_QRY_MAX_FUNC_ARGS], *arg;

	if (argv[0].type != PCBQ_VT_STRING) {
		rnd_message(RND_MSG_ERROR, "query: action() first argument must be a string\n");
		return -1;
	}

	/* convert query arguments to action arguments */
	for(n = 1; n < argc; n++) {
		switch(argv[n].type) {
			case PCBQ_VT_VOID:
				arga[n].type = FGW_PTR;
				arga[n].val.ptr_void = 0;
				break;
			case PCBQ_VT_LST:
				{
					long i;
					pcb_idpath_list_t *list = calloc(sizeof(pcb_idpath_list_t), 1);
					for(i = 0; i < argv[n].data.lst.used; i++) {
						fgw_ptr_reg(&rnd_fgw, &tmp, RND_PTR_DOMAIN_IDPATH, FGW_PTR | FGW_STRUCT, pcb_obj2idpath(argv[n].data.lst.array[i]));
						pcb_idpath_list_append(list, fgw_idpath(&tmp));
					}
					fgw_ptr_reg(&rnd_fgw, &(arga[n]), RND_PTR_DOMAIN_IDPATH_LIST, FGW_PTR | FGW_STRUCT, list);
				}
				break;
			case PCBQ_VT_OBJ:
				fgw_ptr_reg(&rnd_fgw, &(arga[n]), RND_PTR_DOMAIN_IDPATH, FGW_PTR | FGW_STRUCT, pcb_obj2idpath(argv[n].data.obj));
				break;
			case PCBQ_VT_COORD:
				arga[n].type = FGW_COORD;
				fgw_coord(&arga[n]) = argv[n].data.crd;
				break;
			case PCBQ_VT_LONG:
				arga[n].type = FGW_LONG;
				arga[n].val.nat_long = argv[n].data.lng;
				break;
			case PCBQ_VT_DOUBLE:
				arga[n].type = FGW_DOUBLE;
				arga[n].val.nat_double = argv[n].data.dbl;
				break;
			case PCBQ_VT_STRING:
				arga[n].type = FGW_STR;
				arga[n].val.cstr = argv[n].data.str;
				break;
		}
	}

	if (rnd_actionv_bin(&ectx->pcb->hidlib, argv[0].data.str, &resa, argc, arga) != 0) {
		retv = -1;
		goto fin;
	}

	/* convert action result to query result */

#	define FGW_TO_QRY_NUM(lst, val)   res->source = NULL; res->type = PCBQ_VT_LONG; res->data.lng = val; goto fin;
#	define FGW_TO_QRY_STR(lst, val)   res->source = NULL; res->type = PCBQ_VT_STRING; res->data.str = rnd_strdup(val == NULL ? "" : val); goto fin;
#	define FGW_TO_QRY_NIL(lst, val)   res->source = NULL; res->type = PCBQ_VT_VOID; goto fin;

/* has to free idpath and idpathlist return, noone else will have the chance */
#	define FGW_TO_QRY_PTR(lst, val) \
	if (val == NULL) { \
		res->source = NULL; \
		res->type = PCBQ_VT_VOID; \
		goto fin; \
	} \
	else if (fgw_ptr_in_domain(&rnd_fgw, val, RND_PTR_DOMAIN_IDPATH)) { \
		pcb_idpath_t *idp = val; \
		res->source = NULL; \
		res->type = PCBQ_VT_OBJ; \
		res->data.obj = pcb_idpath2obj(ectx->pcb, idp); \
		pcb_idpath_list_remove(idp); \
		fgw_ptr_unreg(&rnd_fgw, &resa, RND_PTR_DOMAIN_IDPATH); \
		free(idp); \
		goto fin; \
	} \
	else if (fgw_ptr_in_domain(&rnd_fgw, val, RND_PTR_DOMAIN_IDPATH_LIST)) { \
		rnd_message(RND_MSG_ERROR, "query action(): can not convert object list yet\n"); \
		res->source = NULL; \
		res->type = PCBQ_VT_VOID; \
		goto fin; \
	} \
	else { \
		rnd_message(RND_MSG_ERROR, "query action(): can not convert unknown pointer\n"); \
		res->source = NULL; \
		res->type = PCBQ_VT_VOID; \
		goto fin; \
	}

	arg = &resa;
	if (FGW_IS_TYPE_CUSTOM(resa.type))
		fgw_arg_conv(&rnd_fgw, &resa, FGW_AUTO);

	switch(FGW_BASE_TYPE(resa.type)) {
		ARG_CONV_CASE_LONG(lst, FGW_TO_QRY_NUM);
		ARG_CONV_CASE_LLONG(lst, FGW_TO_QRY_NUM);
		ARG_CONV_CASE_DOUBLE(lst, FGW_TO_QRY_NUM);
		ARG_CONV_CASE_LDOUBLE(lst, FGW_TO_QRY_NUM);
		ARG_CONV_CASE_PTR(lst, FGW_TO_QRY_PTR);
		ARG_CONV_CASE_STR(lst, FGW_TO_QRY_STR);
		ARG_CONV_CASE_CLASS(lst, FGW_TO_QRY_NIL);
		ARG_CONV_CASE_INVALID(lst, FGW_TO_QRY_NIL);
	}
	if (arg->type & FGW_PTR) {
		FGW_TO_QRY_PTR(lst, arg->val.ptr_void);
	}
	else {
		FGW_TO_QRY_NIL(lst, 0);
	}

	fin:;
	for(n = 1; n < argc; n++) {
		switch(argv[n].type) {
			case PCBQ_VT_LST:
				{
					pcb_idpath_list_t *list = arga[n].val.ptr_void;
					pcb_idpath_list_clear(list);
					free(list);
					arga[n].val.ptr_void = NULL;
				}
				break;
			default: break;
		}
	}

	fgw_arg_free(&rnd_fgw, &resa);
	return retv;
}

/*** net integrity ***/

typedef struct {
	pcb_qry_exec_t *ectx;
	pcb_qry_val_t *res;
} fnc_netint_t;

static int fnc_netint_cb(pcb_net_int_t *nictx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	fnc_netint_t *ctx = nictx->cb_data;

	ctx->res->type = PCBQ_VT_LST;
	vtp0_init(&ctx->res->data.lst);
	vtp0_append(&ctx->res->data.lst, &pcb_qry_drc_ctrl[PCB_QRY_DRC_GRP1]);
	vtp0_append(&ctx->res->data.lst, new_obj);
	vtp0_append(&ctx->res->data.lst, &pcb_qry_drc_ctrl[PCB_QRY_DRC_GRP2]);
	vtp0_append(&ctx->res->data.lst, arrived_from);

	return 1; /*can return only one pair at the moment so break the search */
}

static int fnc_netint_any(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res, int is_shrink)
{
	rnd_coord_t shrink = 0, bloat = 0;
	fnc_netint_t ctx;

	if ((argc != 2) || (argv[0].type != PCBQ_VT_OBJ) || ((argv[1].type != PCBQ_VT_COORD) && (argv[1].type != PCBQ_VT_LONG)))
		return -1;

	if (is_shrink)
		shrink = argv[1].data.crd;
	else
		bloat = argv[1].data.crd;

	ctx.ectx = ectx;
	ctx.res = res;
	res->type = PCBQ_VT_VOID;
	pcb_net_integrity(ectx->pcb, argv[0].data.obj, shrink, bloat, fnc_netint_cb, &ctx);
	return 0;
}

static int fnc_netbreak(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	return fnc_netint_any(ectx, argc, argv, res, 1);
}

static int fnc_netshort(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	return fnc_netint_any(ectx, argc, argv, res, 0);
}

int pcb_qry_fnc_getconf(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	rnd_conf_native_t *nat;

	if (argc != 1)
		return -1;
	if (argv[0].type != PCBQ_VT_STRING)
		return -1;

	nat = rnd_conf_get_field(argv[0].data.str);
	if (nat == NULL)
		PCB_QRY_RET_INV(res);

	switch(nat->type) {
		case RND_CFN_STRING:   PCB_QRY_RET_STR(res, nat->val.string[0]);
		case RND_CFN_BOOLEAN:  PCB_QRY_RET_INT(res, nat->val.boolean[0]);
		case RND_CFN_INTEGER:  PCB_QRY_RET_INT(res, nat->val.integer[0]);
		case RND_CFN_REAL:     PCB_QRY_RET_DBL(res, nat->val.real[0]);
		case RND_CFN_COORD:    PCB_QRY_RET_COORD(res, nat->val.coord[0]);
		
		case RND_CFN_COLOR:  PCB_QRY_RET_STR(res, nat->val.color[0].str);

		case RND_CFN_UNIT:
			if (nat->val.unit[0] == NULL)
				PCB_QRY_RET_INV(res);
			else
				PCB_QRY_RET_STR(res, nat->val.unit[0]->suffix);
			break;

		case RND_CFN_LIST:
		case RND_CFN_HLIST:
		case RND_CFN_max:
			PCB_QRY_RET_INV(res);
	}
	return 0;
}


static int fnc_pstkring(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	rnd_coord_t cnt = 0;
	pcb_pstk_t *ps;

	if ((argc != 2) || (argv[0].type != PCBQ_VT_OBJ) || ((argv[1].type != PCBQ_VT_COORD) && (argv[1].type != PCBQ_VT_LONG)))
		return -1;

	ps = (pcb_pstk_t *)argv[0].data.obj;
	if (ps->type != PCB_OBJ_PSTK)
			PCB_QRY_RET_INV(res);

	pcb_pstk_drc_check_and_warn(ps, &cnt, NULL, argv[1].data.crd, 0);

	PCB_QRY_RET_INT(res, cnt);
}

static int fnc_netlen(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	pcb_board_t *pcb = ectx->pcb;
	pcb_net_t *net = NULL;
	pcb_net_term_t *t;
	pcb_any_obj_t *obj;
	pcb_qry_netseg_len_t *nl;

	if (argc != 1)
		return -1;

	switch(argv[0].type) {
		case PCBQ_VT_STRING:
			net = pcb_net_get(pcb, &pcb->netlist[PCB_NETLIST_EDITED], argv[0].data.str, 0);
			break;
		case PCBQ_VT_OBJ:
			if (argv[0].data.obj->type == PCB_OBJ_NET)
				net = (pcb_net_t *)argv[0].data.obj;
		default:
			break;
	}

	if (net == NULL)
		PCB_QRY_RET_INV(res);

	if (pcb_termlist_length(&net->conns) > 2)
		PCB_QRY_RET_INV(res);

	t = pcb_termlist_first(&net->conns);
	if ((t == NULL) || ((obj = pcb_term_find_name(pcb, pcb->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL)) == NULL))
		PCB_QRY_RET_INV(res);

	nl = pcb_qry_parent_net_lenseg(ectx, obj);

	if ((nl->has_nontrace) || (nl->has_junction))
		PCB_QRY_RET_INV(res);

	PCB_QRY_RET_COORD(res, nl->len);
}
