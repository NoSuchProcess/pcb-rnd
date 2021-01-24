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
#include <genht/htpp.h>
#include <genht/hash.h>

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
conf_show_netnames_t conf_show_netnames;

static pcb_qry_exec_t shn_qctx;

typedef struct {
	rnd_coord_t w, h; /* bbox width and height when netname drawn with 100% font scale */
	unsigned show:1;  /* 1 if label should be printed */
} shn_net_t;

static shn_net_t shn_invalid = {0};

#define HT_HAS_CONST_KEY
#define HT_INVALID_VALUE shn_invalid
typedef void *htshn_key_t;
typedef const void *htshn_const_key_t;
typedef shn_net_t htshn_value_t;
#define HT(x) htshn_ ## x
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT
#undef HT_INVALID_VALIUE


static htshn_t shn_cache;
static int shn_cache_inited;
static int shn_cache_uptodate;
static shn_net_t shn_nonet;

static void shn_cache_update(pcb_board_t *pcb)
{
	pcb_net_t *net;
	pcb_net_it_t it;
	pcb_text_t t = {0};

	if (shn_cache_inited)
		htshn_clear(&shn_cache);
	else
		htshn_init(&shn_cache, ptrhash, ptrkeyeq);

	t.Scale = 100;
	for(net = pcb_net_first(&it, &pcb->netlist[PCB_NETLIST_EDITED]); net != NULL; net = pcb_net_next(&it)) {
		shn_net_t shn;
		t.TextString = net->name;
		pcb_text_bbox(pcb_font(pcb, 0, 1), &t);

		shn.w = t.BoundingBox.X2 - t.BoundingBox.X1;
		shn.h = t.BoundingBox.Y2 - t.BoundingBox.Y1;
		shn.show = 1;
		htshn_set(&shn_cache, net, shn);
	}

	t.TextString = "<nonet #00000000>";
	pcb_text_bbox(pcb_font(pcb, 0, 1), &t);
	shn_nonet.w = t.BoundingBox.X2 - t.BoundingBox.X1;
	shn_nonet.h = t.BoundingBox.Y2 - t.BoundingBox.Y1;
	shn_nonet.show = 1;


	shn_cache_uptodate = 1;
}

static void show_netnames_invalidate(void)
{
	pcb_qry_uninit(&shn_qctx);
}

static show_netnames_inv_cache(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	shn_cache_uptodate = 0;
}

static void show_netnames_brd_chg(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (conf_show_netnames.plugins.show_netnames.enable)
		show_netnames_invalidate();
	shn_cache_uptodate = 0;
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
	shn_net_t *shn = NULL;
	htshn_entry_t *e;
	pcb_net_t *net = NULL;

	if (obj->type != PCB_OBJ_LINE)
		return NULL;

	if (!shn_cache_uptodate)
		shn_cache_update(PCB);

	term = pcb_qry_parent_net_term(&shn_qctx, obj);
	if ((term != NULL) && (term->type == PCB_OBJ_NET_TERM))
		net = term->parent.net;

	if ((net == NULL) || (net->type != PCB_OBJ_NET)) {
		if (conf_show_netnames.plugins.show_netnames.omit_nonets)
			return;
		shn = &shn_nonet;
		if (term != NULL) {
			sprintf(tmp, "<nonet #%ld>", term->ID);
			netname = tmp;
		}
		else
			netname = "<nonet>";
	}
	else {
		e = htshn_getentry(&shn_cache, net);
		if (e != NULL)
			shn = &e->value;
		netname = net->name;
	}

	if (shn == NULL)
		return;

	pcb_obj_center(obj, &x, &y);
	font = pcb_font(PCB, 0, 0);

	switch(obj->type) {
		case PCB_OBJ_LINE:
			{
				pcb_line_t *l = (pcb_line_t *)obj;

				/* text height is 80% of trace thickness */
				lscale = (double)l->Thickness / (double)shn->h * 0.8;

				rot = atan2(l->Point2.Y - l->Point1.Y, l->Point2.X - l->Point1.X) * -RND_RAD_TO_DEG;

				/* offset the text for approx. center alignment */
				dx = -shn->w * lscale / 2; dy = -shn->h * lscale / 2;
				vx = l->Point2.X - l->Point1.X; vy = l->Point2.Y - l->Point1.Y;
				len = vx*vx + vy*vy;

				if (len != 0) {
					len = sqrt(len);

				/* Don't print if label is longer than line's 80% */
				if (shn->w * lscale > len * 0.8)
					return;

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

	rnd_conf_unreg_intern(show_netnames_conf_internal);
	rnd_hid_menu_unload(rnd_gui, pcb_show_netnames_cookie);
	rnd_event_unbind_allcookie(pcb_show_netnames_cookie);
	rnd_remove_actions_by_cookie(pcb_show_netnames_cookie);
	rnd_conf_unreg_fields("plugins/show_netnames/");

	if (shn_cache_inited)
		htshn_uninit(&shn_cache);
}

int pplg_init_show_netnames(void)
{
	RND_API_CHK_VER;

	rnd_conf_reg_intern(show_netnames_conf_internal);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_show_netnames, field,isarray,type_name,cpath,cname,desc,flags);
#include "show_netnames_conf_fields.h"

	rnd_event_bind(PCB_EVENT_BOARD_EDITED, show_netnames_brd_chg, NULL, pcb_show_netnames_cookie);
	rnd_event_bind(RND_EVENT_BOARD_CHANGED, show_netnames_brd_chg, NULL, pcb_show_netnames_cookie);
	rnd_event_bind(RND_EVENT_GUI_DRAW_OVERLAY_XOR, show_netnames_render, NULL, pcb_show_netnames_cookie);
	rnd_event_bind(PCB_EVENT_NETLIST_CHANGED, show_netnames_inv_cache, NULL, pcb_show_netnames_cookie);
	rnd_event_bind(PCB_EVENT_FONT_CHANGED, show_netnames_inv_cache, NULL, pcb_show_netnames_cookie);

	rnd_hid_menu_load(rnd_gui, NULL, pcb_show_netnames_cookie, 150, NULL, 0, show_netnames_menu, "plugin: show_netnames");
	return 0;
}
