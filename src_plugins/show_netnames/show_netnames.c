/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_menu.h>

#include "board.h"
#include "conf_core.h"
#include "draw.h"
#include "event.h"
#include "netlist.h"
#include "select.h"
#include "obj_text_draw.h"
#include "../src_plugins/query/net_int.h"

#include "show_netnames_conf.h"
#include "menu_internal.c"
#include "conf_internal.c"

const char *pcb_show_netnames_cookie = "show_netnames plugin";
#define SHOW_NETNAMES_CONF_FN "show_netnames.conf"
conf_show_netnames_t conf_show_netnames;

static pcb_qry_exec_t shn_qctx;

static void show_netnames_invalidate(void)
{
	pcb_qry_uninit(&shn_qctx);
}

static void show_netnames_brd_chg(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (conf_show_netnames.plugins.show_netnames.enable)
		show_netnames_invalidate();
}

static void *shn_render_cb(void *ctx, pcb_any_obj_t *obj)
{
	pcb_any_obj_t *term;
	char tmp[128];
	const char *netname;
	pcb_draw_info_t *info = ctx;
	rnd_coord_t x, y, dx, dy;
	pcb_font_t *font;
	double rot, vx, vy, nx, ny, len, lscale;

	if (obj->type != PCB_OBJ_LINE)
		return NULL;

	term = pcb_qry_parent_net_term(&shn_qctx, obj);
	if ((term != NULL) && (term->type == PCB_OBJ_NET_TERM)) {
		pcb_net_t *net = term->parent.net;
		if ((net == NULL) || (net->type != PCB_OBJ_NET)) {
			sprintf(tmp, "<nonet #%ld>", term->ID);
			netname = tmp;
		}
		else
			netname = net->name;
	}
	else
		netname = "<nonet>";

	pcb_obj_center(obj, &x, &y);
	font = pcb_font(PCB, 0, 0);
	lscale = (double)conf_show_netnames.plugins.show_netnames.zoom_level / 100000.0;
	switch(obj->type) {
		case PCB_OBJ_LINE:
			{
				pcb_line_t *l = (pcb_line_t *)obj;

				rot = atan2(l->Point2.Y - l->Point1.Y, l->Point2.X - l->Point1.X) * -RND_RAD_TO_DEG;

				/* offset the text for approx. center alignment */
				dx = 0; dy = -font->MaxHeight/2*lscale;
				vx = l->Point2.X - l->Point1.X; vy = l->Point2.Y - l->Point1.Y;
				len = vx*vx + vy*vy;
				if (len != 0) {
					len = sqrt(len);
					if (l != 0) {
						vx /= len;
						vy /= len;
						nx = -vy;
						ny = vx;
						x += nx * dy + vx * dx;
						y += ny * dy + vy * dx;
					}
				}

				pcb_text_draw_string(info, font, (const unsigned char *)netname, x, y, lscale, lscale, rot, 0, conf_core.appearance.label_thickness, 0, 0, 0, 0, PCB_TXT_TINY_HIDE);
			}
			break;
		default:
			pcb_text_draw_string(info, font, (const unsigned char *)netname, x, y, lscale, lscale, 0.0, 0, conf_core.appearance.label_thickness, 0, 0, 0, 0, PCB_TXT_TINY_HIDE);
	}
	return NULL;
}

static void show_netnames_render(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb;
	pcb_draw_info_t *info;
	rnd_box_t bx;

	if (!conf_show_netnames.plugins.show_netnames.enable || (rnd_gui == NULL) || (rnd_gui->coord_per_pix > conf_show_netnames.plugins.show_netnames.zoom_level))
		return;

	if (argv[2].type != RND_EVARG_PTR)
		return;

	pcb = (pcb_board_t *)hidlib;
	info = argv[2].d.p;

	if (shn_qctx.pcb != pcb) {
		show_netnames_invalidate();
		shn_qctx.pcb = pcb;
	}

	/* search negative box so anything touched is included */
	bx.X1 = info->drawn_area->X2; bx.Y1 = info->drawn_area->Y2;
	bx.X2 = info->drawn_area->X1; bx.Y2 = info->drawn_area->Y1;
	pcb_list_lyt_block_cb(pcb, PCB_LYT_COPPER, &bx, shn_render_cb, info);
}

int pplg_check_ver_show_netnames(int ver_needed) { return 0; }

void pplg_uninit_show_netnames(void)
{
	show_netnames_invalidate();

	rnd_conf_unreg_file(SHOW_NETNAMES_CONF_FN, show_netnames_conf_internal);
	rnd_hid_menu_unload(rnd_gui, pcb_show_netnames_cookie);
	rnd_event_unbind_allcookie(pcb_show_netnames_cookie);
	rnd_remove_actions_by_cookie(pcb_show_netnames_cookie);
	rnd_conf_unreg_fields("plugins/show_netnames/");
}

int pplg_init_show_netnames(void)
{
	RND_API_CHK_VER;

	rnd_conf_reg_file(SHOW_NETNAMES_CONF_FN, show_netnames_conf_internal);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_show_netnames, field,isarray,type_name,cpath,cname,desc,flags);
#include "show_netnames_conf_fields.h"

	rnd_event_bind(PCB_EVENT_BOARD_EDITED, show_netnames_brd_chg, NULL, pcb_show_netnames_cookie);
	rnd_event_bind(RND_EVENT_BOARD_CHANGED, show_netnames_brd_chg, NULL, pcb_show_netnames_cookie);
	rnd_event_bind(RND_EVENT_GUI_DRAW_OVERLAY_XOR, show_netnames_render, NULL, pcb_show_netnames_cookie);

	rnd_hid_menu_load(rnd_gui, NULL, pcb_show_netnames_cookie, 150, NULL, 0, show_netnames_menu, "plugin: show_netnames");
	return 0;
}
