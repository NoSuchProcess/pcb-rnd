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

typedef struct{
	unsigned long flag_bit[64];
	int wid[64];
	int len;
	pcb_board_t *pcb;
	pcb_any_obj_t *obj;
	pcb_hid_attribute_t *attrs;
} fe_ctx_t;

#define PCB_FLAGEDIT_TYPES \
	(PCB_TYPE_VIA | PCB_TYPE_ELEMENT | PCB_TYPE_LINE | PCB_TYPE_POLYGON | \
	PCB_TYPE_TEXT | PCB_TYPE_SUBC | PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_ARC)

static void fe_attr_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	fe_ctx_t *ctx = caller_data;
}



static const char pcb_acts_FlagEdit[] = "FlagEdit(object)\n";
static const char pcb_acth_FlagEdit[] = "Change the layer binding.";
static int pcb_act_FlagEdit(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	fe_ctx_t ctx;
	pcb_hid_attr_val_t val;
	int type, objtype;

	memset(&ctx, 0, sizeof(ctx));

	if ((argc == 0) || (pcb_strcasecmp(argv[0], "object") == 0)) {
		void *ptr1, *ptr2, *ptr3;
		pcb_gui->get_coords("Click on object to change size of", &x, &y);
		type = pcb_search_screen(x, y, PCB_FLAGEDIT_TYPES, &ptr1, &ptr2, &ptr3);
		ctx.obj = (pcb_any_obj_t *)ptr2;
	}
	else
		PCB_ACT_FAIL(FlagEdit);

	objtype = type & 0xFFFF;

	if (objtype != 0) { /* interactive mode */
		int n;
		char tmp[128];

		PCB_DAD_DECL(dlg);

		ctx.pcb = PCB;
		ctx.len = 0;

		PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_COMPFLAG(dlg, PCB_HATF_LABEL);

		sprintf(tmp, "Object flags of %x #%ld\n", objtype, ctx.obj->ID);
		PCB_DAD_LABEL(dlg, tmp);

		for(n = 0; n < pcb_object_flagbits_len; n++) {
			if (pcb_object_flagbits[n].object_types & objtype) {
				printf("name=%s mask=%x\n", pcb_object_flagbits[n].name, pcb_object_flagbits[n].mask);
				PCB_DAD_BOOL(dlg, pcb_object_flagbits[n].name);
				ctx.wid[ctx.len] = PCB_DAD_CURRENT(dlg);
				ctx.flag_bit[ctx.len] = pcb_object_flagbits[n].mask;
				if (PCB_FLAG_TEST(ctx.flag_bit[ctx.len], ctx.obj))
					PCB_DAD_SET_VALUE(dlg, ctx.wid[ctx.len], int_value, 1);
			
				ctx.len++;
			}
		}
		PCB_DAD_END(dlg);

		ctx.attrs = dlg;

		PCB_DAD_NEW(dlg, "flag_edit", "Edit flags", &ctx);
		val.func = fe_attr_chg;
		pcb_gui->attr_dlg_property(dlg_hid_ctx, PCB_HATP_GLOBAL_CALLBACK, &val);

		PCB_DAD_RUN(dlg);

		PCB_DAD_FREE(dlg);
	}

	return 0;
}
