/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019,2020 Tibor 'Igor2' Palinkas
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

#include <librnd/core/hidlib_conf.h>
#include "event.h"
#include <librnd/core/grid.h>
#include <librnd/core/actions.h>
#include <librnd/core/tool.h>
#include <librnd/core/conf_hid.h>

#include "board.h"
#include "conf_core.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"

#include "tool_logic.h"

static void tool_logic_chg_layer(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);
static void pcb_release_mode(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);
static void pcb_press_mode(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);

/*** Generic part, all rnd apps should do something like this ***/

static char tool_logic_cookie[] = "tool_logic";

static void tool_logic_chg_tool(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	int *ok = argv[1].d.p;
	int id = argv[2].d.i;
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	if (pcb->RatDraw && !(rnd_tool_get(id)->user_flags & PCB_TLF_RAT)) {
		rnd_message(RND_MSG_WARNING, "That tool can not be used on the rat layer!\n");
		*ok = 0;
	}
}

static void tool_logic_chg_mode(rnd_conf_native_t *cfg, int arr_idx)
{
	rnd_tool_chg_mode(&PCB->hidlib);
}


void pcb_tool_logic_init(void)
{
	static rnd_conf_hid_callbacks_t cbs_mode;
	static rnd_conf_hid_id_t tool_conf_id;
	rnd_conf_native_t *n_mode = rnd_conf_get_field("editor/mode");
	tool_conf_id = rnd_conf_hid_reg(tool_logic_cookie, NULL);

	if (n_mode != NULL) {
		memset(&cbs_mode, 0, sizeof(rnd_conf_hid_callbacks_t));
		cbs_mode.val_change_post = tool_logic_chg_mode;
		rnd_conf_hid_set_cb(n_mode, tool_conf_id, &cbs_mode);
	}

	rnd_event_bind(RND_EVENT_TOOL_SELECT_PRE, tool_logic_chg_tool, NULL, tool_logic_cookie);
	rnd_event_bind(RND_EVENT_TOOL_RELEASE, pcb_release_mode, NULL, tool_logic_cookie);
	rnd_event_bind(RND_EVENT_TOOL_PRESS, pcb_press_mode, NULL, tool_logic_cookie);
	rnd_event_bind(PCB_EVENT_LAYERVIS_CHANGED, tool_logic_chg_layer, NULL, tool_logic_cookie);
}

void pcb_tool_logic_uninit(void)
{
	rnd_event_unbind_allcookie(tool_logic_cookie);
	rnd_conf_hid_unreg(tool_logic_cookie);
}

/*** pcb-rnd-specific parts ***/


static void tool_logic_chg_layer(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	static int was_rat;
	if (PCB->RatDraw && !was_rat && !(rnd_tool_get(rnd_conf.editor.mode)->user_flags & PCB_TLF_RAT))
		rnd_tool_select_by_name(&PCB->hidlib, "line");
	was_rat = PCB->RatDraw;
}

static void get_grid_lock_coordinates(int type, void *ptr1, void *ptr2, void *ptr3, rnd_coord_t * x, rnd_coord_t * y)
{
	switch (type) {
	case PCB_OBJ_LINE:
		*x = ((pcb_line_t *) ptr2)->Point1.X;
		*y = ((pcb_line_t *) ptr2)->Point1.Y;
		break;
	case PCB_OBJ_TEXT:
		*x = ((pcb_text_t *) ptr2)->X;
		*y = ((pcb_text_t *) ptr2)->Y;
		break;
	case PCB_OBJ_POLY:
		*x = ((pcb_poly_t *) ptr2)->Points[0].X;
		*y = ((pcb_poly_t *) ptr2)->Points[0].Y;
		break;

	case PCB_OBJ_LINE_POINT:
	case PCB_OBJ_POLY_POINT:
		*x = ((rnd_point_t *) ptr3)->X;
		*y = ((rnd_point_t *) ptr3)->Y;
		break;
	case PCB_OBJ_ARC:
		pcb_arc_get_end((pcb_arc_t *) ptr2, 0, x, y);
		break;
	case PCB_OBJ_ARC_POINT:
		if (ptr3 != NULL) /* need to check because: if snap off, there's no known endpoint (leave x;y as is, then) */
			pcb_arc_get_end((pcb_arc_t *) ptr2, ((*(int **)ptr3) != pcb_arc_start_ptr), x, y);
		break;
	}
}

void pcb_tool_attach_for_copy(rnd_hidlib_t *hl, rnd_coord_t PlaceX, rnd_coord_t PlaceY, rnd_bool do_rubberband)
{
	rnd_coord_t mx = 0, my = 0;

	rnd_event(hl, PCB_EVENT_RUBBER_RESET, NULL);
	if (!conf_core.editor.snap_pin) {
		/* dither the grab point so that the mark, center, etc
		 * will end up on a grid coordinate
		 */
		get_grid_lock_coordinates(pcb_crosshair.AttachedObject.Type,
															pcb_crosshair.AttachedObject.Ptr1,
															pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3, &mx, &my);
		mx = rnd_grid_fit(mx, hl->grid, hl->grid_ox) - mx;
		my = rnd_grid_fit(my, hl->grid, hl->grid_oy) - my;
	}
	pcb_crosshair.AttachedObject.X = PlaceX - mx;
	pcb_crosshair.AttachedObject.Y = PlaceY - my;
	if ((!pcb_marked.status || conf_core.editor.local_ref) && !pcb_marked.user_placed)
		pcb_crosshair_set_local_ref(PlaceX - mx, PlaceY - my, rnd_true);
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;

	/* get all attached objects if necessary */
	if (do_rubberband && conf_core.editor.rubber_band_mode)
		rnd_event(hl, PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
	if (do_rubberband &&
			(pcb_crosshair.AttachedObject.Type == PCB_OBJ_SUBC ||
			 pcb_crosshair.AttachedObject.Type == PCB_OBJ_PSTK ||
			 pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE || pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE_POINT))
		rnd_event(hl, PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
}

void pcb_tool_notify_block(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	switch (pcb_crosshair.AttachedBox.State) {
	case PCB_CH_STATE_FIRST:						/* setup first point */
		pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X = pcb_crosshair.X;
		pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y = pcb_crosshair.Y;
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_SECOND;
		break;

	case PCB_CH_STATE_SECOND:						/* setup second point */
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_THIRD;
		break;
	}
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

/*** old helpers ***/

static void pcb_release_mode(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_draw();
}

static void pcb_press_mode(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;

	if (conf_core.temp.rat_warn) {
		if (pcb_data_clear_flag(pcb->Data, PCB_FLAG_WARN, 1, 0) > 0)
			pcb_board_set_changed_flag(rnd_true);
	}
	pcb_draw();
}
