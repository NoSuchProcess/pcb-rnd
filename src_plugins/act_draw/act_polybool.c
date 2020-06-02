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

/* included from act_draw.c */
#include "search.h"
#include "obj_poly.h"
#include "data_list.h"

/* returns number of objects added before esc */
static int pick_obj(vtp0_t *objs, const char *ask_first, const char *ask_subseq)
{
	rnd_coord_t x, y;
	const char *msg = objs->used == 0 ? ask_first : ask_subseq;
	for(;;) {
		int res = rnd_hid_get_coords(msg, &x, &y, 1);
		void *p1, *p2, *p3;
		if (res < 0)
			return 0;
		res = pcb_search_screen(x, y, PCB_OBJ_POLY, &p1, &p2, &p3);
		if (res == PCB_OBJ_POLY) {
			int n, found = 0;
			for(n = 0; n < objs->used; n++) {
				if (objs->array[n] == p2) {
					found = 1;
					break;
				}
			}
			if (found) {
				rnd_message(RND_MSG_ERROR, "That polygon is already on the list! Try again or press esc!\n");
				continue;
			}
			vtp0_append(objs, p2);
			return 1;
		}
		rnd_message(RND_MSG_ERROR, "There's no polygon there. Try again or press esc!\n");
	}
}

static const char pcb_acts_PolyBool[] = "PstkProto([noundo,] unite|isect|sub, [poly1, poly2, [poly...]])\n";
static const char pcb_acth_PolyBool[] = "Perform polygon boolean operation on the clipped polygons referred. A poly is either and idpath, selected, found or object (for the object under the cursor). When not specified, two object polygons are used.";
/* doc: polybool.html */
static fgw_error_t pcb_act_PolyBool(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	char *cmd;
	int cmdi, n;
	vtp0_t objs;
	rnd_bool_op_t bop;
	const char *ask_first, *ask_subseq;
	rnd_polyarea_t *pa, *ptmp;
	DRAWOPTARG;


	RND_ACT_CONVARG(1+ao, FGW_STR, PolyBool, cmd = argv[1+ao].val.str);

	cmdi = act_draw_keywords_sphash(cmd);
	switch(cmdi) {
		case act_draw_keywords_unite: bop = RND_PBO_UNITE; ask_first = ask_subseq = "click on polygon to unite, press esc after the last"; break;
		case act_draw_keywords_isect: bop = RND_PBO_ISECT; ask_first = ask_subseq = "click on polygon to contribute in intersection, press esc after the last"; break;
		case act_draw_keywords_sub:   bop = RND_PBO_SUB; ask_first = "click on polygon to subtract other polygons from"; ask_subseq = "click on polygon to subtract from the first, press esc after the last"; break;
/*		case act_draw_keywords_xor:   bop = RND_PBO_XOR; ask_first = ask_subseq = "click on polygon to xor, press esc after the last"; break;*/
		default:
			RND_ACT_FAIL(PolyBool);
	}

	RND_ACT_IRES(-1);
	vtp0_init(&objs);
	for(n = 2+ao; n < argc; n++) {
		if ((argv[n].type & FGW_STR) == FGW_STR) {
			if (strcmp(argv[n].val.str, "object") == 0)
				pick_obj(&objs, ask_first, ask_subseq);
			else if (strcmp(argv[n].val.str, "selected") == 0)
				pcb_data_list_by_flag(pcb->Data, &objs, PCB_OBJ_POLY, PCB_FLAG_SELECTED);
			else if (strcmp(argv[n].val.str, "found") == 0)
				pcb_data_list_by_flag(pcb->Data, &objs, PCB_OBJ_POLY, PCB_FLAG_FOUND);
			else
				goto try_idp;
		}
		else {
			pcb_idpath_t *idp;
			pcb_any_obj_t *obj;
			try_idp:;
			RND_ACT_MAY_CONVARG(n, FGW_IDPATH, PolyBool, idp = fgw_idpath(&argv[n]));
			if ((idp == NULL) || !fgw_ptr_in_domain(&rnd_fgw, &argv[n], RND_PTR_DOMAIN_IDPATH)) {
				rnd_message(RND_MSG_ERROR, "Invalid polygon specification idpath in arg %d: pointer domain error\n", n);
				goto error;
			}
			obj = pcb_idpath2obj(PCB, idp);
			if ((obj == NULL) || (obj->type != PCB_OBJ_POLY)) {
				rnd_message(RND_MSG_ERROR, "Invalid polygon specification idpath in arg %d: object is not a polygon\n", n);
				goto error;
			}
			vtp0_append(&objs, obj);
		}
	}

	while(objs.used < 2) {
		if (pick_obj(&objs, ask_first, ask_subseq) == 0)
			goto error; /* aborted */
	}

	/* set up the base poly in pa */
	if (!rnd_polyarea_m_copy0(&pa, ((pcb_poly_t *)objs.array[0])->Clipped)) {
		rnd_message(RND_MSG_ERROR, "Failed to copy the first polygon\n", n);
		goto error;
	}

	/* peform the boolean ops */
	for(n = 1; n < objs.used; n++) {
		rnd_polyarea_boolean(pa, ((pcb_poly_t *)objs.array[n])->Clipped, &ptmp, bop);
		rnd_polyarea_free(&pa);
		pa = ptmp;
	}

	if (noundo)
		pcb_undo_freeze_add();
	pcb_poly_to_polygons_on_layer(pcb->Data, PCB_CURRLAYER(pcb), pa, pcb_flag_make(PCB_FLAG_CLEARLINE));
	if (noundo)
		pcb_undo_unfreeze_add();


	rnd_polyarea_free(&pa);

	error:;
	vtp0_uninit(&objs);
	return 0;
}

