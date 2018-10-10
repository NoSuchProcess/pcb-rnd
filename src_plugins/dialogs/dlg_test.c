/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "hid_dad_tree.h"

static const char dlg_test_syntax[] = "dlg_test()\n";
static const char dlg_test_help[] = "test the attribute dialog";

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int wtab, tt;
	int ttctr;
} test_t;


static void pcb_act_attr_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
static void cb_tab_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
static void cb_jump(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
static void cb_ttbl_insert(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
static void cb_ttbl_append(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
static void cb_ttbl_select(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
static void cb_ttbl_free_row(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row);


static int attr_idx, attr_idx2;
static fgw_error_t pcb_act_dlg_test(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *vals[] = { "foo", "bar", "baz", NULL };
	const char *tabs[] = { "original test", "new test", "tree-table", NULL };
	char *row1[] = {"one", "foo", "FOO", NULL};
	char *row2[] = {"two", "bar", "BAR", NULL};
	char *row2b[] = {"under_two", "ut", "uuut", NULL};
	char *row3[] = {"three", "baz", "BAZ", NULL};
	const char *hdr[] = {"num", "data1", "data2", NULL};
	pcb_hid_row_t *row;

	test_t ctx;
	memset(&ctx, 0, sizeof(ctx));

	PCB_DAD_BEGIN_TABBED(ctx.dlg, tabs);
		PCB_DAD_CHANGE_CB(ctx.dlg, cb_tab_chg);
		ctx.wtab = PCB_DAD_CURRENT(ctx.dlg);

		/* tab 0: "original test" */
		PCB_DAD_BEGIN_VBOX(ctx.dlg);
			PCB_DAD_LABEL(ctx.dlg, "text1");
			PCB_DAD_BEGIN_TABLE(ctx.dlg, 3);
				PCB_DAD_LABEL(ctx.dlg, "text2a");
				PCB_DAD_LABEL(ctx.dlg, "text2b");
				PCB_DAD_LABEL(ctx.dlg, "text2c");
				PCB_DAD_LABEL(ctx.dlg, "text2d");
			PCB_DAD_END(ctx.dlg);
			PCB_DAD_LABEL(ctx.dlg, "text3");

			PCB_DAD_ENUM(ctx.dlg, vals);
				PCB_DAD_CHANGE_CB(ctx.dlg, pcb_act_attr_chg);
				attr_idx = PCB_DAD_CURRENT(ctx.dlg);
			PCB_DAD_INTEGER(ctx.dlg, "text2e");
				PCB_DAD_MINVAL(ctx.dlg, 1);
				PCB_DAD_MAXVAL(ctx.dlg, 10);
				PCB_DAD_DEFAULT(ctx.dlg, 3);
				PCB_DAD_CHANGE_CB(ctx.dlg, pcb_act_attr_chg);
				attr_idx2 = PCB_DAD_CURRENT(ctx.dlg);
			PCB_DAD_BUTTON(ctx.dlg, "update!");
				PCB_DAD_CHANGE_CB(ctx.dlg, pcb_act_attr_chg);
		PCB_DAD_END(ctx.dlg);

		/* tab 1: "new test" */
		PCB_DAD_BEGIN_VBOX(ctx.dlg);
			PCB_DAD_LABEL(ctx.dlg, "new test.");
			PCB_DAD_BUTTON(ctx.dlg, "jump to the first tab");
				PCB_DAD_CHANGE_CB(ctx.dlg, cb_jump);
		PCB_DAD_END(ctx.dlg);

		/* tab 2: tree table widget */
		PCB_DAD_BEGIN_VBOX(ctx.dlg);
			PCB_DAD_TREE(ctx.dlg, 3, 1, hdr);
				ctx.tt = PCB_DAD_CURRENT(ctx.dlg);
				PCB_DAD_CHANGE_CB(ctx.dlg, cb_ttbl_select);
				PCB_DAD_TREE_SET_CB(ctx.dlg, free_cb, cb_ttbl_free_row);
				PCB_DAD_TREE_APPEND(ctx.dlg, NULL, row1);
				row = PCB_DAD_TREE_APPEND(ctx.dlg, NULL, row2);
				PCB_DAD_TREE_APPEND_UNDER(ctx.dlg, row, row2b);
				PCB_DAD_TREE_APPEND(ctx.dlg, NULL, row3);
			PCB_DAD_BEGIN_HBOX(ctx.dlg);
				PCB_DAD_BUTTON(ctx.dlg, "insert row");
					PCB_DAD_CHANGE_CB(ctx.dlg, cb_ttbl_insert);
				PCB_DAD_BUTTON(ctx.dlg, "append row");
					PCB_DAD_CHANGE_CB(ctx.dlg, cb_ttbl_append);
			PCB_DAD_END(ctx.dlg);
		PCB_DAD_END(ctx.dlg);
	PCB_DAD_END(ctx.dlg);

	PCB_DAD_AUTORUN(ctx.dlg, "dlg_test", "attribute dialog test", &ctx);

	PCB_DAD_FREE(ctx.dlg);

	PCB_ACT_IRES(0);
	return 0;
}

static void pcb_act_attr_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	static pcb_hid_attr_val_t val;
	static pcb_bool st;
	printf("Chg\n");

	st = !st;
	val.int_value = (val.int_value + 1) % 3;
/*	pcb_gui->attr_dlg_widget_state(hid_ctx, attr_idx, st);*/

	pcb_gui->attr_dlg_set_value(hid_ctx, attr_idx, &val);
}

static void cb_tab_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	printf("Tab switch to %d!\n", ctx->dlg[ctx->wtab].default_val.int_value);
}

static void cb_jump(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_attr_val_t val;
	test_t *ctx = caller_data;

	printf("Jumping tabs\n");
	val.int_value = 0;
	pcb_gui->attr_dlg_set_value(hid_ctx, ctx->wtab, &val);
}

static void cb_ttbl_insert(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	pcb_hid_attribute_t *treea = &ctx->dlg[ctx->tt];
	char *rowdata[] = {NULL, "ins", "dummy", NULL};
	pcb_hid_row_t *new_row, *row = pcb_dad_tree_get_selected(treea);

	rowdata[0] = pcb_strdup_printf("dyn_%d", ctx->ttctr++);
	new_row = pcb_dad_tree_insert(treea, row, rowdata);
	new_row->user_data2.lng = 1;
}

static void cb_ttbl_append(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	pcb_hid_attribute_t *treea = &ctx->dlg[ctx->tt];
	char *rowdata[] = {NULL, "app", "dummy", NULL};
	pcb_hid_row_t *new_row, *row = pcb_dad_tree_get_selected(treea);

	rowdata[0] = pcb_strdup_printf("dyn_%d", ctx->ttctr++);
	new_row = pcb_dad_tree_append(treea, row, rowdata);
	new_row->user_data2.lng = 1;
}

static void cb_ttbl_select(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(attr);
	if (attr->default_val.str_value != NULL)
		pcb_trace("tt selected: path=%s row=%p '%s'\n", attr->default_val.str_value, row, row->cell[0]);
	else
		pcb_trace("tt selected: <NONE>\n");
}

static void cb_ttbl_free_row(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	if (row->user_data2.lng)
		free(row->cell[0]);
}
