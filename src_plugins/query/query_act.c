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

/* Query language - actions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "actions.h"
#include "query.h"
#include "query_y.h"
#include "query_exec.h"
#include "draw.h"
#include "select.h"
#include "board.h"
#include "macro.h"

static const char pcb_acts_query[] =
	"query(dump, expr) - dry run: compile and dump an expression\n"
	;
static const char pcb_acth_query[] = "Perform various queries on PCB data.";

typedef struct {
	int trues, falses;
} eval_stat_t;

static void eval_cb(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current)
{
	char *resv;
	eval_stat_t *st = (eval_stat_t *)user_ctx;
	int t;

	if (res->type == PCBQ_VT_VOID) {
		printf(" <void>\n");
		st->falses++;
		return;
	}

	if (res->type != PCBQ_VT_COORD) {
		st->trues++;
		resv = pcb_query_sprint_val(res);
		printf(" %s\n", resv);
		free(resv);
		return;
	}

	t = pcb_qry_is_true(res);
	printf(" %s", t ? "true" : "false");
	if (t) {
		resv = pcb_query_sprint_val(res);
		printf(" (%s)\n", resv);
		free(resv);
		st->trues++;
	}
	else {
		printf("\n");
		st->falses++;
	}
}

typedef struct {
	pcb_cardinal_t cnt;
	pcb_change_flag_t how;
} select_t;

static void select_cb(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current)
{
	select_t *sel = (select_t *)user_ctx;
	if (!pcb_qry_is_true(res))
		return;
	if (PCB_OBJ_IS_CLASS(current->type, PCB_OBJ_CLASS_OBJ)) {
		int state_wanted = (sel->how == PCB_CHGFLG_SET);
		int state_is     = PCB_FLAG_TEST(PCB_FLAG_SELECTED, current);
		if (state_wanted != state_is) {
			PCB_FLAG_CHANGE(sel->how, PCB_FLAG_SELECTED, current);
			sel->cnt++;
		}
	}
}

static int run_script(const char *script, void (*cb)(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current), void *user_ctx)
{
	pcb_qry_node_t *prg = NULL;

	pcb_qry_set_input(script);
	qry_parse(&prg);

	if (prg == NULL) {
		pcb_message(PCB_MSG_ERROR, "Compilation error.\n");
		return -1;
	}

	return pcb_qry_run(prg, cb, user_ctx);
}

static fgw_error_t pcb_act_query(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *arg = NULL;
	select_t sel;

	sel.cnt = 0;

	PCB_ACT_CONVARG(1, FGW_STR, query, cmd = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, query, arg = argv[2].val.str);

	if (strcmp(cmd, "version") == 0) {
		PCB_ACT_IRES(0100); /* 1.0 */
		return 0;
	}

	if (strcmp(cmd, "dump") == 0) {
		pcb_qry_node_t *prg = NULL;
		printf("Script dump: '%s'\n", arg);
		pcb_qry_set_input(arg);
		qry_parse(&prg);
		pcb_qry_dump_tree(" ", prg);
		PCB_ACT_IRES(0);
		return 0;
	}

	if (strcmp(cmd, "eval") == 0) {
		int errs;
		eval_stat_t st;

		memset(&st, 0, sizeof(st));
		printf("Script eval: '%s'\n", arg);
		errs = run_script(arg, eval_cb, &st);

		if (errs < 0)
			printf("Failed to run the query\n");
		else
			printf("eval statistics: true=%d false=%d errors=%d\n", st.trues, st.falses, errs);
		PCB_ACT_IRES(0);
		return 0;
	}

	if (strcmp(cmd, "select") == 0) {
		sel.how = PCB_CHGFLG_SET;
		if (run_script(arg, select_cb, &sel) < 0)
			printf("Failed to run the query\n");
		if (sel.cnt > 0) {
			pcb_board_set_changed_flag(pcb_true);
			pcb_hid_redraw(PCB);
		}
		PCB_ACT_IRES(0);
		return 0;
	}

	if (strcmp(cmd, "unselect") == 0) {
		sel.how = PCB_CHGFLG_CLEAR;
		if (run_script(arg, select_cb, &sel) < 0)
			printf("Failed to run the query\n");
		if (sel.cnt > 0) {
			pcb_board_set_changed_flag(pcb_true);
			pcb_hid_redraw(PCB);
		}
		PCB_ACT_IRES(0);
		return 0;
	}

	PCB_ACT_IRES(-1);
	return 0;
}

pcb_action_t query_action_list[] = {
	{"query", pcb_act_query, pcb_acth_query, pcb_acts_query}
};

PCB_REGISTER_ACTIONS(query_action_list, NULL)

#include "dolists.h"
void query_action_reg(const char *cookie)
{
	PCB_REGISTER_ACTIONS(query_action_list, cookie)
}
