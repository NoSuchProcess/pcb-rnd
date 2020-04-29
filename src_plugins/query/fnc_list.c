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

/* Query language - listing functions */

#define PCB dontuse

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

static int fnc_violation(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	int n;

	if ((argc < 2) || (argc % 2 != 0))
		return -1;

	/* argument sanity checks */
	for(n = 0; n < argc; n+=2) {
		pcb_qry_val_t *val = &argv[n+1];
		pcb_qry_drc_ctrl_t ctrl = pcb_qry_drc_ctrl_decode(argv[n].data.obj);
		switch(ctrl) {
			case PCB_QRY_DRC_GRP1:
			case PCB_QRY_DRC_GRP2:
				if (val->type != PCBQ_VT_OBJ)
					return -1;
				break;
			case PCB_QRY_DRC_EXPECT:
			case PCB_QRY_DRC_MEASURE:
				if ((val->type != PCBQ_VT_COORD) && (val->type != PCBQ_VT_LONG) && (val->type != PCBQ_VT_DOUBLE))
					return -1;
				break;
			case PCB_QRY_DRC_TEXT:
				break; /* accept anything */
			default:
				return -1;
		}
	}

	res->type = PCBQ_VT_LST;
	vtp0_init(&res->data.lst);
	for(n = 0; n < argc; n+=2) {
		pcb_qry_drc_ctrl_t ctrl = pcb_qry_drc_ctrl_decode(argv[n].data.obj);
		pcb_qry_val_t *val = &argv[n+1];
		pcb_obj_qry_const_t *tmp;

		if (ctrl == PCB_QRY_DRC_TEXT) {
			char *str = "<invalid node>", buff[128];
			switch(val->type) {
				case PCBQ_VT_VOID:   str = "<void>"; break;
				case PCBQ_VT_OBJ:    str = "<obj>"; break;
				case PCBQ_VT_LST:    str = "<list>"; break;
				case PCBQ_VT_COORD:  pcb_snprintf(buff, sizeof(buff), "%mH$", argv[n+1].data.crd); break;
				case PCBQ_VT_LONG:   pcb_snprintf(buff, sizeof(buff), "%ld", argv[n+1].data.lng); break;
				case PCBQ_VT_DOUBLE: pcb_snprintf(buff, sizeof(buff), "%f", argv[n+1].data.dbl); break;
				case PCBQ_VT_STRING: str = (char *)argv[n+1].data.str; break;
			}
			str = rnd_strdup(str == NULL ? "" : str);
			vtp0_append(&res->data.lst, argv[n].data.obj);
			vtp0_append(&res->data.lst, str);
			vtp0_append(&ectx->autofree, str);
		}
		else switch(val->type) {
			case PCBQ_VT_OBJ:
				vtp0_append(&res->data.lst, argv[n].data.obj);
				vtp0_append(&res->data.lst, argv[n+1].data.obj);
				break;
			case PCBQ_VT_COORD:
			case PCBQ_VT_LONG:
			case PCBQ_VT_DOUBLE:
				tmp = malloc(sizeof(pcb_obj_qry_const_t));
				memcpy(&tmp->val, val, sizeof(pcb_qry_val_t));
				vtp0_append(&res->data.lst, argv[n].data.obj);
				vtp0_append(&res->data.lst, tmp);
				vtp0_append(&ectx->autofree, tmp);
				break;
			case PCBQ_VT_VOID:
			case PCBQ_VT_LST:
			case PCBQ_VT_STRING:
				/* can't be on a violation list at the moment */
				break;
		}
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

static int fnc_layerlist(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	long n;

	res->type = PCBQ_VT_LST;
	vtp0_init(&res->data.lst);
	for(n = 0; n < ectx->pcb->Data->LayerN; n++)
		vtp0_append(&res->data.lst, &ectx->pcb->Data->Layer[n]);

	return 0;
}

static int fnc_net_cb(pcb_find_t *fctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	vtp0_t *v = fctx->user_data;
	vtp0_append(v, new_obj);
	return 0;
}

static void fnc_find_init(pcb_qry_exec_t *ectx, pcb_find_t *fctx, vtp0_t *resvt, int rats)
{
	memset(fctx, 0, sizeof(pcb_find_t));
	fctx->consider_rats = rats;
	if (resvt != NULL) {
		fctx->found_cb = fnc_net_cb;
		fctx->user_data = resvt;
	}
	pcb_find_from_obj(fctx, ectx->pcb->Data, NULL);
}

static void fnc_find_from(pcb_qry_exec_t *ectx, pcb_find_t *fctx, pcb_any_obj_t *from)
{
	if (!PCB_FIND_IS_MARKED(fctx, from))
		pcb_find_from_obj_next(fctx, ectx->pcb->Data, from);
}

static int fnc_net_any(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res, int find_all, int netseg)
{
	pcb_net_t *net;
	pcb_net_term_t *t;
	pcb_find_t fctx;


	if ((argv[0].type != PCBQ_VT_OBJ) || (argv[0].data.obj->type != PCB_OBJ_NET))
		return -1;

	if (find_all)
		fnc_find_init(ectx, &fctx, netseg ? NULL : &res->data.lst, 0);

	res->type = PCBQ_VT_LST;
	vtp0_init(&res->data.lst);

	net = (pcb_net_t *)argv[0].data.obj;
	for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
		pcb_any_obj_t *o = pcb_term_find_name(ectx->pcb, ectx->pcb->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
		if (o != NULL) {
			if (find_all) {
				if (netseg && !PCB_FIND_IS_MARKED(&fctx, o)) /* report first (unmarked) object of the seg */
					vtp0_append(&res->data.lst, o);
				fnc_find_from(ectx, &fctx, o); /* mark all objects on this seg, potentiall also reporting them */
			}
			else
				vtp0_append(&res->data.lst, o); /* no search required, just report all terminals */
		}
	}

	if (find_all)
		pcb_find_free(&fctx);

	return 0;
}

static int fnc_netterms(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	return fnc_net_any(ectx, argc, argv, res, 0, 0);
}

static int fnc_netobjs(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	return fnc_net_any(ectx, argc, argv, res, 1, 0);
}

static int fnc_netsegs(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	return fnc_net_any(ectx, argc, argv, res, 1, 1);
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

