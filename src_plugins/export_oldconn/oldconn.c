/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "actions.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include "draw.h"
#include "plugins.h"
#include "plug_io.h"
#include "find.h"
#include "macro.h"
#include "obj_subc_parent.h"
#include "undo.h"
#include "funchash_core.h"
#include "search.h"

#include "hid_init.h"
#include "hid_attrib.h"

const char *oldconn_cookie = "export_oldconn HID";

#define SEPARATE(fp) \
	do { \
		int __i__; \
		FILE *__f__ = (fp); \
		fputc('#', __f__); \
		for (__i__ = conf_core.appearance.messages.char_per_line; __i__ > 0; __i__--) \
			fputc('=', __f__); \
		fputc('\n', __f__); \
	} while(0)

/* writes the several names of a subcircuit to a file */
static void print_subc_name(FILE *f, pcb_subc_t *subc)
{
	fputc('(', f);
	pcb_print_quoted_string(f, (char *)PCB_EMPTY(pcb_attribute_get(&subc->Attributes, "footprint")));
	fputc(' ', f);
	pcb_print_quoted_string(f, (char *)PCB_EMPTY(subc->refdes));
	fputc(' ', f);
	pcb_print_quoted_string(f, (char *)PCB_EMPTY(pcb_attribute_get(&subc->Attributes, "value")));
	fputs(")\n", f);
}

static void pcb_print_conn_subc_name(FILE *f, pcb_subc_t *subc)
{
	fputs("Element", f);
	print_subc_name(f, subc);
	fputs("{\n", f);
}

static int count_term_cb(pcb_find_t *fctx, pcb_any_obj_t *o)
{
	unsigned long *cnt = fctx->user_data;

	if (o->term == NULL)
		return 0;

	(*cnt)++;
	if ((*cnt) > 1)
		return 1; /* stop searching after the second object - no need to know how many terminals are connected, the fact that it's more than 1 is enough */
	return 0;
}

/* prints all unused pins of a subcircuit to f */
static void print_select_unused_subc_terms(FILE *f, pcb_subc_t *subc, int do_select)
{
	unsigned long cnt;
	pcb_find_t fctx;
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	int subc_announced = 0;

	memset(&fctx, 0, sizeof(fctx));
	fctx.user_data = &cnt;
	fctx.found_cb = count_term_cb;

	for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		if (o->term == NULL) /* consider named terminals only */
			continue;

		cnt = 0;
		pcb_find_from_obj(&fctx, PCB->Data, o);
		pcb_find_free(&fctx);

		if (cnt <= 1) {
			if (!subc_announced) {
				pcb_print_conn_subc_name(f, subc);
				subc_announced = 1;
			}

			fputc('\t', f);
			pcb_print_quoted_string(f, (char *)PCB_EMPTY(o->term));
			fputc('\n', f);
			if (do_select) {
				PCB_FLAG_SET(PCB_FLAG_SELECTED, o);
				pcb_draw_obj(o);
			}
		}
	}

	/* print separator if element has unused pins or pads */
	if (subc_announced) {
		fputs("}\n\n", f);
		SEPARATE(f);
	}
}

typedef struct {
	FILE *f;
	pcb_any_obj_t *start;
} term_cb_t;

static int print_term_conn_cb(pcb_find_t *fctx, pcb_any_obj_t *o)
{
	term_cb_t *ctx = fctx->user_data;
	pcb_subc_t *sc;

	if (ctx->start == o)
		return 0;

	sc = pcb_obj_parent_subc(o);
	if (sc == NULL)
		return 0;

	fputs("\t\t", ctx->f);
	pcb_print_quoted_string(ctx->f, PCB_EMPTY(o->term));
	fputs(" ", ctx->f);
	print_subc_name(ctx->f, sc);
	return 0;
}


/* Find connected terminals to each terminal of subc and write them to f. */
static void pcb_print_subc_conns(FILE *f, pcb_subc_t *subc)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	pcb_find_t fctx;
	term_cb_t cbctx;

	pcb_print_conn_subc_name(f, subc);

	cbctx.f = f;
	memset(&fctx, 0, sizeof(fctx));

	for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		unsigned long res;

		if (o->term == NULL) /* consider named terminals only */
			continue;

		fputs("\t", f);
		pcb_print_quoted_string(f, PCB_EMPTY(o->term));
		fputs("\n\t{\n", f);

		cbctx.start = o;
		fctx.user_data = &cbctx;
		fctx.found_cb = print_term_conn_cb;
		pcb_find_from_obj(&fctx, PCB->Data, o);
		pcb_find_free(&fctx);

		fputs("\t}\n", f);
	}
	fputs("}\n\n", f);
}

/* Find and print (to f) all unused pins of all subcircuits */
static void pcb_lookup_unused_pins(FILE *f, int do_select)
{
	PCB_SUBC_LOOP(PCB->Data);
	{
		print_select_unused_subc_terms(f, subc, do_select);
	}
	PCB_END_LOOP;

	if (conf_core.editor.beep_when_finished)
		pcb_gui->beep();

	if (do_select) {
		pcb_undo_inc_serial();
		pcb_draw();
	}
}

/* Find and print (to f) all connections from terminals of subc */
static void pcb_lookup_subc_conns(FILE *f, pcb_subc_t *subc)
{
	pcb_print_subc_conns(f, subc);
	if (conf_core.editor.beep_when_finished)
		pcb_gui->beep();
}

/* Find all connections from all terminals of all subcircuits and print in f. */
static void pcb_lookup_conns_to_all_subcs(FILE *f)
{
	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_print_subc_conns(f, subc);
		SEPARATE(f);
	}
	PCB_END_LOOP;

	if (conf_core.editor.beep_when_finished)
		pcb_gui->beep();
	pcb_redraw();
}


static const char pcb_acts_ExportOldConn[] = "ExportOldConn(AllConnections|AllUnusedPins|ElementConnections,filename)\n";
static const char pcb_acth_ExportOldConn[] = "Export galvanic connection data in an old, custom file format.";
/* DOC: exportoldconn.html */
fgw_error_t pcb_act_ExportOldConn(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	const char *name = NULL;
	FILE *f;
	pcb_bool result;
	void *ptrtmp;
	pcb_coord_t x, y;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, ExportOldConn, op = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_STR, ExportOldConn, name = argv[2].val.str);
	PCB_ACT_IRES(0);

	switch(op) {
		case F_AllConnections:
			f = pcb_check_and_open_file(name, pcb_true, pcb_false, &result, NULL);
			if (f != NULL) {
				pcb_lookup_conns_to_all_subcs(f);
				fclose(f);
			}
			return 0;

		case F_AllUnusedPins:
			f = pcb_check_and_open_file(name, pcb_true, pcb_false, &result, NULL);
			if (f != NULL) {
				pcb_lookup_unused_pins(f, 1);
				fclose(f);
				pcb_board_set_changed_flag(pcb_true);
			}
			return 0;

		case F_ElementConnections:
		case F_SubcConnections:
			pcb_hid_get_coords("Click on a subc", &x, &y, 0);
			if (pcb_search_screen(x, y, PCB_OBJ_SUBC, &ptrtmp, &ptrtmp, &ptrtmp) != PCB_OBJ_VOID) {
				pcb_subc_t *subc = (pcb_subc_t *) ptrtmp;
				f = pcb_check_and_open_file(name, pcb_true, pcb_false, &result, NULL);
				if (f != NULL) {
					pcb_lookup_subc_conns(f, subc);
					fclose(f);
				}
			}
			return 0;
	}
	PCB_ACT_FAIL(ExportOldConn);
}

static pcb_action_t oldconn_action_list[] = {
	{"ExportOldConn", pcb_act_ExportOldConn, pcb_acth_ExportOldConn, pcb_acts_ExportOldConn}
};

PCB_REGISTER_ACTIONS(oldconn_action_list, oldconn_cookie)

int pplg_check_ver_export_oldconn(int ver_needed) { return 0; }

void pplg_uninit_export_oldconn(void)
{
}

#include "dolists.h"

int pplg_init_export_oldconn(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(oldconn_action_list, oldconn_cookie)

	return 0;
}
