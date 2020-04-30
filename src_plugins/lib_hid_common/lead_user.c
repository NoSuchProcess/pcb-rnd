/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <librnd/config.h>

#include <assert.h>

#include <librnd/core/unit.h>
#include <librnd/core/event.h>
#include <librnd/core/hid.h>

#define LEAD_ARROW_LEN    PCB_MM_TO_COORD(3)
#define LEAD_CENTER_RAD   PCB_MM_TO_COORD(0.5)
#define LEAD_ANIM_AMP     PCB_MM_TO_COORD(2)
#define LEAD_ANIM_SPEED   0.6
#define LEAD_PERIOD_MS    100

static rnd_coord_t leadx, leady, step;
static rnd_bool lead;
static rnd_hidval_t lead_timer;
static rnd_hidlib_t *lead_hidlib;

static void lead_cb(rnd_hidval_t user_data)
{
	static double p;

	step = sin(p) * LEAD_ANIM_AMP - LEAD_ANIM_AMP;
	p += LEAD_ANIM_SPEED;

	pcb_gui->invalidate_all(pcb_gui);

	if (lead)
		lead_timer = pcb_gui->add_timer(pcb_gui, lead_cb, LEAD_PERIOD_MS, user_data);
}

static void pcb_lead_user_to_location(rnd_hidlib_t *hidlib, rnd_coord_t x, rnd_coord_t y, rnd_bool enabled)
{
	rnd_hidval_t user_data;

	if (lead) {
		pcb_gui->stop_timer(pcb_gui, lead_timer);
		lead = enabled;
		pcb_gui->invalidate_all(pcb_gui);
	}

	leadx = x;
	leady = y;
	lead = enabled;
	lead_hidlib = hidlib;

	if (enabled)
		lead_timer = pcb_gui->add_timer(pcb_gui, lead_cb, LEAD_PERIOD_MS, user_data);
}

void pcb_lead_user_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (argc < 4)
		return;
	if ((argv[1].type != RND_EVARG_COORD) || (argv[2].type != RND_EVARG_COORD) || (argv[3].type != RND_EVARG_INT))
		return;

	pcb_lead_user_to_location(hidlib, argv[1].d.c, argv[2].d.c, argv[3].d.i);
}



#define ARL LEAD_ARROW_LEN/3
void pcb_lead_user_draw_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (lead) {
		rnd_hid_gc_t *gc = argv[1].d.p;
		rnd_coord_t x = leadx + step, y = leady + step;

		assert(argv[1].type == RND_EVARG_PTR);

		pcb_render->set_line_width(*gc, ARL/40);
		pcb_render->draw_arc(*gc, leadx, leady, LEAD_CENTER_RAD+step/10, LEAD_CENTER_RAD+step/10, 0, 360);

/*		pcb_render->set_color(*gc, const rnd_color_t *color);*/

		pcb_render->draw_line(*gc, x, y, x - ARL, y);
		pcb_render->draw_line(*gc, x, y, x, y-ARL);
		pcb_render->draw_line(*gc, x - ARL, y, x, y-ARL);

		pcb_gui->set_line_width(*gc, ARL/4);
		pcb_render->draw_line(*gc, x - ARL/2, y - ARL/2, x - LEAD_ARROW_LEN, y - LEAD_ARROW_LEN);

	}
}


