/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2019 Tibor 'Igor2' Palinkas
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
#include "search.h"
#include "polygon.h"
#include "conf.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "error.h"

static pcb_layer_t *ly;

typedef struct {
	pcb_coord_t x, y;
	pcb_coord_t r;
} overlap_t;

static pcb_r_dir_t overlap(const pcb_box_t *box, void *closure)
{
	pcb_any_obj_t *obj = (pcb_any_obj_t *)box;
	overlap_t *ovl = (overlap_t *)closure;
	switch(obj->type) {
		case PCB_OBJ_LINE:
			if (pcb_is_point_in_line(ovl->x, ovl->y, ovl->r, (pcb_any_line_t *)obj))
				return PCB_R_DIR_CANCEL;
			break;
		case PCB_OBJ_ARC:
			if (pcb_is_point_on_arc(ovl->x, ovl->y, ovl->r, (pcb_arc_t *)obj))
				return PCB_R_DIR_CANCEL;
			break;
		case PCB_OBJ_TEXT:
			if (pcb_is_point_in_box(ovl->x, ovl->y, &obj->bbox_naked, ovl->r))
				return PCB_R_DIR_CANCEL;
			break;
		case PCB_OBJ_POLY:
			if (pcb_poly_is_point_in_p(ovl->x, ovl->y, ovl->r, (pcb_poly_t *)obj))
				return PCB_R_DIR_CANCEL;
			break;
		default: break;
	}
	return PCB_R_DIR_NOT_FOUND;
}

TODO("move this to search.[ch]")
/* Search for object(s) on a specific layer */
static pcb_r_dir_t pcb_search_on_layer(pcb_layer_t *layer, const pcb_box_t *bbox, pcb_r_dir_t (*cb)(const pcb_box_t *box, void *closure), void *closure)
{
	pcb_r_dir_t res, fin = 0;

	if ((res = pcb_r_search(layer->line_tree, bbox, NULL, cb, closure, NULL)) == PCB_R_DIR_CANCEL)
		return res;
	fin |= res;

	if ((res = pcb_r_search(layer->arc_tree, bbox, NULL, cb, closure, NULL)) == PCB_R_DIR_CANCEL)
		return res;
	fin |= res;

	if ((res = pcb_r_search(layer->polygon_tree, bbox, NULL, cb, closure, NULL)) == PCB_R_DIR_CANCEL)
		return res;
	fin |= res;

	if ((res = pcb_r_search(layer->text_tree, bbox, NULL, cb, closure, NULL)) == PCB_R_DIR_CANCEL)
		return res;
	fin |= res;

TODO("padstack too");

	return res;
}


pcb_flag_t flg_mesh_pt;
static void acompnet_mesh_addpt(pcb_layer_t *layer, double x, double y, int score, double sep)
{
	overlap_t ovl;
	pcb_box_t bbox;

	x = pcb_round(x);
	y = pcb_round(y);

	ovl.x = x;
	ovl.y = y;
	ovl.r = pcb_round(sep/2-1);
	bbox.X1 = x - ovl.r;
	bbox.X2 = x + ovl.r;
	bbox.Y1 = y - ovl.r;
	bbox.Y2 = y + ovl.r;

	if (pcb_search_on_layer(layer, &bbox, overlap, &ovl) == PCB_R_DIR_NOT_FOUND)
		pcb_line_new(ly, x, y, x, y, conf_core.design.line_thickness, conf_core.design.bloat, flg_mesh_pt);
}

static void acompnet_mesh(pcb_layer_t *layer)
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

		/* straight line extension points */
		acompnet_mesh_addpt(layer, x1 - vx*sep, y1 - vy*sep, 0, sep);
		acompnet_mesh_addpt(layer, x2 + vx*sep, y2 + vy*sep, 0, sep);

		/* side and extended points; n is in-line offset from endpoint */
		for(n = 0; n <= 1; n++) {
			acompnet_mesh_addpt(layer, x1 - n*vx*sep + nx*sep, y1 - n*vy*sep + ny*sep, 1, sep);
			acompnet_mesh_addpt(layer, x1 - n*vx*sep - nx*sep, y1 - n*vy*sep - ny*sep, 1, sep);

			acompnet_mesh_addpt(layer, x2 + n*vx*sep + nx*sep, y2 + n*vy*sep + ny*sep, 1, sep);
			acompnet_mesh_addpt(layer, x2 + n*vx*sep - nx*sep, y2 + n*vy*sep - ny*sep, 1, sep);
		}
	}
	PCB_END_LOOP;
}


static const char pcb_acts_acompnet[] = "acompnet()\n" ;
static const char pcb_acth_acompnet[] = "Attempt to auto-complete the current network";

static fgw_error_t pcb_act_acompnet(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	acompnet_mesh(CURRENT);
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
