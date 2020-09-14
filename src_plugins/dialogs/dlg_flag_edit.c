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

#include "config.h"
#include <librnd/core/actions.h>
#include <librnd/core/hid_dad.h>
#include "flag.h"
#include "flag_str.h"
#include "change.h"
#include "undo.h"
#include "search.h"
#include "funchash_core.h"
#include "dlg_flag_edit.h"

typedef struct{
	unsigned long flag_bit[64];
	int wid[64];
	int len;
	pcb_board_t *pcb;
	int obj_type;
	void *ptr1;
	pcb_any_obj_t *obj;
	rnd_hid_attribute_t *attrs;
} fe_ctx_t;

#define PCB_FLAGEDIT_TYPES \
	(PCB_OBJ_PSTK | PCB_OBJ_LINE | PCB_OBJ_POLY | \
	PCB_OBJ_TEXT | PCB_OBJ_SUBC | PCB_OBJ_ARC | PCB_OBJ_GFX)

static void fe_attr_chg(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int n;
	fe_ctx_t *ctx = caller_data;
	unsigned long set = 0, clr = 0;

	for(n = 0; n < ctx->len; n++) {
		int wid = ctx->wid[n];
		if ((ctx->attrs[wid].val.lng) && (!PCB_FLAG_TEST(ctx->flag_bit[n], ctx->obj)))
			set |= ctx->flag_bit[n];
		else if (!(ctx->attrs[wid].val.lng) && PCB_FLAG_TEST(ctx->flag_bit[n], ctx->obj))
			clr |= ctx->flag_bit[n];
	}

	if ((set == 0) && (clr == 0))
		return;

	/* Note: this function is called upon each change so only one of these will be non-zero: */

	if (set || clr)
		pcb_undo_add_obj_to_flag(ctx->obj);


	if (set != 0)
		pcb_flag_change(ctx->pcb, PCB_CHGFLG_SET, set, ctx->obj_type, ctx->ptr1, ctx->obj, ctx->obj);

	if (clr != 0)
		pcb_flag_change(ctx->pcb, PCB_CHGFLG_CLEAR, clr, ctx->obj_type, ctx->ptr1, ctx->obj, ctx->obj);

	rnd_gui->invalidate_all(rnd_gui);
}



const char pcb_acts_FlagEdit[] = "FlagEdit(object)\n";
const char pcb_acth_FlagEdit[] = "Change the layer binding.";
fgw_error_t pcb_act_FlagEdit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = F_Object;
	fe_ctx_t ctx;
	rnd_hid_attr_val_t val;
	int type;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	memset(&ctx, 0, sizeof(ctx));

	RND_ACT_MAY_CONVARG(1, FGW_KEYWORD, FlagEdit, op = fgw_keyword(&argv[1]));

	if (op == F_Object) {
		rnd_coord_t x, y;
		void *ptr1, *ptr2, *ptr3;
		rnd_hid_get_coords("Click on object to change flags of", &x, &y, 0);
		type = pcb_search_screen(x, y, PCB_FLAGEDIT_TYPES | PCB_LOOSE_SUBC(PCB), &ptr1, &ptr2, &ptr3);
		ctx.ptr1 = ptr1;
		ctx.obj = (pcb_any_obj_t *)ptr2;
		ctx.obj_type = (type & 0xFFFF) | (type & PCB_OBJ_PSTK);
	}
	else
		RND_ACT_FAIL(FlagEdit);

	if ((ctx.obj_type != 0) && (PCB_FLAG_TEST(PCB_FLAG_LOCK, ctx.obj))) {
		rnd_message(RND_MSG_ERROR, "Can't edit the flags of a locked object, unlock first.\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	if (ctx.obj_type != 0) { /* interactive mode */
		int n;
		char tmp[128];
		RND_DAD_DECL(dlg);

		ctx.pcb = PCB;
		ctx.len = 0;

		pcb_undo_save_serial();

		RND_DAD_BEGIN_VBOX(dlg);
			RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
			sprintf(tmp, "Object flags of %s #%ld\n", pcb_obj_type_name(ctx.obj_type), ctx.obj->ID);
			RND_DAD_LABEL(dlg, tmp);

			for(n = 0; n < pcb_object_flagbits_len; n++) {
				if (pcb_object_flagbits[n].object_types & ctx.obj_type) {
					RND_DAD_BEGIN_HBOX(dlg);
						RND_DAD_BOOL(dlg);
							RND_DAD_HELP(dlg, pcb_object_flagbits[n].help);
						ctx.wid[ctx.len] = RND_DAD_CURRENT(dlg);
						ctx.flag_bit[ctx.len] = pcb_object_flagbits[n].mask;
						if (PCB_FLAG_TEST(ctx.flag_bit[ctx.len], ctx.obj))
							RND_DAD_DEFAULT_NUM(dlg, 1);
						RND_DAD_LABEL(dlg, pcb_object_flagbits[n].name);
						ctx.len++;
					RND_DAD_END(dlg);
				}
			}
			RND_DAD_BUTTON_CLOSES(dlg, clbtn);
		RND_DAD_END(dlg);

		ctx.attrs = dlg;

		RND_DAD_NEW("flags", dlg, "Edit flags", &ctx, rnd_true, NULL);

		val.func = fe_attr_chg;
		rnd_gui->attr_dlg_property(dlg_hid_ctx, RND_HATP_GLOBAL_CALLBACK, &val);

		RND_DAD_RUN(dlg);

		RND_DAD_FREE(dlg);
		pcb_undo_restore_serial();
		pcb_undo_inc_serial();
	}

	RND_ACT_IRES(0);
	return 0;
}
