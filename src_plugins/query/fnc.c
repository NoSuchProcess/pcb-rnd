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

/* Query language - basic functions */

#include "config.h"

#include <librnd/core/actions.h>
#include <librnd/core/conf.h>
#include <librnd/core/compat_misc.h>
#include <libfungw/fungw_conv.h>

#include "board.h"
#include "data.h"
#include "find.h"
#include "obj_term.h"
#include "netlist.h"
#include "net_int.h"

#include "query_access.h"
#include "query_exec.h"

#define PCB dontuse

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
		PCB_QRY_RET_DBL(res, rnd_distance(argv[0].data.crd, argv[1].data.crd, argv[2].data.crd, argv[3].data.crd));
	PCB_QRY_RET_INV(res);
}

#include "fnc_glue.c"
#include "fnc_list.c"
#include "fnc_geo.c"

void pcb_qry_basic_fnc_init(void)
{
	pcb_qry_fnc_reg("llen", fnc_llen);
	pcb_qry_fnc_reg("distance", fnc_distance);
	pcb_qry_fnc_reg("mklist", fnc_mklist);
	pcb_qry_fnc_reg("violation", fnc_violation);
	pcb_qry_fnc_reg("layerlist", fnc_layerlist);
	pcb_qry_fnc_reg("netlist", fnc_netlist);
	pcb_qry_fnc_reg("netterms", fnc_netterms);
	pcb_qry_fnc_reg("netobjs", fnc_netobjs);
	pcb_qry_fnc_reg("netsegs", fnc_netsegs);
	pcb_qry_fnc_reg("netbreak", fnc_netbreak);
	pcb_qry_fnc_reg("netshort", fnc_netshort);
	pcb_qry_fnc_reg("subcobjs", fnc_subcobjs);
	pcb_qry_fnc_reg("action", fnc_action);
	pcb_qry_fnc_reg("getconf", fnc_getconf);
	pcb_qry_fnc_reg("pstkring", fnc_pstkring);

	pcb_qry_fnc_reg("poly_num_islands", fnc_poly_num_islands);
	pcb_qry_fnc_reg("overlap", fnc_overlap);
	pcb_qry_fnc_reg("intersect", fnc_intersect);
}
