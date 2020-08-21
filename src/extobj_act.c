/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
#include "conf_core.h"
#include "funchash_core.h"
#include <librnd/core/hid_dad.h>
#include "search.h"
#include <librnd/core/tool.h>

#include "extobj.h"

static int extobj_pick_gui(void)
{
	int n, res;
	static int last = 0;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {NULL, 0}};
	RND_DAD_DECL(dlg);

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(dlg, "Choose extended object:");
		if ((last > 0) && (last < pcb_extobj_i2o.used)) {
			pcb_extobj_t *eo = pcb_extobj_i2o.array[last];
			RND_DAD_BEGIN_HBOX(dlg);
				RND_DAD_LABEL(dlg, "Last used:");
				RND_DAD_BUTTON_CLOSE(dlg, eo->name, last);
			RND_DAD_END(dlg);
		}
		RND_DAD_BEGIN_TABLE(dlg, 3);
			for(n = 1; n < pcb_extobj_i2o.used; n++) {
				pcb_extobj_t *eo = pcb_extobj_i2o.array[n];
				RND_DAD_BUTTON_CLOSE(dlg, eo->name, n);
			}
		RND_DAD_END(dlg);
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_NEW("extobj_select", dlg, "Select extended object", NULL, rnd_true, NULL);
	res = RND_DAD_RUN(dlg);
	RND_DAD_FREE(dlg);
	if (res > 0)
		last = res;
	return res;
}

static const char pcb_acts_ExtobjConvFrom[] = "ExtobjConvFrom(selected|buffer, extotype)\nExtobjConvFrom(object, extotype, [idpath])";
static const char pcb_acth_ExtobjConvFrom[] = "Create a new extended object of extotype by converting existing objects";
fgw_error_t pcb_act_ExtobjConvFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	pcb_extobj_t *eo;
	const char *eoname = NULL;
	int op;
	pcb_subc_t *sc;
	pcb_any_obj_t *obj;

	RND_ACT_CONVARG(1, FGW_KEYWORD, ExtobjConvFrom, op = fgw_keyword(&argv[1]));
	RND_ACT_CONVARG(2, FGW_STR, ExtobjConvFrom, eoname = argv[2].val.str);

	if (strcmp(eoname, "@gui") != 0) {
		eo = pcb_extobj_lookup(eoname);
		if (eo == NULL) {
			rnd_message(RND_MSG_ERROR, "ExtobjConvFrom: extended object '%s' is not available\n", eoname);
			RND_ACT_IRES(-1);
			return 0;
		}
	}
	else {
		int idx = extobj_pick_gui();
		if (idx <= 0) {
			RND_ACT_IRES(-1);
			return 0;
		}
		eo = pcb_extobj_i2o.array[idx];
	}

	switch(op) {
		case F_Object:
			if (argc > 3) { /* convert by idpath */
				pcb_idpath_t *idp;
				RND_ACT_CONVARG(3, FGW_IDPATH, ExtobjConvFrom, idp = fgw_idpath(&argv[2]));
				if ((idp == NULL) || !fgw_ptr_in_domain(&rnd_fgw, &argv[3], RND_PTR_DOMAIN_IDPATH))
					return FGW_ERR_PTR_DOMAIN;
				obj = pcb_idpath2obj(PCB, idp);
			}
			else { /* interactive convert */
				void *p1, *p3;
				rnd_coord_t x, y;
				rnd_hid_get_coords("Click on object to convert", &x, &y, 0);
				obj = NULL;
				if (pcb_search_screen(x, y, PCB_OBJ_CLASS_REAL, &p1, (void **)&obj, &p3) == 0) {
					rnd_message(RND_MSG_ERROR, "ExtobjConvFrom: object not found (no object under the cursor)\n");
					RND_ACT_IRES(-1);
					return 0;
				}
			}
			if ((obj == NULL) || ((obj->type & PCB_OBJ_CLASS_REAL) == 0)) {
				rnd_message(RND_MSG_ERROR, "ExtobjConvFrom: object not found\n");
				RND_ACT_IRES(-1);
				return 0;
			}
			sc = pcb_extobj_conv_obj(pcb, eo, pcb->Data, obj, 1);
			break;

		case F_Selected:
			sc = pcb_extobj_conv_selected_objs(pcb, eo, pcb->Data, pcb->Data, 1);
			break;

		case F_Buffer:
			sc = pcb_extobj_conv_all_objs(pcb, eo, PCB_PASTEBUFFER->Data, PCB_PASTEBUFFER->Data, 1);
			break;

		default:
			RND_ACT_FAIL(ExtobjConvFrom);
	}

	if (sc == NULL) {
		rnd_message(RND_MSG_ERROR, "ExtobjConvFrom: failed to create the extended object\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ExtobjGUIPropEdit[] = "ExtobjGUIPropEdit([object, [idpath]])";
static const char pcb_acth_ExtobjGUIPropEdit[] = "Invoke the extobj-implementation-specific GUI property editor, if available";

static int exto_get_obj(fgw_arg_t *res, int argc, fgw_arg_t *argv, pcb_subc_t **dst_obj, pcb_extobj_t **dst_eo, rnd_bool require_extobj)
{
	int op = F_Object;
	pcb_extobj_t *eo;
	pcb_subc_t *obj;

	*dst_obj = NULL;
	*dst_eo = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_KEYWORD, ExtobjGUIPropEdit, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_Object:
			if (argc > 2) { /* convert by idpath */
				pcb_idpath_t *idp;
				RND_ACT_CONVARG(2, FGW_IDPATH, ExtobjGUIPropEdit, idp = fgw_idpath(&argv[2]));
				if ((idp == NULL) || !fgw_ptr_in_domain(&rnd_fgw, &argv[2], RND_PTR_DOMAIN_IDPATH))
					return FGW_ERR_PTR_DOMAIN;
				obj = (pcb_subc_t *)pcb_idpath2obj(PCB, idp);
			}
			else { /* interactive convert */
				void *p1, *p3;
				rnd_coord_t x, y;
				rnd_hid_get_coords("Click on extended object to edit", &x, &y, 0);
				obj = NULL;
				if (pcb_search_screen(x, y, PCB_OBJ_SUBC, &p1, (void **)&obj, &p3) == 0) {
					rnd_message(RND_MSG_ERROR, "ExtobjConvFrom: object not found (no object under the cursor)\n");
					RND_ACT_IRES(-1);
					return 0;
				}
			}
			if ((obj == NULL) || ((obj->type & PCB_OBJ_CLASS_REAL) == 0)) {
				rnd_message(RND_MSG_ERROR, "ExtobjConvFrom: object not found\n");
				RND_ACT_IRES(-1);
				return 0;
			}
			break;

		default:
			RND_ACT_FAIL(ExtobjConvFrom);
	}

	if ((obj == NULL) || (obj->type != PCB_OBJ_SUBC)) {
		rnd_message(RND_MSG_ERROR, "Object is not an extended object (not even a subcircuit)\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	if (require_extobj &&  (obj->extobj == NULL)) {
		rnd_message(RND_MSG_ERROR, "Subcircuit is not an extended object\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	eo = pcb_extobj_get(obj);
	if (require_extobj && (eo == NULL)) {
		rnd_message(RND_MSG_ERROR, "Extended object '%s' is not available\n", obj->extobj);
		RND_ACT_IRES(1);
		return 0;
	}


	*dst_obj = obj;
	*dst_eo = eo;
	return 0;
}

fgw_error_t pcb_act_ExtobjGUIPropEdit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_extobj_t *eo;
	pcb_subc_t *obj;
	int rv = exto_get_obj(res, argc, argv, &obj, &eo, 1);

	if (obj == NULL) {
		RND_ACT_IRES(1);
		return rv;
	}

	if (eo->gui_propedit != NULL)
		eo->gui_propedit(obj);
	else
		rnd_message(RND_MSG_ERROR, "Extended object '%s' does not implement GUI property editor", obj->extobj);

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ExtobjToggle[] = "ExtobjToggle([object, [idpath]])";
static const char pcb_acth_ExtobjToggle[] = "Convert an extobj into a plain subc or back";
fgw_error_t pcb_act_ExtobjToggle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_extobj_t *eo;
	pcb_subc_t *obj;
	int rv = exto_get_obj(res, argc, argv, &obj, &eo, 0);

	if (obj == NULL) {
		RND_ACT_IRES(1);
		return rv;
	}

	if (eo != NULL) {
		pcb_attribute_put(&obj->Attributes, "disabled_extobj", obj->extobj);
		pcb_attribute_remove(&obj->Attributes, "extobj");
		PCB_FLAG_CLEAR(PCB_FLAG_LOCK, obj);
	}
	else {
		const char *type = pcb_attribute_get(&obj->Attributes, "disabled_extobj");
		if (type == NULL) {
			rnd_message(RND_MSG_ERROR, "Subcircuit was not an extended object\n");
			RND_ACT_IRES(1);
			return 0;
		}
		pcb_attribute_put(&obj->Attributes, "extobj", type);
		pcb_attribute_remove(&obj->Attributes, "disabled_extobj");
		PCB_FLAG_SET(PCB_FLAG_LOCK, obj);
	}

	RND_ACT_IRES(0);
	return 0;
}


static rnd_action_t pcb_extobj_action_list[] = {
	{"ExtobjConvFrom", pcb_act_ExtobjConvFrom, pcb_acth_ExtobjConvFrom, pcb_acts_ExtobjConvFrom},
	{"ExtobjGUIPropEdit", pcb_act_ExtobjGUIPropEdit, pcb_acth_ExtobjGUIPropEdit, pcb_acts_ExtobjGUIPropEdit},
	{"ExtobjToggle", pcb_act_ExtobjToggle, pcb_acth_ExtobjToggle, pcb_acts_ExtobjToggle}
};

void pcb_extobj_act_init2(void)
{
	RND_REGISTER_ACTIONS(pcb_extobj_action_list, NULL);
}
