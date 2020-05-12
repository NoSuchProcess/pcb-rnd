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

#include "event.h"
#include "actions_pcb.h"
#include "plug_footprint.h"
#include "plug_io.h"


static void fpmap_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	static rnd_dad_retovr_t retovr;
	char **resname = caller_data;
	*resname = attr->user_data;
	rnd_hid_dad_close(hid_ctx, &retovr, 0);
}

static const char pcb_acts_gui_fpmap_choose[] = "gui_fpmap_choose(map)\n";
static const char pcb_acth_gui_fpmap_choose[] = "Internal call action for a dialog to select a footprint from a map.";
static fgw_error_t pcb_act_gui_fpmap_choose(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *resname = NULL;
	const pcb_plug_fp_map_t *map, *m;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", 0}, {NULL, 0}};
	RND_DAD_DECL(dlg);

	RND_ACT_MAY_CONVARG(1, FGW_PTR, gui_fpmap_choose, map = argv[1].val.ptr_void);

	if ((map == NULL) || (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PCB_PTR_DOMAIN_FPMAP)))
		return -1;

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(dlg, "Multiple footprints available.");
		RND_DAD_LABEL(dlg, "Choose one.");
		RND_DAD_BEGIN_VBOX(dlg);
			RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			for(m = map; m != NULL; m = m->next) {
				if (m->type == PCB_FP_FILE) {
					RND_DAD_BUTTON(dlg, m->name);
						RND_DAD_CHANGE_CB(dlg, fpmap_cb);
						RND_DAD_SET_ATTR_FIELD(dlg, user_data, m->name);
				}
			}
		RND_DAD_END(dlg);
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_DEFSIZE(dlg, 250, 350);

	RND_DAD_NEW("fpmap", dlg, "Choose footprint", &resname, rnd_true, NULL);
	RND_DAD_RUN(dlg);
	RND_DAD_FREE(dlg);


	res->type = FGW_STR;
	res->val.cstr = resname;
	return 0;
}

