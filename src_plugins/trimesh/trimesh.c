/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
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

#include <stdio.h>

#include <librnd/core/plugins.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/hid/hid_attrib.h>
#include <librnd/core/actions.h>

#include "board.h"
#include "data.h"
#include "flag.h"
#include "obj_line.h"

#include <src_plugins/lib_polyhelp/triangulate.h>

static const char *trimesh_cookie = "trimesh";

typedef enum {
	PT_EDGE,
	PT_MAIN,
	PT_AUX,
	PT_max
} point_t;

typedef struct {
	pcb_board_t *pcb;
	rnd_rtree_t pts[PT_max];
} trimesh_t;


static void trimesh_init(trimesh_t *ctx, pcb_board_t *pcb)
{
	int n;

	ctx->pcb = pcb;
	for(n = 0; n < PT_max; n++)
		rnd_rtree_init(&ctx->pts[n]);
}

static void trimesh_uninit(trimesh_t *ctx)
{
	int n;

	for(n = 0; n < PT_max; n++)
		rnd_rtree_uninit(&ctx->pts[n]);

}

static void trimesh_selected_lines(trimesh_t *ctx)
{
	PCB_LINE_ALL_LOOP(ctx->pcb->Data); {
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, line))
			continue;

	} PCB_ENDALL_LOOP;
}

static const char pcb_acts_TriMesh[] = "TriMesh()\n";
static const char pcb_acth_TriMesh[] = "Generate triangular mesh for simulation";
static fgw_error_t pcb_act_TriMesh(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	trimesh_t ctx = {0};

	trimesh_init(&ctx, PCB_ACT_BOARD);
	trimesh_selected_lines(&ctx);
	trimesh_uninit(&ctx);

	RND_ACT_IRES(0);
	return 0;
}


static rnd_action_t trimesh_action_list[] = {
	{"TriMesh", pcb_act_TriMesh, pcb_acth_TriMesh, pcb_acts_TriMesh}
};


int pplg_check_ver_trimesh(int ver_needed) { return 0; }

void pplg_uninit_trimesh(void)
{
	rnd_remove_actions_by_cookie(trimesh_cookie);
}

int pplg_init_trimesh(void)
{
	RND_API_CHK_VER;

	RND_REGISTER_ACTIONS(trimesh_action_list, trimesh_cookie);

	return 0;
}
