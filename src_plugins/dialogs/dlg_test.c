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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "board.h"
#include "obj_text.h"
#include <librnd/core/hid_dad_tree.h>
#include <librnd/core/hid_dad_spin.h>

static const char dlg_test_syntax[] = "dlg_test()\n";
static const char dlg_test_help[] = "test the attribute dialog";

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int wtab, tt, wprog, whpane, wvpane, wtxt, wtxtpos, wtxtro;
	int ttctr, wclr, txtro;
	int wspin_int, wspout_int, wspin_double, wspout_double, wspin_coord, wspout_coord;
} test_t;


static void pcb_act_attr_chg(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void pcb_act_spin_reset(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void pcb_act_spin_upd(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_tab_chg(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_jump(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_color_print(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_color_reset(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_ttbl_insert(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_ttbl_append(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_ttbl_jump(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_ttbl_filt(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_ttbl_select(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_ttbl_row_selected(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row);
static void cb_ttbl_free_row(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row);
static void cb_pane_set(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_text_replace(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_text_insert(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_text_append(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_text_get(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_text_edit(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_text_offs(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);
static void cb_text_ro(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);

static void prv_expose(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e);
static rnd_bool prv_mouse(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y);

static const char * test_xpm[] = {
"8 8 4 1",
" 	c None",
"+	c #550000",
"@	c #ff0000",
"#	c #00ff00",
"  +##+  ",
"  +##+  ",
" ++@@++ ",
" +@  @+ ",
" +@##@+ ",
" +@  @+ ",
" ++@@++ ",
"  ++++  "
};


static int attr_idx, attr_idx2;
static fgw_error_t pcb_act_dlg_test(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *vals[] = { "foo", "bar", "baz", NULL };
	const char *tabs[] = { "original test", "new test", "tree-table", "pane", "preview", "text", NULL };
	char *row1[] = {"one", "foo", "FOO", NULL};
	char *row2[] = {"two", "bar", "BAR", NULL};
	char *row2b[] = {"under_two", "ut", "uuut", NULL};
	char *row3[] = {"three", "baz", "BAZ", NULL};
	const char *hdr[] = {"num", "data1", "data2", NULL};
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	rnd_hid_row_t *row;
	int failed;

	test_t ctx;
	memset(&ctx, 0, sizeof(ctx));

	RND_DAD_BEGIN_VBOX(ctx.dlg);
		RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABBED(ctx.dlg, tabs);
			RND_DAD_CHANGE_CB(ctx.dlg, cb_tab_chg);
			ctx.wtab = RND_DAD_CURRENT(ctx.dlg);

			/* tab 0: "original test" */
			RND_DAD_BEGIN_VBOX(ctx.dlg);
				RND_DAD_LABEL(ctx.dlg, "text1");
				RND_DAD_BEGIN_TABLE(ctx.dlg, 3);
					RND_DAD_LABEL(ctx.dlg, "text2a");
					RND_DAD_LABEL(ctx.dlg, "text2b");
					RND_DAD_LABEL(ctx.dlg, "text2c");
					RND_DAD_LABEL(ctx.dlg, "text2d");
				RND_DAD_END(ctx.dlg);
				RND_DAD_LABEL(ctx.dlg, "text3");

				RND_DAD_BEGIN_VBOX(ctx.dlg);
					RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_FRAME);
					RND_DAD_LABEL(ctx.dlg, "spin test");
					RND_DAD_BUTTON(ctx.dlg, "reset all to 42");
						RND_DAD_CHANGE_CB(ctx.dlg, pcb_act_spin_reset);

					RND_DAD_BEGIN_HBOX(ctx.dlg);
						RND_DAD_LABEL(ctx.dlg, "INT:");
						RND_DAD_SPIN_INT(ctx.dlg);
							ctx.wspin_int = RND_DAD_CURRENT(ctx.dlg);
							RND_DAD_DEFAULT_NUM(ctx.dlg, 42);
							RND_DAD_CHANGE_CB(ctx.dlg, pcb_act_spin_upd);
						RND_DAD_LABEL(ctx.dlg, "->");
						RND_DAD_LABEL(ctx.dlg, "n/a");
							ctx.wspout_int = RND_DAD_CURRENT(ctx.dlg);
					RND_DAD_END(ctx.dlg);

					RND_DAD_BEGIN_HBOX(ctx.dlg);
						RND_DAD_LABEL(ctx.dlg, "DBL:");
						RND_DAD_SPIN_DOUBLE(ctx.dlg);
							ctx.wspin_double = RND_DAD_CURRENT(ctx.dlg);
							RND_DAD_DEFAULT_NUM(ctx.dlg, 42);
							RND_DAD_CHANGE_CB(ctx.dlg, pcb_act_spin_upd);
						RND_DAD_LABEL(ctx.dlg, "->");
						RND_DAD_LABEL(ctx.dlg, "n/a");
							ctx.wspout_double = RND_DAD_CURRENT(ctx.dlg);
					RND_DAD_END(ctx.dlg);

					RND_DAD_BEGIN_HBOX(ctx.dlg);
						RND_DAD_LABEL(ctx.dlg, "CRD:");
						RND_DAD_SPIN_COORD(ctx.dlg);
							ctx.wspin_coord = RND_DAD_CURRENT(ctx.dlg);
							RND_DAD_DEFAULT_NUM(ctx.dlg, RND_MM_TO_COORD(42));
							RND_DAD_CHANGE_CB(ctx.dlg, pcb_act_spin_upd);
						RND_DAD_LABEL(ctx.dlg, "->");
						RND_DAD_LABEL(ctx.dlg, "n/a");
							ctx.wspout_coord = RND_DAD_CURRENT(ctx.dlg);
					RND_DAD_END(ctx.dlg);

				RND_DAD_END(ctx.dlg);

				RND_DAD_ENUM(ctx.dlg, vals);
					RND_DAD_CHANGE_CB(ctx.dlg, pcb_act_attr_chg);
					attr_idx = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_INTEGER(ctx.dlg, "text2e");
					RND_DAD_MINVAL(ctx.dlg, 1);
					RND_DAD_MAXVAL(ctx.dlg, 10);
					RND_DAD_DEFAULT_NUM(ctx.dlg, 3);
					RND_DAD_CHANGE_CB(ctx.dlg, pcb_act_attr_chg);
					attr_idx2 = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_BUTTON(ctx.dlg, "update!");
					RND_DAD_CHANGE_CB(ctx.dlg, pcb_act_attr_chg);
			RND_DAD_END(ctx.dlg);

			/* tab 1: "new test" */
			RND_DAD_BEGIN_VBOX(ctx.dlg);
				RND_DAD_LABEL(ctx.dlg, "new test.");
				RND_DAD_PICTURE(ctx.dlg, test_xpm);
				RND_DAD_BUTTON(ctx.dlg, "jump to the first tab");
					RND_DAD_CHANGE_CB(ctx.dlg, cb_jump);
				RND_DAD_PICBUTTON(ctx.dlg, test_xpm);
					RND_DAD_CHANGE_CB(ctx.dlg, cb_color_reset);
				RND_DAD_COLOR(ctx.dlg);
					RND_DAD_CHANGE_CB(ctx.dlg, cb_color_print);
					ctx.wclr = RND_DAD_CURRENT(ctx.dlg);
			RND_DAD_END(ctx.dlg);


			/* tab 2: tree table widget */
			RND_DAD_BEGIN_VBOX(ctx.dlg);
				RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(ctx.dlg, 3, 1, hdr);
					ctx.tt = RND_DAD_CURRENT(ctx.dlg);
					RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_SCROLL);
					RND_DAD_CHANGE_CB(ctx.dlg, cb_ttbl_select);
					RND_DAD_TREE_SET_CB(ctx.dlg, free_cb, cb_ttbl_free_row);
					RND_DAD_TREE_SET_CB(ctx.dlg, selected_cb, cb_ttbl_row_selected);
					RND_DAD_TREE_APPEND(ctx.dlg, NULL, row1);
					row = RND_DAD_TREE_APPEND(ctx.dlg, NULL, row2);
					RND_DAD_TREE_APPEND_UNDER(ctx.dlg, row, row2b);
					RND_DAD_TREE_APPEND(ctx.dlg, NULL, row3);
				RND_DAD_BEGIN_HBOX(ctx.dlg);
					RND_DAD_BUTTON(ctx.dlg, "insert row");
						RND_DAD_CHANGE_CB(ctx.dlg, cb_ttbl_insert);
					RND_DAD_BUTTON(ctx.dlg, "append row");
						RND_DAD_CHANGE_CB(ctx.dlg, cb_ttbl_append);
					RND_DAD_BUTTON(ctx.dlg, "jump!");
						RND_DAD_CHANGE_CB(ctx.dlg, cb_ttbl_jump);
					RND_DAD_BOOL(ctx.dlg, "filter");
						RND_DAD_CHANGE_CB(ctx.dlg, cb_ttbl_filt);
				RND_DAD_END(ctx.dlg);
				RND_DAD_BEGIN_VBOX(ctx.dlg);
					RND_DAD_PROGRESS(ctx.dlg);
						ctx.wprog = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_END(ctx.dlg);
			RND_DAD_END(ctx.dlg);

			/* tab 3: pane */
			RND_DAD_BEGIN_HPANE(ctx.dlg);
				ctx.whpane = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_BEGIN_VBOX(ctx.dlg);
					RND_DAD_LABEL(ctx.dlg, "left1");
					RND_DAD_LABEL(ctx.dlg, "left2");
				RND_DAD_END(ctx.dlg);
				RND_DAD_BEGIN_VPANE(ctx.dlg);
					ctx.wvpane = RND_DAD_CURRENT(ctx.dlg);
					RND_DAD_BEGIN_VBOX(ctx.dlg);
						RND_DAD_LABEL(ctx.dlg, "right top1");
						RND_DAD_LABEL(ctx.dlg, "right top2");
					RND_DAD_END(ctx.dlg);
					RND_DAD_BEGIN_VBOX(ctx.dlg);
						RND_DAD_LABEL(ctx.dlg, "right bottom1");
						RND_DAD_LABEL(ctx.dlg, "right bottom2");
						RND_DAD_BUTTON(ctx.dlg, "set all to 30%");
							RND_DAD_CHANGE_CB(ctx.dlg, cb_pane_set);
					RND_DAD_END(ctx.dlg);
				RND_DAD_END(ctx.dlg);
			RND_DAD_END(ctx.dlg);

			/* tab 4: preview */
			RND_DAD_BEGIN_VBOX(ctx.dlg);
				RND_DAD_PREVIEW(ctx.dlg, prv_expose, prv_mouse, NULL, NULL, 200, 200, NULL);
				RND_DAD_LABEL(ctx.dlg, "This is a cool preview widget.");
			RND_DAD_END(ctx.dlg);

			/* tab 5: text */
			RND_DAD_BEGIN_VBOX(ctx.dlg);
				RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_TEXT(ctx.dlg, NULL);
					RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_SCROLL | RND_HATF_EXPFILL);
					RND_DAD_CHANGE_CB(ctx.dlg, cb_text_edit);
					ctx.wtxt = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_BEGIN_HBOX(ctx.dlg);
					RND_DAD_LABEL(ctx.dlg, "<pos>");
						ctx.wtxtpos = RND_DAD_CURRENT(ctx.dlg);
						RND_DAD_BUTTON(ctx.dlg, "half the offset");
							RND_DAD_CHANGE_CB(ctx.dlg, cb_text_offs);
				RND_DAD_END(ctx.dlg);
				RND_DAD_BEGIN_HBOX(ctx.dlg);
					RND_DAD_BUTTON(ctx.dlg, "replace");
						RND_DAD_CHANGE_CB(ctx.dlg, cb_text_replace);
					RND_DAD_BUTTON(ctx.dlg, "insert");
						RND_DAD_CHANGE_CB(ctx.dlg, cb_text_insert);
					RND_DAD_BUTTON(ctx.dlg, "append");
						RND_DAD_CHANGE_CB(ctx.dlg, cb_text_append);
					RND_DAD_BUTTON(ctx.dlg, "get");
						RND_DAD_CHANGE_CB(ctx.dlg, cb_text_get);
					RND_DAD_BUTTON(ctx.dlg, "ro");
						ctx.txtro = 0;
						RND_DAD_CHANGE_CB(ctx.dlg, cb_text_ro);
				RND_DAD_END(ctx.dlg);
			RND_DAD_END(ctx.dlg);

		RND_DAD_END(ctx.dlg);
		RND_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
	RND_DAD_END(ctx.dlg);

	RND_DAD_AUTORUN("dlg_test", ctx.dlg, "attribute dialog test", &ctx, failed);

	if (failed != 0)
		rnd_message(RND_MSG_WARNING, "Test dialog cancelled");

	RND_DAD_FREE(ctx.dlg);

	RND_ACT_IRES(0);
	return 0;
}

static void pcb_act_attr_chg(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	static rnd_hid_attr_val_t val;
	static rnd_bool st;
	printf("Chg\n");

	st = !st;
	val.lng = (val.lng + 1) % 3;
/*	rnd_gui->attr_dlg_widget_state(hid_ctx, attr_idx, st);*/

	rnd_gui->attr_dlg_set_value(hid_ctx, attr_idx, &val);
}

static void pcb_act_spin_reset(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attr_val_t hv;

	hv.lng = 42;
	hv.dbl = 42.0;
	hv.crd = RND_MM_TO_COORD(42);

	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wspin_int, &hv);
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wspin_double, &hv);
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wspin_coord, &hv);
}

static void pcb_act_spin_upd(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attr_val_t hv;
	char tmp[256];

	hv.str = tmp;

	sprintf(tmp, "%ld", ctx->dlg[ctx->wspin_int].val.lng);
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wspout_int, &hv);
	sprintf(tmp, "%f", ctx->dlg[ctx->wspin_double].val.dbl);
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wspout_double, &hv);
	rnd_snprintf(tmp, sizeof(tmp), "%mm\n%ml", ctx->dlg[ctx->wspin_coord].val.crd, ctx->dlg[ctx->wspin_coord].val.crd);
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wspout_coord, &hv);
}


static void cb_tab_chg(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	printf("Tab switch to %ld!\n", ctx->dlg[ctx->wtab].val.lng);
}

static void cb_jump(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_attr_val_t val;
	test_t *ctx = caller_data;

	printf("Jumping tabs\n");
	val.lng = 0;
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wtab, &val);
}

static void cb_ttbl_insert(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attribute_t *treea = &ctx->dlg[ctx->tt];
	char *rowdata[] = {NULL, "ins", "dummy", NULL};
	rnd_hid_row_t *new_row, *row = rnd_dad_tree_get_selected(treea);
	rnd_hid_attr_val_t val;

	rowdata[0] = rnd_strdup_printf("dyn_%d", ctx->ttctr++);
	new_row = rnd_dad_tree_insert(treea, row, rowdata);
	new_row->user_data2.lng = 1;

	val.dbl = (double)ctx->ttctr / 20.0;
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wprog, &val);
}

static void cb_ttbl_append(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attribute_t *treea = &ctx->dlg[ctx->tt];
	char *rowdata[] = {NULL, "app", "dummy", NULL};
	rnd_hid_row_t *new_row, *row = rnd_dad_tree_get_selected(treea);
	rnd_hid_attr_val_t val;

	rowdata[0] = rnd_strdup_printf("dyn_%d", ctx->ttctr++);
	new_row = rnd_dad_tree_append(treea, row, rowdata);
	new_row->user_data2.lng = 1;

	val.dbl = (double)ctx->ttctr / 20.0;
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wprog, &val);
}

static void cb_ttbl_jump(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attr_val_t val;

	val.str = "two/under_two";
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->tt, &val);
}

static void ttbl_filt(gdl_list_t *list, int hide)
{
	rnd_hid_row_t *r;
	for(r = gdl_first(list); r != NULL; r = gdl_next(list, r)) {
		if (r->user_data2.lng)
			r->hide = hide;
		ttbl_filt(&r->children, hide);
	}
}

static void cb_ttbl_filt(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attribute_t *treea = &ctx->dlg[ctx->tt];
	rnd_hid_tree_t *tree = treea->wdata;

	ttbl_filt(&tree->rows, attr->val.lng);
	rnd_dad_tree_update_hide(treea);
}

/* table level selection */
static void cb_ttbl_select(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(attr);
	if (attr->val.str != NULL)
		rnd_trace("tt tbl selected: path=%s row=%p '%s'\n", attr->val.str, row, row->cell[0]);
	else
		rnd_trace("tt tbl selected: <NONE>\n");
}

/* row level selection */
static void cb_ttbl_row_selected(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	if (row != NULL)
		rnd_trace("tt row selected: row=%p '%s'\n", row, row->cell[0]);
	else
		rnd_trace("tt row selected: <NONE>\n");
}

static void cb_ttbl_free_row(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	if (row->user_data2.lng)
		free(row->cell[0]);
}

static void cb_pane_set(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attr_val_t val;

	val.dbl = 0.3;
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->whpane, &val);
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wvpane, &val);
}

static void cb_text_replace(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;
	txt->hid_set_text(atxt, hid_ctx, RND_HID_TEXT_REPLACE, "Hello\nworld!\n");
}

static void cb_text_insert(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;
	txt->hid_set_text(atxt, hid_ctx, RND_HID_TEXT_INSERT, "ins");
}

static void cb_text_append(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;
	txt->hid_set_text(atxt, hid_ctx, RND_HID_TEXT_APPEND | RND_HID_TEXT_MARKUP, "app<R>red</R>\n");
}

static void cb_text_get(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;
	char *s;
	s = txt->hid_get_text(atxt, hid_ctx);
	printf("text: '%s'\n", s);
	free(s);
}

static void cb_text_edit(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_text_t *txt = attr->wdata;
	long x, y, o;
	char buf[256];
	rnd_hid_attr_val_t val;

	txt->hid_get_xy(attr, hid_ctx, &x, &y);
	o = txt->hid_get_offs(attr, hid_ctx);
	sprintf(buf, "cursor after edit: line %ld col %ld offs %ld", y, x, o);
	val.str = buf;
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wtxtpos, &val);
}

static void cb_text_offs(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;
	txt->hid_set_offs(atxt, hid_ctx, txt->hid_get_offs(atxt, hid_ctx) / 2);
}

static void cb_text_ro(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
	rnd_hid_text_t *txt = atxt->wdata;
	ctx->txtro = !ctx->txtro;
	txt->hid_set_readonly(atxt, hid_ctx, ctx->txtro);
}

static void prv_expose(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	rnd_render->set_color(gc, rnd_color_red);
	pcb_text_draw_string_simple(NULL, "foo", RND_MM_TO_COORD(1), RND_MM_TO_COORD(20), 5.0, 5.0, 10.0, 0, 0, 0, 0, 0);

	printf("expose in dlg_test!\n");
}


static rnd_bool prv_mouse(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	rnd_printf("Mouse %d %mm %mm\n", kind, x, y);
	return (kind == RND_HID_MOUSE_PRESS) || (kind == RND_HID_MOUSE_RELEASE);
}

static void cb_color_print(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;

	printf("currenct color: #%02x%02x%02x\n",
		ctx->dlg[ctx->wclr].val.clr.r, ctx->dlg[ctx->wclr].val.clr.g, ctx->dlg[ctx->wclr].val.clr.b);
}

static void cb_color_reset(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	test_t *ctx = caller_data;
	rnd_hid_attr_val_t val;

	rnd_color_load_str(&val.clr, "#005599");
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wclr, &val);
}

