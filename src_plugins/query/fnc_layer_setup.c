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

void pcb_qry_uninit_layer_setup(pcb_qry_exec_t *ectx)
{
	if (!ectx->layer_setup_inited)
		return;

	ectx->layer_setup_inited = 0;
}

static void pcb_qry_init_layer_setup(pcb_qry_exec_t *ectx)
{
	ectx->layer_setup_inited = 1;
}

static int fnc_layer_setup(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	pcb_any_obj_t *obj, *lo;
	pcb_layer_t *ly = NULL;
	const char *lss;

	if ((argc != 2) || (argv[0].type != PCBQ_VT_OBJ) || (argv[1].type != PCBQ_VT_STRING))
		return -1;

	lss = argv[1].data.str;

	if (!ectx->layer_setup_inited)
		pcb_qry_init_layer_setup(ectx);

	obj = (pcb_any_obj_t *)argv[0].data.obj;
rnd_trace("layer_setup: %p/'%s'\n", lss, lss);

	PCB_QRY_RET_INV(res);
}

