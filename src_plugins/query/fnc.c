/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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

#include <librnd/core/actions.h>
#include <libfungw/fungw_conv.h>

#include "board.h"
#include "data.h"
#include "find.h"
#include "obj_term.h"
#include "netlist.h"

#include "query_access.h"
#include "query_exec.h"

#define PCB dontuse

/* Query language - basic functions */
static int fnc_llen(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	if (argc != 1)
		return -1;
	if (argv[0].type != PCBQ_VT_LST)
		PCB_QRY_RET_INV(res);
	PCB_QRY_RET_INT(res, vtp0_len(&argv[0].data.lst));
}

static int fnc_distance(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	if ((argc == 4) && (argv[0].type == PCBQ_VT_COORD) && (argv[1].type == PCBQ_VT_COORD) && (argv[2].type == PCBQ_VT_COORD) && (argv[3].type == PCBQ_VT_COORD))
		PCB_QRY_RET_DBL(res, pcb_distance(argv[0].data.crd, argv[1].data.crd, argv[2].data.crd, argv[3].data.crd));
	PCB_QRY_RET_INV(res);
}

static int fnc_mklist(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	int n;
	res->type = PCBQ_VT_LST;
	vtp0_init(&res->data.lst);
	for(n = 0; n < argc; n++) {
		if (argv[n].type == PCBQ_VT_OBJ)
			vtp0_append(&res->data.lst, argv[n].data.obj);
	}
	return 0;
}

static int fnc_netlist(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	long n;
	htsp_entry_t *e;
	pcb_netlist_t *nl = &ectx->pcb->netlist[PCB_NETLIST_EDITED];

	res->type = PCBQ_VT_LST;
	vtp0_init(&res->data.lst);

	if (nl->used != 0)
		for(e = htsp_first(nl), n = 0; e != NULL; e = htsp_next(nl, e), n++)
			vtp0_append(&res->data.lst, e->value);

	return 0;
}

static int fnc_net_cb(pcb_find_t *fctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	vtp0_t *v = fctx->user_data;
	vtp0_append(v, new_obj);
	return 0;
}


static int fnc_net_any(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res, int find_all)
{
	pcb_net_t *net;
	pcb_net_term_t *t;
	pcb_find_t fctx;


	if ((argv[0].type != PCBQ_VT_OBJ) || (argv[0].data.obj->type != PCB_OBJ_NET))
		return -1;

	if (find_all) {
		memset(&fctx, 0, sizeof(fctx));
		fctx.consider_rats = 1;
		fctx.found_cb = fnc_net_cb;
		fctx.user_data = &res->data.lst;
		pcb_find_from_obj(&fctx, ectx->pcb->Data, NULL);
	}

	res->type = PCBQ_VT_LST;
	vtp0_init(&res->data.lst);

	net = (pcb_net_t *)argv[0].data.obj;
	for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
		pcb_any_obj_t *o = pcb_term_find_name(ectx->pcb, ectx->pcb->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
		if (o != NULL) {
			if (find_all) {
				if (!PCB_FIND_IS_MARKED(&fctx, o))
					pcb_find_from_obj_next(&fctx, ectx->pcb->Data, o);
			}
			else
				vtp0_append(&res->data.lst, o);
		}
	}

	return 0;
}

static int fnc_netterms(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res, int find)
{
	return fnc_net_any(ectx, argc, argv, res, 0);
}

static int fnc_netobjs(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res, int find)
{
	return fnc_net_any(ectx, argc, argv, res, 1);
}


static int fnc_subcobjs(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	if ((argv[0].type != PCBQ_VT_OBJ) || (argv[0].data.obj->type != PCB_OBJ_SUBC))
		return -1;

	res->type = PCBQ_VT_LST;
	vtp0_init(&res->data.lst);
	pcb_qry_list_all_subc(res, (pcb_subc_t *)argv[0].data.obj, PCB_OBJ_ANY & (~PCB_OBJ_LAYER));

	return 0;
}

static int fnc_action(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	int n;
	fgw_arg_t tmp, resa, arga[PCB_QRY_MAX_FUNC_ARGS], *arg;
	fgw_error_t e;

	if (argv[0].type != PCBQ_VT_STRING) {
		pcb_message(PCB_MSG_ERROR, "query: action() first argument must be a string\n");
		return -1;
	}

	/* convert query arguments to action arguments */
	for(n = 1; n < argc; n++) {
		switch(argv[n].type) {
			case PCBQ_VT_VOID:
				arga[n].type = FGW_PTR;
				arga[n].val.ptr_void = 0;
			case PCBQ_VT_LST:
				{
					long i;
					pcb_idpath_list_t *list = calloc(sizeof(pcb_idpath_list_t), 1);
					for(i = 0; i < argv[n].data.lst.used; i++) {
						fgw_ptr_reg(&pcb_fgw, &tmp, PCB_PTR_DOMAIN_IDPATH, FGW_PTR | FGW_STRUCT, pcb_obj2idpath(argv[n].data.lst.array[i]));
						pcb_idpath_list_append(list, fgw_idpath(&tmp));
					}
					fgw_ptr_reg(&pcb_fgw, &(arga[n]), PCB_PTR_DOMAIN_IDPATH_LIST, FGW_PTR | FGW_STRUCT, list);
				}
				break;
			case PCBQ_VT_OBJ:
				fgw_ptr_reg(&pcb_fgw, &(arga[n]), PCB_PTR_DOMAIN_IDPATH, FGW_PTR | FGW_STRUCT, pcb_obj2idpath(argv[n].data.obj));
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

	if (pcb_actionv_bin(&ectx->pcb->hidlib, argv[0].data.str, &resa, argc, arga) != 0)
		return -1;

	/* convert action result to query result */

#	define FGW_TO_QRY_NUM(lst, val)   res->type = PCBQ_VT_LONG; res->data.lng = val; goto fin;
#	define FGW_TO_QRY_STR(lst, val)   res->type = PCBQ_VT_STRING; res->data.str = pcb_strdup(val == NULL ? "" : val); goto fin;
#	define FGW_TO_QRY_NIL(lst, val)   res->type = PCBQ_VT_VOID; goto fin;

/* has to free idpath and idpathlist return, noone else will have the chance */
#	define FGW_TO_QRY_PTR(lst, val) \
	if (val == NULL) { \
		res->type = PCBQ_VT_VOID; \
		goto fin; \
	} \
	else if (fgw_ptr_in_domain(&pcb_fgw, val, PCB_PTR_DOMAIN_IDPATH)) { \
		pcb_idpath_t *idp = val; \
		res->type = PCBQ_VT_OBJ; \
		res->data.obj = pcb_idpath2obj(ectx->pcb, idp); \
		pcb_idpath_list_remove(idp); \
		fgw_ptr_unreg(&pcb_fgw, &resa, PCB_PTR_DOMAIN_IDPATH); \
		free(idp); \
		goto fin; \
	} \
	else if (fgw_ptr_in_domain(&pcb_fgw, val, PCB_PTR_DOMAIN_IDPATH_LIST)) { \
		pcb_message(PCB_MSG_ERROR, "query action(): can not convert object list yet\n"); \
		res->type = PCBQ_VT_VOID; \
		goto fin; \
	} \
	else { \
		pcb_message(PCB_MSG_ERROR, "query action(): can not convert unknown pointer\n"); \
		res->type = PCBQ_VT_VOID; \
		goto fin; \
	}

	arg = &resa;
	if (FGW_IS_TYPE_CUSTOM(resa.type))
		fgw_arg_conv(&pcb_fgw, &resa, FGW_AUTO);

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
	fgw_arg_free(&pcb_fgw, &resa);
	return 0;
}


void pcb_qry_basic_fnc_init(void)
{
	pcb_qry_fnc_reg("llen", fnc_llen);
	pcb_qry_fnc_reg("distance", fnc_distance);
	pcb_qry_fnc_reg("mklist", fnc_mklist);
	pcb_qry_fnc_reg("netlist", fnc_netlist);
	pcb_qry_fnc_reg("netterms", fnc_netterms);
	pcb_qry_fnc_reg("netobjs", fnc_netobjs);
	pcb_qry_fnc_reg("subcobjs", fnc_subcobjs);
	pcb_qry_fnc_reg("action", fnc_action);
}
