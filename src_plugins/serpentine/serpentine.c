/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Adrian Purser
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

#include "config.h"
#include "conf_core.h"

#include <math.h>

#include "crosshair.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include "error.h"
#include "search.h"
#include "tool.h"
#include "rats.h"
#include "rtree.h"
#include "flag_str.h"
#include "macro.h"
#include "undo.h"
#include "find.h"
#include "draw.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "actions.h"
#include "misc_util.h"
#include "serpentine_conf.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "layer.h"
#include "obj_term.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_subc_parent.h"

#include <genregex/regex_sei.h>

conf_serpentine_t conf_serpentine;


static void tool_serpentine_init(void)
{
	/* TODO */
}

static void tool_serpentine_uninit(void)
{
  pcb_notify_crosshair_change(pcb_false);
  pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
  pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
  pcb_notify_crosshair_change(pcb_true);
}


static void tool_serpentine_notify_mode(void)
{
  int type;
  void *ptr1, *ptr2, *ptr3;
  pcb_any_obj_t *term_obj;

  switch (pcb_crosshair.AttachedObject.State) {
  case PCB_CH_STATE_FIRST:
    type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_OBJ_LINE, &ptr1, &ptr2, &ptr3);
    /* TODO: check if an object is on the current layer */
    if (type == PCB_OBJ_LINE) {
				/* TODO */
      }
      else
        pcb_message(PCB_MSG_WARNING, _("Serpentines can be only drawn onto a line\n"));
    break;

  case PCB_CH_STATE_SECOND:
		/* TODO */
    break;
  }
}


static void tool_serpentine_adjust_attached_objects(void)
{
	/* TODO */
}

static void tool_serpentine_draw_attached(void)
{
	/* TODO */
}


pcb_bool tool_serpentine_undo_act()
{
	/* TODO */
	return pcb_false;
}

static pcb_tool_t tool_serpentine = {
  "serpentine", NULL, 100,
  tool_serpentine_init,
  tool_serpentine_uninit,
  tool_serpentine_notify_mode,
  NULL,
  tool_serpentine_adjust_attached_objects,
  tool_serpentine_draw_attached,
  tool_serpentine_undo_act,
  NULL,

  pcb_false
};



/*** Actions ***/

static const char pcb_acts_serpentine[] = "serpentine()";
static const char pcb_acth_serpentine[] = "Tool for drawing serpentines";
fgw_error_t pcb_act_serpentine(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
  pcb_tool_select_by_name("serpentine");

  PCB_ACT_IRES(0);
  return 0;
}


pcb_action_t serpentine_action_list[] = {
  {"Serpentine", pcb_act_serpentine, pcb_acth_serpentine, pcb_acts_serpentine}
};

static const char *serpentine_cookie = "serpentine plugin";

PCB_REGISTER_ACTIONS(serpentine_action_list, serpentine_cookie)

int pplg_check_ver_serpentine(int ver_needed) { return 0; }

void pplg_uninit_serpentine(void)
{
  pcb_remove_actions_by_cookie(serpentine_cookie);
	pcb_tool_unreg_by_cookie(serpentine_cookie);
  conf_unreg_fields("plugins/serpentine/");
}

#include "dolists.h"
int pplg_init_serpentine(void)
{
  PCB_API_CHK_VER;
  PCB_REGISTER_ACTIONS(serpentine_action_list, serpentine_cookie)
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
  conf_reg_field(conf_serpentine, field,isarray,type_name,cpath,cname,desc,flags);
#include "serpentine_conf_fields.h"

  pcb_tool_reg(&tool_serpentine, serpentine_cookie);

  return 0;
}

