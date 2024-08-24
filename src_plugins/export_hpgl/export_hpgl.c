/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2022,2024 Tibor 'Igor2' Palinkas
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
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <librnd/core/math_helper.h>
#include "board.h"
#include "data.h"
#include "draw.h"
#include <librnd/core/error.h>
#include "layer.h"
#include "layer_vis.h"
#include <librnd/core/misc_util.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>
#include "funchash_core.h"

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>

#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_attrib.h>
#include "hid_cam.h"
#include "endp.h"

const char *exp_hpgl_cookie = "exp_hpgl HID";

static pcb_cam_t exp_hpgl_cam;

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_color_t color, last_color;
	rnd_cap_style_t style, last_style;
	rnd_coord_t capw, last_capw;
} rnd_hid_gc_s;

static FILE *f = NULL;
static gds_t fn_gds;
static int fn_baselen = 0;
static char *filename = NULL;
static char *filesuff = NULL;
static htendp_t ht;
static int ht_active;
static pcb_dynf_t dflg;
static rnd_coord_t maxy;
static void *hpgl_objs = NULL;
static int sepfiles, pens;
static int hpgl_ovr;

#define TX(x)  ((long)(RND_COORD_TO_MM(x)/0.025))
#define TY(y)  ((long)((RND_COORD_TO_MM(maxy)/0.025) - ((RND_COORD_TO_MM(y)/0.025))))

static void print_header(void)
{
	fprintf(f, "IN;\n");
}

static void print_footer(void)
{
	fprintf(f, "PU;\n");
}

static void render_obj(void *uctx, hpgl_any_obj_t *o, endp_state_t st)
{
	hpgl_line_t *l = (hpgl_line_t *)o;
	hpgl_arc_t *a = (hpgl_arc_t *)o;

	if (st & ENDP_ST_START) {
		rnd_cheap_point_t ep;
		switch(o->type) {
			case PCB_OBJ_ARC:
				if (st & ENDP_ST_REVERSE) {
					ep.X = a->Point2.X; ep.Y = a->Point2.Y;
				}
				else {
					ep.X = a->Point1.X; ep.Y = a->Point1.Y;
				}
				break;
			case PCB_OBJ_LINE:
				if (st & ENDP_ST_REVERSE) {
					ep.X = l->Point2.X; ep.Y = l->Point2.Y;
				}
				else {
					ep.X = l->Point1.X; ep.Y = l->Point1.Y;
				}
				break;
			default:
				abort();
		}
		fprintf(f, "PU;PA%ld,%ld;PD;\n", TX(ep.X), TY(ep.Y));
	}

	/* we are at the starting point already, pen is down;
	   move to the other end of the object */
	switch(o->type) {
		case PCB_OBJ_ARC:
			if (st & ENDP_ST_REVERSE)
				fprintf(f, "AA%ld,%ld,%.2f,0.1;\n", TX(a->X), TY(a->Y), -a->Delta);
			else
				fprintf(f, "AA%ld,%ld,%.2f,0.1;\n", TX(a->X), TY(a->Y), a->Delta);
			break;
		case PCB_OBJ_LINE:
			if (st & ENDP_ST_REVERSE)
				fprintf(f, "PA%ld,%ld;\n", TX(l->Point1.X), TY(l->Point1.Y));
			else
				fprintf(f, "PA%ld,%ld;\n", TX(l->Point2.X), TY(l->Point2.Y));
			break;
		default:
			break;
	}
}

static void hpgl_flush_layer(void)
{
	if (ht_active && (hpgl_objs != NULL)) {
		hpgl_line_t *curr, *next;

		if (!sepfiles) {
			fprintf(f, "SP%d;\n", pens);
			pens++;
		}

		hpgl_endp_render(&ht, render_obj, NULL, dflg);
		hpgl_endp_uninit(&ht);

		for(curr = hpgl_objs; curr != NULL; curr = next) {
			next = curr->next;
			free(curr);
		}
		
		hpgl_objs = NULL;
		ht_active = 0;
	}
}

#include "hpgl_thin.c"

int pplg_check_ver_export_hpgl(int ver_needed) { return 0; }

void pplg_uninit_export_hpgl(void)
{
	hpgl_thin_uninit();
}

int pplg_init_export_hpgl(void)
{
	RND_API_CHK_VER;

	if (hpgl_thin_init() != 0)
		return -1;

	return 0;
}
