/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  auto routing with external router process
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
#include <genvector/vts0.h>

#include "board.h"
#include "data.h"
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include "conf_core.h"
#include "obj_pstk_inlines.h"
#include "src_plugins/lib_compat_help/pstk_compat.h"
#include "src_plugins/lib_netmap/netmap.h"

static const char *extern_cookie = "extern autorouter plugin";

typedef enum {
	ERSC_BOARD, ERSC_SELECTED
} ext_route_scope_t;

typedef struct {
	const char *name;
	int (*route)(pcb_board_t *pcb, ext_route_scope_t scope, const char *method, int argc, fgw_arg_t *argv);
	int (*list_methods)(rnd_hidlib_t *hl, vts0_t *dst);
	rnd_export_opt_t *(*list_conf)(rnd_hidlib_t *hl, const char *method);
} ext_router_t;

#include "e_route-rnd.c"

static const ext_router_t *routers[] = { &route_rnd, NULL };

static const ext_router_t *find_router(const char *name)
{
	const ext_router_t **r;

	for(r = routers; *r != NULL; r++)
		if (strcmp((*r)->name, name) == 0)
			return *r;

	return NULL;
}

static void extroute_gui(pcb_board_t *pcb)
{
	const ext_router_t **r;
		vts0_t methods = {0};

	printf("GUI!\n");
	for(r = routers; *r != NULL; r++) {
		int n;
		rnd_export_opt_t *table, *cfg;

		printf(" router=%s\n", (*r)->name);
		methods.used = 0;
		(*r)->list_methods(&pcb->hidlib, &methods);
		for(n = 0; n < methods.used; n+=2) {
			printf("  method=%s (%s)\n", methods.array[n], methods.array[n+1]);
			table = (*r)->list_conf(&pcb->hidlib, methods.array[n]);
			for(cfg = table; cfg->name != NULL; cfg++)
				printf("    %s: %s\n", cfg->name, cfg->help_text);
			free(methods.array[n]);
			free(methods.array[n+1]);
		}
	}
	vts0_uninit(&methods);
}

static const char pcb_acts_extroute[] = "extroute(board|selected, router, [confkey=value, ...])";
static const char pcb_acth_extroute[] = "Executed external autorouter to route the board or parts of the board";
fgw_error_t pcb_act_extroute(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *scope, *router_;
	char *router, *method;
	ext_route_scope_t scp;
	const ext_router_t *r;
	pcb_board_t *pcb = PCB_ACT_BOARD;

	RND_ACT_IRES(0);
	if (argc < 2) {
		extroute_gui(pcb);
		/* GUI is non-modal so always succesful */
		return 0;
	}

	RND_ACT_CONVARG(1, FGW_STR, extroute, scope = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, extroute, router_ = argv[2].val.str);
	
	if (strcmp(scope, "board") == 0) scp = ERSC_BOARD;
	else if (strcmp(scope, "selected") == 0) scp = ERSC_SELECTED;
	else {
		rnd_message(RND_MSG_ERROR, "Invalid scope: '%s'\n", scope);
		return FGW_ERR_ARG_CONV;
	}

	router = rnd_strdup(router_);
	method = strchr(router, '/');
	if (method != NULL) {
		*method = '\0';
		method++;
		if (*method == '\0')
			method = NULL;
	}

	r = find_router(router);
	if (r == NULL) {
		free(router);
		rnd_message(RND_MSG_ERROR, "Invalid router: '%s'\n", scope);
		return FGW_ERR_ARG_CONV;
	}

	if (r->route != NULL)
		RND_ACT_IRES(r->route(pcb, scp, method, argc-3, argv+3));

	free(router);
	return 0;
}

static rnd_action_t extern_action_list[] = {
	{"extroute", pcb_act_extroute, pcb_acth_extroute, pcb_acts_extroute}
};

int pplg_check_ver_ar_extern(int ver_needed) { return 0; }

void pplg_uninit_ar_extern(void)
{
	rnd_remove_actions_by_cookie(extern_cookie);
}


int pplg_init_ar_extern(void)
{
	RND_API_CHK_VER;

	RND_REGISTER_ACTIONS(extern_action_list, extern_cookie)

	return 0;
}
