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

#include "board.h"
#include "data.h"
#include "obj_term.h"
#include "netlist.h"

#include "query_access.h"
#include "query_exec.h"

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
	pcb_netlist_t *nl = &PCB->netlist[PCB_NETLIST_EDITED];

	res->type = PCBQ_VT_LST;
	vtp0_init(&res->data.lst);

	if (nl->used != 0)
		for(e = htsp_first(nl), n = 0; e != NULL; e = htsp_next(nl, e), n++)
			vtp0_append(&res->data.lst, e->value);

	return 0;
}

static int fnc_netterms(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	pcb_net_t *net;
	pcb_net_term_t *t;

	if ((argv[0].type != PCBQ_VT_OBJ) || (argv[0].data.obj->type != PCB_OBJ_NET))
		return -1;

	res->type = PCBQ_VT_LST;
	vtp0_init(&res->data.lst);

	net = (pcb_net_t *)argv[0].data.obj;
	for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
		pcb_any_obj_t *o = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
		if (o != NULL)
			vtp0_append(&res->data.lst, o);
	}

	return 0;
}

void pcb_qry_basic_fnc_init(void)
{
	pcb_qry_fnc_reg("llen", fnc_llen);
	pcb_qry_fnc_reg("distance", fnc_distance);
	pcb_qry_fnc_reg("mklist", fnc_mklist);
	pcb_qry_fnc_reg("netlist", fnc_netlist);
	pcb_qry_fnc_reg("netterms", fnc_netterms);
}
