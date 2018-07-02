/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Wojciech Krutnik
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

#include "plugins.h"
#include "actions.h"
#include "board.h"
#include "data.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "layer_ui.h"

#include "cdt/cdt.h"


const char *pcb_sketch_route_cookie = "sketch_route plugin";

typedef struct {
	cdt_t *cdt;
	pcb_layer_t *ui_layer_cdt;
} sketch_t;

static sketch_t sketch; /* TODO: should be created dynamically for each copper layer */


static void sketch_create_for_layer(sketch_t *sk, pcb_layer_t *layer)
{
	sk->cdt = malloc(sizeof(cdt_t));
	cdt_init(sk->cdt, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	PCB_PADSTACK_LOOP(PCB->Data);
	{
		if (pcb_pstk_shape_at(PCB, padstack, layer) != NULL) {
			cdt_insert_point(sk->cdt, padstack->x, padstack->y);
		}
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(PCB->Data);
	{
		PCB_PADSTACK_LOOP(subc->data);
		{
			if (pcb_pstk_shape_at(PCB, padstack, layer) != NULL) {
				cdt_insert_point(sk->cdt, padstack->x, padstack->y);
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;

	sk->ui_layer_cdt = pcb_uilayer_alloc(pcb_sketch_route_cookie, "CDT", layer->meta.real.color);
}

static void sketch_free(sketch_t *sk)
{
	if (sk->cdt != NULL) {
		cdt_free(sk->cdt);
		free(sk->cdt);
		sk->cdt = NULL;
	}
	if (sk->ui_layer_cdt != NULL) {
		pcb_uilayer_free(sk->ui_layer_cdt);
		sk->ui_layer_cdt = NULL;
	}
}


static const char pcb_acts_skretriangulate[] = "skretriangulate()";
static const char pcb_acth_skretriangulate[] = "Construct a new CDT on the current layer";
fgw_error_t pcb_act_skretriangulate(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	sketch_free(&sketch);
	sketch_create_for_layer(&sketch, CURRENT);

	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t sketch_route_action_list[] = {
	{"skretriangulate", pcb_act_skretriangulate, pcb_acth_skretriangulate, pcb_acts_skretriangulate}
};

PCB_REGISTER_ACTIONS(sketch_route_action_list, pcb_sketch_route_cookie)

int pplg_check_ver_sketch_route(int ver_needed) { return 0; }

void pplg_uninit_sketch_route(void)
{
	pcb_remove_actions_by_cookie(pcb_sketch_route_cookie);
	sketch_free(&sketch);
}


#include "dolists.h"
int pplg_init_sketch_route(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(sketch_route_action_list, pcb_sketch_route_cookie)

	memset(&sketch, 0, sizeof(sketch_t));

	return 0;
}
