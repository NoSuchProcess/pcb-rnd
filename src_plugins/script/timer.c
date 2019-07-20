/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, debug, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

#include <genvector/vtp0.h>

typedef struct {
	double next, period;
	long count;
	char *user_data;
	char aname[1];
} script_timer_t;

vtp0_t timers;
int timer_running = 0;

static void start_timer(void);
static void timer_cb(pcb_hidval_t hv)
{
	long n, len;
	double now;

	len = vtp0_len(&timers);
	if (len == 0) {
		timer_running = 0;
		return; /* don't even restart the timer if we are not waiting for anything */
	}

	now = pcb_dtime();

	/* we could do something clever here, e.g. sort timers by ->next but
	   in reality there will be so few timers at any given time that it's
	   probably not worth the effort */
	for(n = len-1; n >= 0; n--) {
		script_timer_t *t = timers.array[n];
		fgw_func_t *f;
		fgw_arg_t res, argv[4];

		if (t->next <= now) {
			t->next += t->period;

			f = fgw_func_lookup(&pcb_fgw, t->aname);
			if (f == NULL)
				goto remove;
			argv[0].type = FGW_FUNC;
			argv[0].val.func = f;
			argv[1].type = FGW_DOUBLE;
			argv[1].val.nat_double = now;
			argv[2].type = FGW_LONG;
			argv[2].val.nat_long = t->count;
			if (t->user_data != NULL) {
				argv[3].type = FGW_STR;
				argv[3].val.str = t->user_data;
			}
			else {
				argv[3].type = FGW_STR;
				argv[3].val.str = "";
			}

			res.type = FGW_INVALID;
			if (pcb_actionv_(f, &res, 4, argv) != 0)
				goto remove;
			fgw_arg_conv(&pcb_fgw, &res, FGW_INT);
			if ((res.type != FGW_INT) || (res.val.nat_int != 0)) /* action requested timer removal */
				goto remove;

			if (t->count > 0) {
				t->count--;
				if (t->count == 0) {
					remove:;
					vtp0_remove(&timers, n, 1);
					free(t->user_data);
					free(t);
				}
			}
		}
	}

	start_timer();
}

static void start_timer(void)
{
	static pcb_hidval_t hv;
	timer_running = 1;
	pcb_gui->add_timer(pcb_gui, timer_cb, 100, hv);
}


static const char pcb_acth_AddTimer[] = "Add a new timer";
static const char pcb_acts_AddTimer[] = "AddTimer(action, period, [repeat], [userdata])";
/* DOC: addtimer.html */
static fgw_error_t pcb_act_AddTimer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *act, *user_data = NULL;
	double period;
	int count = 1, len;
	char fn[PCB_ACTION_NAME_MAX];
	script_timer_t *t;

	PCB_ACT_CONVARG(1, FGW_STR, AddTimer, act = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_DOUBLE, AddTimer, period = argv[2].val.nat_double);
	PCB_ACT_MAY_CONVARG(3, FGW_INT, AddTimer, count = argv[3].val.nat_int);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, AddTimer, user_data = argv[4].val.str);

	pcb_aname(fn, act);
	len = strlen(fn);
	t = malloc(sizeof(script_timer_t) + len);
	t->next = pcb_dtime() + period;
	t->period = period;
	t->count = count;
	strcpy(t->aname, fn);
	if (user_data != NULL)
		t->user_data = pcb_strdup(user_data);
	else
		t->user_data = NULL;

	vtp0_append(&timers, t);

	if (!timer_running)
		start_timer();

	PCB_ACT_IRES(0);
	return 0;
}
