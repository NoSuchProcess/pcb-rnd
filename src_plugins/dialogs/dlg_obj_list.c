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

extern vtp0_t pcb_obj_list_vect;

static long result_idx;
RND_DAD_DECL_NOINIT(dlg)


static void view_expose_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	rnd_xform_t xform;
	rnd_color_t *saved_color;
	long oidx = (pcb_any_obj_t **)prv->user_ctx - (pcb_any_obj_t **)pcb_obj_list_vect.array;
	pcb_any_obj_t *obj = pcb_obj_list_vect.array[oidx];

	rnd_dad_preview_zoomto(attrib, &obj->BoundingBox);

	/* save object color and make it red */
	saved_color = obj->override_color;
	obj->override_color = (rnd_color_t *)&rnd_color_red;

	/* draw the board */
	memset(&xform, 0, sizeof(xform));
	xform.layer_faded = 1;
	rnd_expose_main(rnd_gui, e, &xform);

	/* restore object color */
	obj->override_color = saved_color;
}


static rnd_bool view_mouse_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	if (kind == RND_HID_MOUSE_PRESS) {
		result_idx = (pcb_any_obj_t **)prv->user_ctx - (pcb_any_obj_t **)pcb_obj_list_vect.array;
		RND_DAD_FREE(dlg);
	}
	return rnd_false; /* don't redraw */
}


static const char pcb_acts_dlg_obj_list[] = "dlg_obj_list()";
static const char pcb_acth_dlg_obj_list[] = "Let the user select an object from a list of objects";
static fgw_error_t pcb_act_dlg_obj_list(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {NULL, 0}};
	long n;

	if (pcb_obj_list_vect.used == 0) {
		RND_ACT_IRES(-1);
		return 0;
	}

	result_idx = -1;
	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(dlg, "There are multiple objects at this location.");
		RND_DAD_LABEL(dlg, "Please pick one of them:");
		RND_DAD_BEGIN_VBOX(dlg);
			RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			for(n = 0; n < pcb_obj_list_vect.used; n++) {
				RND_DAD_PREVIEW(dlg, view_expose_cb, view_mouse_cb, NULL, NULL, 100, 100, &pcb_obj_list_vect.array[n]);
			}
		RND_DAD_END(dlg);
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_DEFSIZE(dlg, 150, 350);
	RND_DAD_NEW("obj_list", dlg, "pick one from available objects", NULL, rnd_true, NULL);
	RND_DAD_RUN(dlg);
	RND_DAD_FREE(dlg);

	RND_ACT_IRES(result_idx);
	return 0;
}
