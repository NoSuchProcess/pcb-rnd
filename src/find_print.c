/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "data_it.h"

/* Connection export/output functions */

/* copyright: written from 0 */
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

/* prints all unused pins of an element to file FP */
/* copyright: rewritten */
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
				pcb_print_conn_subc_name(subc, f);
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
/* copyright: function written from 0 */
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
	PrintElementNameList(sc, ctx->f);
	return 0;
}


/* ---------------------------------------------------------------------------
 * finds all connections to the pins of the passed element.
 * The result is written to file FP
 * Returns pcb_true if operation was aborted
 */
/* copyright: function got rewritten */
static void pcb_print_subc_conns(FILE *f, pcb_subc_t *subc)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	pcb_find_t fctx;
	term_cb_t cbctx;

	pcb_print_conn_subc_name(subc, f);

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

/* ---------------------------------------------------------------------------
 * find all unused pins of all element
 */
/* copyright: rewritten */
void pcb_lookup_unused_pins(FILE *f, int do_select)
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

/* ---------------------------------------------------------------------------
 * find all connections to pins within one element
 */
/* copyright: rewritten */
void pcb_lookup_subc_conns(FILE *f, pcb_subc_t *subc)
{
	pcb_print_subc_conns(f, subc);
	if (conf_core.editor.beep_when_finished)
		pcb_gui->beep();
}


/* ---------------------------------------------------------------------------
 * find all connections to pins of all element
 */
void pcb_lookup_conns_to_all_elements(FILE * FP)
{
	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_print_subc_conns(FP, subc);
		SEPARATE(FP);
	}
	PCB_END_LOOP;

	if (conf_core.editor.beep_when_finished)
		pcb_gui->beep();
	pcb_redraw();
}
