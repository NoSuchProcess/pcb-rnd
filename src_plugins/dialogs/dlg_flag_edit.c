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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "flag.h"
#include "flag_str.h"
#include "change.h"

typedef struct{
	unsigned long flag_bit[64];
	int wid[64];
	int len;
	pcb_board_t *pcb;
	int obj_type;
	void *ptr1;
	pcb_any_obj_t *obj;
	pcb_hid_attribute_t *attrs;
} fe_ctx_t;

#define PCB_FLAGEDIT_TYPES \
	(PCB_TYPE_VIA | PCB_TYPE_ELEMENT | PCB_TYPE_LINE | PCB_TYPE_POLY | \
	PCB_TYPE_TEXT | PCB_TYPE_SUBC | PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_ARC)

static void fe_attr_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	int n;
	fe_ctx_t *ctx = caller_data;
	unsigned long set = 0, clr = 0;

	for(n = 0; n < ctx->len; n++) {
		int wid = ctx->wid[n];
		if ((ctx->attrs[wid].default_val.int_value) && (!PCB_FLAG_TEST(ctx->flag_bit[n], ctx->obj)))
			set |= ctx->flag_bit[n];
		else if (!(ctx->attrs[wid].default_val.int_value) && PCB_FLAG_TEST(ctx->flag_bit[n], ctx->obj))
			clr |= ctx->flag_bit[n];
	}

	if ((set == 0) && (clr == 0))
		return;

	/* Note: this function is called upon each change so only one of these will be non-zero: */

	if (set != 0)
		pcb_flag_change(ctx->pcb, PCB_CHGFLG_SET, set, ctx->obj_type, ctx->ptr1, ctx->obj, ctx->obj);

	if (clr != 0)
		pcb_flag_change(ctx->pcb, PCB_CHGFLG_CLEAR, clr, ctx->obj_type, ctx->ptr1, ctx->obj, ctx->obj);

	pcb_gui->invalidate_all();
}



static const char pcb_acts_FlagEdit[] = "FlagEdit(object)\n";
static const char pcb_acth_FlagEdit[] = "Change the layer binding.";
static int pcb_act_FlagEdit(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	fe_ctx_t ctx;
	pcb_hid_attr_val_t val;
	int type;

	memset(&ctx, 0, sizeof(ctx));

	if ((argc == 0) || (pcb_strcasecmp(argv[0], "object") == 0)) {
		void *ptr1, *ptr2, *ptr3;
		pcb_gui->get_coords("Click on object to change flags of", &x, &y);
		type = pcb_search_screen(x, y, PCB_FLAGEDIT_TYPES, &ptr1, &ptr2, &ptr3);
		ctx.ptr1 = ptr1;
		ctx.obj = (pcb_any_obj_t *)ptr2;
		ctx.obj_type = type & 0xFFFF;
	}
	else
		PCB_ACT_FAIL(FlagEdit);

	if (ctx.obj_type != 0) { /* interactive mode */
		int n;
		char tmp[128];
		PCB_DAD_DECL(dlg);

		ctx.pcb = PCB;
		ctx.len = 0;

		PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_COMPFLAG(dlg, PCB_HATF_LABEL);

		sprintf(tmp, "Object flags of %x #%ld\n", ctx.obj_type, ctx.obj->ID);
		PCB_DAD_LABEL(dlg, tmp);

		for(n = 0; n < pcb_object_flagbits_len; n++) {
			if (pcb_object_flagbits[n].object_types & ctx.obj_type) {
				PCB_DAD_BOOL(dlg, pcb_object_flagbits[n].name);
				PCB_DAD_HELP(dlg, pcb_object_flagbits[n].help);
				ctx.wid[ctx.len] = PCB_DAD_CURRENT(dlg);
				ctx.flag_bit[ctx.len] = pcb_object_flagbits[n].mask;
				if (PCB_FLAG_TEST(ctx.flag_bit[ctx.len], ctx.obj))
					PCB_DAD_DEFAULT(dlg, 1);
				ctx.len++;
			}
		}
		PCB_DAD_END(dlg);

		ctx.attrs = dlg;

		PCB_DAD_NEW(dlg, "flag_edit", "Edit flags", &ctx, pcb_true);

		val.func = fe_attr_chg;
		pcb_gui->attr_dlg_property(dlg_hid_ctx, PCB_HATP_GLOBAL_CALLBACK, &val);

		PCB_DAD_RUN(dlg);

		PCB_DAD_FREE(dlg);
	}

	return 0;
}
