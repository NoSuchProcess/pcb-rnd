/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

/* includeed from act_read.c */

static const char pcb_acts_IDPList[] =
	"IDPList(alloc)\n"
	"IDPList(free|clear|print|dup|length, list)\n"
	"IDPList(get|pop|remove, list, idx)\n"
	"IDPList(prepend|append|push, list, idpath)"
	;
static const char pcb_acth_IDPList[] = "Basic idpath list manipulation.";
static fgw_error_t pcb_act_IDPList(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd_;
	pcb_idpath_list_t *list;
	pcb_idpath_t *idp;
	int cmd;
	long idx;

	PCB_ACT_CONVARG(1, FGW_STR, IDPList, cmd_ = argv[1].val.str);

	cmd = act_read_keywords_sphash(cmd_);
	if (cmd == act_read_keywords_alloc) {
		list = calloc(sizeof(pcb_idpath_list_t), 1);
		fgw_ptr_reg(&pcb_fgw, res, PCB_PTR_DOMAIN_IDPATH_LIST, FGW_PTR | FGW_STRUCT, list);
		return 0;
	}
	PCB_ACT_CONVARG(2, FGW_IDPATH_LIST, IDPList, list = fgw_idpath_list(&argv[2]));

	if (!fgw_ptr_in_domain(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH_LIST))
		return FGW_ERR_PTR_DOMAIN;

	switch(cmd) {
		case act_read_keywords_clear:
			pcb_idpath_list_clear(list);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_length:
			PCB_ACT_IRES(pcb_idpath_list_length(list));
			return 0;

		case act_read_keywords_free:
			fgw_ptr_unreg(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH_LIST);
			pcb_idpath_list_clear(list);
			free(list);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_append:
		case act_read_keywords_push:
		case act_read_keywords_prepend:
			PCB_ACT_CONVARG(3, FGW_IDPATH, IDPList, idp = fgw_idpath(&argv[3]));
			if (!fgw_ptr_in_domain(&pcb_fgw, &argv[3], PCB_PTR_DOMAIN_IDPATH))
				return FGW_ERR_PTR_DOMAIN;
			if (cmd == act_read_keywords_append)
				pcb_idpath_list_append(list, idp);
			else /* prepend or push */
				pcb_idpath_list_insert(list, idp);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_remove:
			PCB_ACT_CONVARG(3, FGW_LONG, IDPList, idx = argv[3].val.nat_long);
			idp = pcb_idpath_list_nth(list, idx);
			if (idp == NULL) {
				PCB_ACT_IRES(-1);
				return 0;
			}
			pcb_idpath_list_remove(idp);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_get:
			PCB_ACT_CONVARG(3, FGW_LONG, IDPList, idx = argv[3].val.nat_long);
			idp = pcb_idpath_list_nth(list, idx);
			if (idp == NULL) {
				res->type = FGW_PTR;
				res->val.ptr_struct = NULL;
				return 0;
			}
			fgw_ptr_reg(&pcb_fgw, res, PCB_PTR_DOMAIN_IDPATH, FGW_PTR | FGW_STRUCT, idp);
			return 0;

		case act_read_keywords_pop:
			idp = pcb_idpath_list_first(list);
			if (idp == NULL) {
				res->type = FGW_PTR;
				res->val.ptr_struct = NULL;
				return 0;
			}
			fgw_ptr_reg(&pcb_fgw, res, PCB_PTR_DOMAIN_IDPATH, FGW_PTR | FGW_STRUCT, idp);
			pcb_idpath_list_remove(idp);
			return 0;

		case act_read_keywords_print:
TODO("print");
			break;
	}

	return -1;
}

static const char pcb_acts_IDP[] = "IDP([print|free], idpath)\n";
static const char pcb_acth_IDP[] = "Basic idpath manipulation.";
static fgw_error_t pcb_act_IDP(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd;
	pcb_idpath_t *idp;

	PCB_ACT_CONVARG(1, FGW_STR, IDP, cmd = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_IDPATH, IDPList, idp = fgw_idpath(&argv[2]));
	if ((idp == NULL) || !fgw_ptr_in_domain(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH))
		return FGW_ERR_PTR_DOMAIN;


	switch(act_read_keywords_sphash(cmd)) {
		case act_read_keywords_free:
			pcb_idpath_list_remove(idp);
			fgw_ptr_unreg(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH);
			free(idp);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_print:
TODO("print");
			break;
	}

	return -1;
}

static const char pcb_acts_GetParentData[] = "GetParentData([root_data,] idpath)\n";
static const char pcb_acth_GetParentData[] = "Return the closest upstream pcb_data_t * parent of an object";
static fgw_error_t pcb_act_GetParentData(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_idpath_t *idp;
	pcb_data_t *root_data = PCB->Data;
	int iidx = 1;
	pcb_any_obj_t *obj;

	res->type = FGW_PTR | FGW_STRUCT;
	res->val.ptr_void = NULL;

	if (argc > 2) {
		PCB_ACT_CONVARG(1, FGW_DATA, GetParentData, root_data = fgw_data(&argv[1]));
		iidx++;
	}

	PCB_ACT_CONVARG(iidx, FGW_IDPATH, IDPList, idp = fgw_idpath(&argv[iidx]));
	if ((idp == NULL) || !fgw_ptr_in_domain(&pcb_fgw, &argv[iidx], PCB_PTR_DOMAIN_IDPATH))
		return FGW_ERR_PTR_DOMAIN;

	obj = pcb_idpath2obj_in(root_data, idp);
	if (obj == NULL)
		return 0;

	switch(obj->parent_type) {
		case PCB_PARENT_LAYER:
			assert(obj->parent.layer != NULL);
			assert(obj->parent.layer->parent_type == PCB_PARENT_DATA);
			res->val.ptr_void = obj->parent.layer->parent.data;
			break;
		case PCB_PARENT_SUBC:
			assert(obj->parent.subc != NULL);
			assert(obj->parent.subc->parent_type == PCB_PARENT_DATA);
			res->val.ptr_void = obj->parent.subc->parent.data;
			break;
		case PCB_PARENT_DATA:
			res->val.ptr_void = obj->parent.data;
			break;
		case PCB_PARENT_BOARD:
		case PCB_PARENT_NET:
		case PCB_PARENT_UI:
		case PCB_PARENT_INVALID:
			break;
	}
	return 0;
}
