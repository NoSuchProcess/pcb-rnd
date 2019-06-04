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

#include "obj_pstk.h"
#include "obj_pstk_inlines.h"

#include "funchash_core.h"
#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "actions.h"
#include "search.h"
#include "data_list.h"

static const char pcb_acts_padstackconvert[] = "PadstackConvert(buffer|selected, [originx, originy])";
static const char pcb_acth_padstackconvert[] = "Convert selection or current buffer to padstack";
fgw_error_t pcb_act_padstackconvert(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	pcb_coord_t x, y;
	pcb_cardinal_t pid;
	pcb_pstk_proto_t tmp, *p;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, padstackconvert, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_Selected:
		if (argc > 3) {
			PCB_ACT_CONVARG(2, FGW_COORD, padstackconvert, x = fgw_coord(&argv[2]));
			PCB_ACT_CONVARG(3, FGW_COORD, padstackconvert, y = fgw_coord(&argv[3]));
		}
		else {
			pcb_hid_get_coords("Click at padstack origin", &x, &y, 0);
			/* rather use the snapped corsshair coords */
			x = pcb_crosshair.X;
			y = pcb_crosshair.Y;
		}
		pid = pcb_pstk_conv_selection(PCB, 0, x, y);

		if (pid != PCB_PADSTACK_INVALID) {
			pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
			p = pcb_vtpadstack_proto_alloc_append(&PCB_PASTEBUFFER->Data->ps_protos, 1);
			pcb_pstk_proto_copy(p, &PCB->Data->ps_protos.array[pid]);
			p->parent = PCB_PASTEBUFFER->Data;
			pid = pcb_pstk_get_proto_id(p); /* should be 0 because of the clear, but just in case... */
		}
		break;
		case F_Buffer:
		pid = pcb_pstk_conv_buffer(0);

		if (pid != PCB_PADSTACK_INVALID) {
			/* have to save and restore the prototype around the buffer clear */
			tmp = PCB_PASTEBUFFER->Data->ps_protos.array[pid];
			memset(&PCB_PASTEBUFFER->Data->ps_protos.array[pid], 0, sizeof(PCB_PASTEBUFFER->Data->ps_protos.array[0]));
			pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
			p = pcb_vtpadstack_proto_alloc_append(&PCB_PASTEBUFFER->Data->ps_protos, 1);
			*p = tmp;
			p->parent = PCB_PASTEBUFFER->Data;
			pid = pcb_pstk_get_proto_id(p); /* should be 0 because of the clear, but just in case... */
		}
		break;
		default:
			PCB_ACT_FAIL(padstackconvert);
	}

	if (pid != PCB_PADSTACK_INVALID) {
		pcb_message(PCB_MSG_INFO, "Pad stack registered with ID %d\n", pid);
		pcb_pstk_new(PCB_PASTEBUFFER->Data, -1, pid, 0, 0, conf_core.design.clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
		pcb_set_buffer_bbox(PCB_PASTEBUFFER);
		PCB_PASTEBUFFER->X = PCB_PASTEBUFFER->Y = 0;
		PCB_ACT_IRES(0);
	}
	else {
		pcb_message(PCB_MSG_ERROR, "(failed to convert to padstack)\n", pid);
		PCB_ACT_IRES(1);
	}

	return 0;
}

static const char pcb_acts_padstackbreakup[] = "PadstackBreakup(buffer|selected|objet)";
static const char pcb_acth_padstackbreakup[] = "Break up a padstack into one non-padstack object per layer type (the hole is ignored)";
fgw_error_t pcb_act_padstackbreakup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	PCB_ACT_CONVARG(1, FGW_KEYWORD, padstackconvert, op = fgw_keyword(&argv[1]));
	PCB_ACT_IRES(-1);

	switch(op) {
		case F_Object:
			{
				void *ptr1, *ptr2, *ptr3;
				pcb_pstk_t *ps;
				pcb_objtype_t type;
				pcb_coord_t x, y;
				
				pcb_hid_get_coords("Select a padstack to break up", &x, &y, 0);
				if ((type = pcb_search_screen(x, y, PCB_OBJ_PSTK, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_PSTK) {
					pcb_message(PCB_MSG_ERROR, "Need a padstack under the cursor\n");
					break;
				}
				ps = (pcb_pstk_t *)ptr2;
				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *)ps)) {
					pcb_message(PCB_MSG_ERROR, "Sorry, that padstack is locked\n");
					break;
				}
				PCB_ACT_IRES(pcb_pstk_proto_breakup(PCB->Data, ps, 1));
			}
			break;
		case F_Selected:
			{
				pcb_cardinal_t n;
				int ret = 0;
				vtp0_t objs;
				pcb_any_obj_t **o;

				vtp0_init(&objs);
				pcb_data_list_by_flag(PCB->Data, &objs, PCB_OBJ_PSTK, PCB_FLAG_SELECTED);
				for(n = 0, o = (pcb_any_obj_t **)objs.array; n < vtp0_len(&objs); n++,o++)
					ret |= pcb_pstk_proto_breakup(PCB->Data, (pcb_pstk_t *)*o, 1);
				PCB_ACT_IRES(ret);
				vtp0_uninit(&objs);
			}
			break;
		case F_Buffer:
			{
				int ret = 0;
				PCB_PADSTACK_LOOP(PCB_PASTEBUFFER->Data) {
					ret |= pcb_pstk_proto_breakup(PCB_PASTEBUFFER->Data, padstack, 1);
				} PCB_END_LOOP;
				PCB_ACT_IRES(ret);
			}
			break;
		default:
			PCB_ACT_FAIL(padstackbreakup);
	}
	return 0;
}

static const char pcb_acts_padstackplace[] = "PadstackPlace([proto_id|default], [x, y])";
static const char pcb_acth_padstackplace[] = "Place a pad stack (either proto_id, or if not specified, the default for style)";
fgw_error_t pcb_act_padstackplace(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *pids = NULL;
	pcb_cardinal_t pid;
	pcb_pstk_t *ps;
	pcb_coord_t x, y;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, padstackplace, pids = argv[1].val.str);

	if (argc > 3) {
		PCB_ACT_CONVARG(2, FGW_COORD, padstackconvert, x = fgw_coord(&argv[2]));
		PCB_ACT_CONVARG(3, FGW_COORD, padstackconvert, y = fgw_coord(&argv[3]));
	}
	else {
		pcb_hid_get_coords("Click at padstack origin", &x, &y, 0);
		/* rather use the snapped corsshair coords */
		x = pcb_crosshair.X;
		y = pcb_crosshair.Y;
	}

	if ((pids == NULL) || (strcmp(pids, "default") == 0)) {
TODO("pstk: style default proto")
		pid = 0;
	}
	else {
		char *end;
		pid = strtol(pids, &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "Error in proto ID format: need an integer\n");
			return -1;
		}
	}

	if ((pid >= PCB->Data->ps_protos.used) || (PCB->Data->ps_protos.array[pid].in_use == 0)) {
		pcb_message(PCB_MSG_ERROR, "Invalid padstack proto %ld\n", (long)pid);
		return -1;
	}

	ps = pcb_pstk_new(PCB->Data, -1, pid, x, y, conf_core.design.clearance, pcb_no_flags());
	if (ps == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to place padstack\n");
		return -1;
	}

	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

pcb_action_t padstack_action_list[] = {
	{"PadstackConvert", pcb_act_padstackconvert, pcb_acth_padstackconvert, pcb_acts_padstackconvert},
	{"PadstackBreakup", pcb_act_padstackbreakup, pcb_acth_padstackbreakup, pcb_acts_padstackbreakup},
	{"PadstackPlace", pcb_act_padstackplace, pcb_acth_padstackplace, pcb_acts_padstackplace}
};

PCB_REGISTER_ACTIONS(padstack_action_list, NULL)
