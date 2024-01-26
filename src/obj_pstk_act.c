/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2024 Tibor 'Igor2' Palinkas
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
#include <librnd/core/actions.h>
#include <librnd/core/rotate.h>
#include "search.h"
#include "data_list.h"
#include "undo.h"
#include "draw.h"
#include "move.h"

#define PCB do_not_use_PCB

static const char pcb_acts_padstackconvert[] = "PadstackConvert(buffer|selected, [originx, originy])";
static const char pcb_acth_padstackconvert[] = "Convert selection or current buffer to padstack";
fgw_error_t pcb_act_padstackconvert(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	rnd_coord_t x, y;
	rnd_cardinal_t pid;
	pcb_pstk_proto_t tmp, *p;
	pcb_board_t *pcb = PCB_ACT_BOARD;

	RND_ACT_CONVARG(1, FGW_KEYWORD, padstackconvert, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_Selected:
		if (argc > 3) {
			RND_ACT_CONVARG(2, FGW_COORD, padstackconvert, x = fgw_coord(&argv[2]));
			RND_ACT_CONVARG(3, FGW_COORD, padstackconvert, y = fgw_coord(&argv[3]));
		}
		else {
			rnd_hid_get_coords("Click at padstack origin", &x, &y, 0);
			/* rather use the snapped corsshair coords */
			x = pcb_crosshair.X;
			y = pcb_crosshair.Y;
		}
		pid = pcb_pstk_conv_selection(pcb, 0, x, y);

		if (pid != PCB_PADSTACK_INVALID) {
			pcb_buffer_clear(pcb, PCB_PASTEBUFFER);
			p = pcb_vtpadstack_proto_alloc_append(&PCB_PASTEBUFFER->Data->ps_protos, 1);
			pcb_pstk_proto_copy(p, &pcb->Data->ps_protos.array[pid]);
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
			pcb_buffer_clear(pcb, PCB_PASTEBUFFER);
			p = pcb_vtpadstack_proto_alloc_append(&PCB_PASTEBUFFER->Data->ps_protos, 1);
			*p = tmp;
			p->parent = PCB_PASTEBUFFER->Data;
			pid = pcb_pstk_get_proto_id(p); /* should be 0 because of the clear, but just in case... */
		}
		break;
		default:
			RND_ACT_FAIL(padstackconvert);
	}

	if (pid != PCB_PADSTACK_INVALID) {
		rnd_message(RND_MSG_INFO, "Pad stack registered with ID %d\n", pid);
		pcb_pstk_new(PCB_PASTEBUFFER->Data, -1, pid, 0, 0, conf_core.design.clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
		pcb_set_buffer_bbox(PCB_PASTEBUFFER);
		PCB_PASTEBUFFER->X = PCB_PASTEBUFFER->Y = 0;
		RND_ACT_IRES(0);
	}
	else {
		rnd_message(RND_MSG_ERROR, "(failed to convert to padstack)\n", pid);
		RND_ACT_IRES(1);
	}

	return 0;
}

static const char pcb_acts_padstackbreakup[] = "PadstackBreakup(buffer|selected|objet)";
static const char pcb_acth_padstackbreakup[] = "Break up a padstack into one non-padstack object per layer type (the hole is ignored)";
fgw_error_t pcb_act_padstackbreakup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int op;
	RND_ACT_CONVARG(1, FGW_KEYWORD, padstackconvert, op = fgw_keyword(&argv[1]));
	RND_ACT_IRES(-1);

	switch(op) {
		case F_Object:
			{
				void *ptr1, *ptr2, *ptr3;
				pcb_pstk_t *ps;
				pcb_objtype_t type;
				rnd_coord_t x, y;
				
				rnd_hid_get_coords("Select a padstack to break up", &x, &y, 0);
				if ((type = pcb_search_screen(x, y, PCB_OBJ_PSTK, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_PSTK) {
					rnd_message(RND_MSG_ERROR, "Need a padstack under the cursor\n");
					break;
				}
				ps = (pcb_pstk_t *)ptr2;
				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *)ps)) {
					rnd_message(RND_MSG_ERROR, "Sorry, that padstack is locked\n");
					break;
				}
				RND_ACT_IRES(pcb_pstk_proto_breakup(pcb->Data, ps, 1));
			}
			break;
		case F_Selected:
			{
				rnd_cardinal_t n;
				int ret = 0;
				vtp0_t objs;
				pcb_any_obj_t **o;

				vtp0_init(&objs);
				pcb_data_list_by_flag(pcb->Data, &objs, PCB_OBJ_PSTK, PCB_FLAG_SELECTED);
				for(n = 0, o = (pcb_any_obj_t **)objs.array; n < vtp0_len(&objs); n++,o++)
					ret |= pcb_pstk_proto_breakup(pcb->Data, (pcb_pstk_t *)*o, 1);
				RND_ACT_IRES(ret);
				vtp0_uninit(&objs);
			}
			break;
		case F_Buffer:
			{
				int ret = 0;
				PCB_PADSTACK_LOOP(PCB_PASTEBUFFER->Data) {
					ret |= pcb_pstk_proto_breakup(PCB_PASTEBUFFER->Data, padstack, 1);
				} PCB_END_LOOP;
				RND_ACT_IRES(ret);
			}
			break;
		default:
			RND_ACT_FAIL(padstackbreakup);
	}
	return 0;
}

static const char pcb_acts_padstackplace[] = "PadstackPlace([proto_id|default], [x, y])";
static const char pcb_acth_padstackplace[] = "Place a pad stack (either proto_id, or if not specified, the default for style)";
fgw_error_t pcb_act_padstackplace(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *pids = NULL;
	rnd_cardinal_t pid;
	pcb_pstk_t *ps;
	rnd_coord_t x, y;

	RND_ACT_MAY_CONVARG(1, FGW_STR, padstackplace, pids = argv[1].val.str);

	if (argc > 3) {
		RND_ACT_CONVARG(2, FGW_COORD, padstackconvert, x = fgw_coord(&argv[2]));
		RND_ACT_CONVARG(3, FGW_COORD, padstackconvert, y = fgw_coord(&argv[3]));
	}
	else {
		rnd_hid_get_coords("Click at padstack origin", &x, &y, 0);
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
			rnd_message(RND_MSG_ERROR, "Error in proto ID format: need an integer\n");
			return -1;
		}
	}

	if ((pid >= pcb->Data->ps_protos.used) || (pcb->Data->ps_protos.array[pid].in_use == 0)) {
		rnd_message(RND_MSG_ERROR, "Invalid padstack proto %ld\n", (long)pid);
		return -1;
	}

	ps = pcb_pstk_new(pcb->Data, -1, pid, x, y, conf_core.design.clearance, pcb_no_flags());
	if (ps == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to place padstack\n");
		return -1;
	}

	RND_ACT_IRES(0);
	return 0;
}

static int pcb_pstk_replace_proto(pcb_pstk_t *dst, const pcb_pstk_proto_t *src)
{
	rnd_cardinal_t pid = pcb_pstk_proto_insert_dup(dst->parent.data, src, 0, 1);

	rnd_trace("PSTK replace %ld!\n", pid);

	if (pid != PCB_PADSTACK_INVALID)
		pcb_pstk_change_instance(dst, &pid, NULL, NULL, NULL, NULL);

	return 0;
}

/* DOC: padstackreplace.html */
static const char pcb_acts_PadstackReplace[] = "PadstackReplace(object|selected, buffer|tool)";
static const char pcb_acth_PadstackReplace[] = "Replace padstack prototypes from buffer's first padstack or from the via tool";
fgw_error_t pcb_act_PadstackReplace(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int dst_op, src_op, ret = 0;
	pcb_pstk_proto_t *src_proto = NULL;

	RND_ACT_CONVARG(1, FGW_KEYWORD, PadstackReplace, dst_op = fgw_keyword(&argv[1]));
	RND_ACT_CONVARG(2, FGW_KEYWORD, PadstackReplace, src_op = fgw_keyword(&argv[2]));


	switch(src_op) {
		case F_Buffer:
			{
				pcb_pstk_t *ps = padstacklist_first(&PCB_PASTEBUFFER->Data->padstack);
				if (ps == NULL) {
					rnd_message(RND_MSG_ERROR, "PadstackReplace: Invalid source padstack (second argument): no padstack in buffer\n");
					return FGW_ERR_ARG_CONV;
				}
				src_proto = pcb_pstk_get_proto(ps);
			}
			break;
		case F_Tool:
			src_proto = pcb_pstk_get_proto_(pcb->Data, conf_core.design.via_proto);
			break;
		default:
			rnd_message(RND_MSG_ERROR, "PadstackReplace: Invalid source padstack (second argument)\n");
			return FGW_ERR_ARG_CONV;
	}

	if (src_proto == NULL) {
		rnd_message(RND_MSG_ERROR, "PadstackReplace: source padstack (second argument): prototype not found\n");
		return FGW_ERR_ARG_CONV;
	}

	switch(dst_op) {
		case F_Selected:
			{
				rnd_cardinal_t n;
				int ret = 0;
				vtp0_t objs;
				pcb_any_obj_t **o;

				vtp0_init(&objs);

				pcb_undo_freeze_serial();

				/* list all selected padstacks within subcircuits */
				PCB_SUBC_LOOP(pcb->Data); {
					pcb_data_list_by_flag(subc->data, &objs, PCB_OBJ_PSTK, PCB_FLAG_SELECTED);
				} PCB_END_LOOP;
				
				/* list all selected board padstacks */
				pcb_data_list_by_flag(pcb->Data, &objs, PCB_OBJ_PSTK, PCB_FLAG_SELECTED);
				for(n = 0, o = (pcb_any_obj_t **)objs.array; n < vtp0_len(&objs); n++,o++)
					ret |= pcb_pstk_replace_proto((pcb_pstk_t *)*o, src_proto);
				RND_ACT_IRES(ret);
				vtp0_uninit(&objs);
				pcb_undo_unfreeze_serial();
				pcb_undo_inc_serial();
			}
			break;
		case F_Object:
			{
				void *ptr1, *ptr2, *ptr3;
				pcb_pstk_t *ps;
				pcb_objtype_t type;
				rnd_coord_t x, y;
				rnd_hid_get_coords("Select a padstack to replace", &x, &y, 0);
				if ((type = pcb_search_screen(x, y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_PSTK) {
					rnd_message(RND_MSG_ERROR, "Need a padstack under the cursor\n");
					break;
				}
				pcb_undo_freeze_serial();
				ps = (pcb_pstk_t *)ptr2;
				ret = pcb_pstk_replace_proto(ps, src_proto);
				pcb_undo_unfreeze_serial();
				pcb_undo_inc_serial();

			}
			break;
		default:
			rnd_message(RND_MSG_ERROR, "PadstackReplace: Invalid padstack destination (first argument)\n");
			return FGW_ERR_ARG_CONV;
	}

	RND_ACT_IRES(ret);
	return 0;
}

static const char pcb_acts_PadstackMoveOrigin[] = "PadstackMoveOrigin(object, [x, y])";
static const char pcb_acth_PadstackMoveOrigin[] = "Move origin of the padstack prototype to x;y or by delta x;y (if relative); if x;y is not specified, move origin to crosshair coords";
fgw_error_t pcb_act_PadstackMoveOrigin(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	char *sx = NULL, *sy = NULL;
	rnd_coord_t x, y, dx, dy;
	rnd_bool relative = 0;
	void *ptr1, *ptr2, *ptr3;
	pcb_pstk_t *ps = NULL;
	pcb_pstk_proto_t *proto = NULL;
	pcb_data_t *data;
	pcb_objtype_t type;

	RND_ACT_CONVARG(1, FGW_KEYWORD, PadstackMoveOrigin, op = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_KEYWORD, PadstackMoveOrigin, sx = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_KEYWORD, PadstackMoveOrigin, sy = argv[2].val.str);

	if (op != F_Object) {
		rnd_message(RND_MSG_ERROR, "PadstackMoveOrigin: first argument must be \"object\"\n");
		return FGW_ERR_ARG_CONV;
	}

	if (rnd_hid_get_coords("Pick new origin for padstack", &x, &y, 0) < 0) {
		RND_ACT_IRES(-1);
		return 0;
	}

	if ((type = pcb_search_screen(x, y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_PSTK) {
		rnd_message(RND_MSG_ERROR, "Need a padstack under the cursor\n");
		RND_ACT_IRES(-1);
		return 0;
	}
	ps = (pcb_pstk_t *)ptr2;
	if (ps != NULL)
		proto = pcb_pstk_get_proto(ps);
	if (proto == NULL) {
		rnd_message(RND_MSG_ERROR, "Internal error: can't get padstack proto\n");
		RND_ACT_IRES(-1);
		return 0;
	}
	data = ps->parent.data;

	if (sy != NULL) {
		rnd_bool absx, absy, succ;
		if (sx == NULL) {
			rnd_message(RND_MSG_ERROR, "PadstackMoveOrigin: need to specify both x and y\n");
			return FGW_ERR_ARG_CONV;
		}
		x = rnd_get_value(sx, NULL, &absx, &succ);
		y = rnd_get_value(sy, NULL, &absy, &succ);
		relative = !absx || !absy;
	}
	else {
		/* clicked on the new origin, it's in x;y already */
		relative = 0;
	}

	if (relative) {
		dx = x;
		dy = y;
	}
	else {
		dx = ps->x - x;
		dy = ps->y - y;
	}

	pcb_undo_freeze_serial();
	pcb_draw_inhibit_inc();
	pcb_data_clip_inhibit_inc(data);

	/* modify the prototype */
	pcb_pstk_proto_move(proto, dx, dy, 1);

	/* move all existing padstacks to compensate */
	for(ps = padstacklist_first(&data->padstack); ps != NULL; ps = ps->link.next) {
		rnd_coord_t tx = -dx, ty = -dy;

		if (ps->rot != 0) {
			double rad = (-ps->rot) / RND_RAD_TO_DEG;
/*			rnd_trace("psrot: %mm;%mm for %f\n", tx, ty, ps->rot);*/
			rnd_rotate(&tx, &ty, 0, 0, cos(rad), sin(rad));
/*			rnd_trace("       %mm;%mm\n", tx, ty);*/
		}
		if (ps->smirror)
			ty = -ty;
		if (ps->xmirror)
			tx = -tx;

		pcb_move_obj(PCB_OBJ_PSTK, data, ps, ps, tx, ty);
	}

	pcb_data_clip_inhibit_dec(data, 1);
	pcb_draw_inhibit_dec();
	pcb_undo_unfreeze_serial();
	pcb_undo_inc_serial();

	RND_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static rnd_action_t padstack_action_list[] = {
	{"PadstackConvert", pcb_act_padstackconvert, pcb_acth_padstackconvert, pcb_acts_padstackconvert},
	{"PadstackBreakup", pcb_act_padstackbreakup, pcb_acth_padstackbreakup, pcb_acts_padstackbreakup},
	{"PadstackPlace", pcb_act_padstackplace, pcb_acth_padstackplace, pcb_acts_padstackplace},
	{"PadstackReplace", pcb_act_PadstackReplace, pcb_acth_PadstackReplace, pcb_acts_PadstackReplace},
	{"PadstackMoveOrigin", pcb_act_PadstackMoveOrigin, pcb_acth_PadstackMoveOrigin, pcb_acts_PadstackMoveOrigin}
};

void pcb_pstk_act_init2(void)
{
	RND_REGISTER_ACTIONS(padstack_action_list, NULL);
}


