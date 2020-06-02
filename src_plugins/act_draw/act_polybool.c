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

#include "obj_pstk_inlines.h"

static void pick_obj(vtp0_t *objs, const char *ask_first, const char *ask_subseq)
{
	rnd_coord_t x, y;
	const char *msg = objs->used == 0 ? ask_first : ask_subseq;
	for(;;) {
		rnd_hid_get_coords(msg, &x, &y, 1);
		rnd_trace("xy: %$mm %$mm\n", x, y);
	}
}

static const char pcb_acts_PolyBool[] = "PstkProto([noundo,] unite|isect|sub|xor, [poly1, poly2, [poly...]])\n";
static const char pcb_acth_PolyBool[] = "Perform polygon boolean operation on the clipped polygons referred. A poly is either and idpath, selected, found or object (for the object under the cursor). When not specified, two object polygons are used.";
static fgw_error_t pcb_act_PolyBool(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *cmd;
	int cmdi, n;
	vtp0_t objs;
	rnd_bool_op_t bop;
	const char *ask_first, *ask_subseq;
	DRAWOPTARG;


	RND_ACT_CONVARG(1+ao, FGW_STR, PolyBool, cmd = argv[1+ao].val.str);

	cmdi = act_draw_keywords_sphash(cmd);
	switch(cmdi) {
		case act_draw_keywords_unite: bop = RND_PBO_UNITE; ask_first = ask_subseq = "click on polygon to unite, press esc after the last"; break;
		case act_draw_keywords_isect: bop = RND_PBO_ISECT; ask_first = ask_subseq = "click on polygon to contribute in intersection, press esc after the last"; break;
		case act_draw_keywords_sub:   bop = RND_PBO_SUB; ask_first = "click on polygon to subtract other polygons from"; ask_subseq = "click on polygon to subtract from the first, press esc after the last"; break;
		case act_draw_keywords_xor:   bop = RND_PBO_XOR; ask_first = ask_subseq = "click on polygon to xor, press esc after the last"; break;
		default:
			RND_ACT_FAIL(PolyBool);
	}

	RND_ACT_IRES(-1);
	vtp0_init(&objs);
	for(n = 2+ao; n < argc; n++) {
		if ((argv[n].type & FGW_STR) == FGW_STR) {
			if (strcmp(argv[n].val.str, "object") == 0) {
				pick_obj(&objs, ask_first, ask_subseq);
			}
			else {
				rnd_message(RND_MSG_ERROR, "Invalid polygon specification string: '%s'\n", argv[n].val.str);
				goto error;
			}
		}
		else {
			pcb_idpath_t *idp;
			RND_ACT_MAY_CONVARG(n, FGW_IDPATH, PolyBool, idp = fgw_idpath(&argv[n]));
			if ((idp == NULL) || !fgw_ptr_in_domain(&rnd_fgw, &argv[n], RND_PTR_DOMAIN_IDPATH)) {
				rnd_message(RND_MSG_ERROR, "Invalid polygon specification idpath in arg %d\n", n);
				goto error;
			}
		}
	}

	while(objs.used < 2)
		pick_obj(&objs, ask_first, ask_subseq);

	error:;
	vtp0_uninit(&objs);
	return 0;
}

