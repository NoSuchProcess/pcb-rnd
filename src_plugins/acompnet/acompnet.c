/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
#include "board.h"
#include "data.h"
#include "layer.h"
#include "layer_ui.h"
/*#include "acompnet_conf.h"*/
#include "actions.h"
#include "plugins.h"
#include "conf.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "error.h"

static pcb_layer_t *ly;

pcb_flag_t flg_mesh_pt;
static void acompnet_mesh_addpt(double x, double y, int score)
{
	x = pcb_round(x);
	y = pcb_round(y);
	
	pcb_line_new(ly, x, y, x, y, conf_core.design.line_thickness, conf_core.design.bloat, flg_mesh_pt);
}

static void acompnet_mesh()
{
	double sep = conf_core.design.line_thickness + conf_core.design.bloat;
	int n;

	PCB_LINE_LOOP(CURRENT) {
		double len, vx, vy, x1, y1, x2, y2, nx, ny;
		x1 = line->Point1.X;
		x2 = line->Point2.X;
		y1 = line->Point1.Y;
		y2 = line->Point2.Y;
		vx = x2 - x1;
		vy = y2 - y1;
		len = sqrt(vx*vx + vy*vy);
		vx = vx/len;
		vy = vy/len;
		nx = vy;
		ny = -vx;

		for(n = 1; n <= 2; n++) {
			acompnet_mesh_addpt(x1 - n*vx*sep, y1 - n*vy*sep, n-1);
			acompnet_mesh_addpt(x1 - n*vx*sep + nx*sep, y1 - n*vy*sep + ny*sep, n);
			acompnet_mesh_addpt(x1 - n*vx*sep - nx*sep, y1 - n*vy*sep - ny*sep, n);

			acompnet_mesh_addpt(x2 + n*vx*sep, y2 + n*vy*sep, n-1);
			acompnet_mesh_addpt(x2 + n*vx*sep + nx*sep, y2 + n*vy*sep + ny*sep, n);
			acompnet_mesh_addpt(x2 + n*vx*sep - nx*sep, y2 + n*vy*sep - ny*sep, n);
		}
	}
	PCB_END_LOOP;
}


static const char pcb_acts_acompnet[] = "acompnet()\n" ;
static const char pcb_acth_acompnet[] = "Attempt to auto-complete the current network";

static fgw_error_t pcb_act_acompnet(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	acompnet_mesh();
	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t acompnet_action_list[] = {
	{"acompnet", pcb_act_acompnet, pcb_acth_acompnet, pcb_acts_acompnet},
};

static const char *acompnet_cookie = "acompnet plugin";

PCB_REGISTER_ACTIONS(acompnet_action_list, acompnet_cookie)

int pplg_check_ver_acompnet(int ver_needed) { return 0; }

void pplg_uninit_acompnet(void)
{
	pcb_remove_actions_by_cookie(acompnet_cookie);
	pcb_uilayer_free_all_cookie(acompnet_cookie);
}

#include "dolists.h"
int pplg_init_acompnet(void)
{
	static pcb_color_t clr;

	PCB_API_CHK_VER;

	if (clr.str[0] != '#')
		pcb_color_load_str(&clr, "#c09920");

	PCB_REGISTER_ACTIONS(acompnet_action_list, acompnet_cookie)
	ly = pcb_uilayer_alloc(acompnet_cookie, "autocomp-net", &clr);

	return 0;
}
