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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Query language - actions */

#include "global.h"
#include "query.h"
#include "query_y.h"
#include "query_exec.h"
#include "set.h"
#include "draw.h"

static const char query_action_syntax[] =
	"query(dump, expr) - dry run: compile and dump an expression\n"
	;
static const char query_action_help[] = "Perform various queries on PCB data.";

typedef struct {
	int trues, falses;
} eval_stat_t;

static void eval_cb(void *user_ctx, pcb_qry_val_t *res, pcb_obj_t *current)
{
	eval_stat_t *st = (eval_stat_t *)user_ctx;
	int t = pcb_qry_is_true(res);

	printf(" %s", t ? "true" : "false");
	if (t) {
		char *resv;
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

static void select_cb(void *user_ctx, pcb_qry_val_t *res, pcb_obj_t *current)
{
	if (!pcb_qry_is_true(res))
		return;
	if (PCB_OBJ_IS_CLASS(current->type, PCB_OBJ_CLASS_OBJ))
		SET_FLAG(PCB_FLAG_SELECTED, current->data.anyobj);
}


static void unselect_cb(void *user_ctx, pcb_qry_val_t *res, pcb_obj_t *current)
{
	if (!pcb_qry_is_true(res))
		return;
	if (PCB_OBJ_IS_CLASS(current->type, PCB_OBJ_CLASS_OBJ))
		CLEAR_FLAG(PCB_FLAG_SELECTED, current->data.anyobj);
}


static int run_script(const char *script, void (*cb)(void *user_ctx, pcb_qry_val_t *res, pcb_obj_t *current), void *user_ctx)
{
	pcb_qry_node_t *prg = NULL;

	pcb_qry_set_input(script);
	qry_parse(&prg);

	if (prg == NULL) {
		Message(PCB_MSG_ERROR, "Compilation error.\n");
		return -1;
	}

	return pcb_qry_run(prg, cb, user_ctx);
}

static int query_action(int argc, const char **argv, Coord x, Coord y)
{
	const char *cmd = argc > 0 ? argv[0] : 0;

	if (cmd == NULL) {
		return -1;
	}

	if (strcmp(cmd, "dump") == 0) {
		pcb_qry_node_t *prg = NULL;
		printf("Script dump: '%s'\n", argv[1]);
		pcb_qry_set_input(argv[1]);
		qry_parse(&prg);
		pcb_qry_dump_tree(" ", prg);
		return 0;
	}

	if (strcmp(cmd, "eval") == 0) {
		int errs;
		eval_stat_t st;

		memset(&st, 0, sizeof(st));
		printf("Script eval: '%s'\n", argv[1]);
		errs = run_script(argv[1], eval_cb, &st);

		if (errs < 0)
			printf("Failed to run the query\n");
		else
			printf("eval statistics: true=%d false=%d errors=%d\n", st.trues, st.falses, errs);
		return 0;
	}

	if (strcmp(cmd, "select") == 0) {
		if (run_script(argv[1], select_cb, NULL) < 0)
			printf("Failed to run the query\n");
		SetChangedFlag(pcb_true);
		Redraw();
		return 0;
	}

	if (strcmp(cmd, "unselect") == 0) {
		if (run_script(argv[1], unselect_cb, NULL) < 0)
			printf("Failed to run the query\n");
		SetChangedFlag(pcb_true);
		Redraw();
		return 0;
	}

	return -1;
}

HID_Action query_action_list[] = {
	{"query", NULL, query_action,
	 query_action_help, query_action_syntax}
};

REGISTER_ACTIONS(query_action_list, NULL)

#include "dolists.h"
void query_action_reg(const char *cookie)
{
	REGISTER_ACTIONS(query_action_list, cookie)
}
