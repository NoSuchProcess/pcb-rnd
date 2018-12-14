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

static void PrepareNextLoop(void);

/* Count terminals that are found, up to maxt */
static void count_terms(vtp0_t *out, pcb_data_t *data, pcb_cardinal_t maxt, pcb_cardinal_t *cnt)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;

	for(o = pcb_data_first(&it, data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		if ((o->term != NULL) && PCB_FLAG_TEST(PCB_FLAG_FOUND, o)) {
			(*cnt)++;
			if (out != NULL)
				vtp0_append(out, o);
		}
		if (o->type == PCB_OBJ_SUBC)
			count_terms(out, ((pcb_subc_t *)o)->data, maxt, cnt);
		if (*cnt >= maxt)
			return;
	}
	return;
}

/* prints all unused pins of an element to file FP */
static pcb_bool print_select_unused_subc_terms(pcb_subc_t *subc, FILE * FP)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	pcb_bool first = pcb_true;

	for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		pcb_cardinal_t number = 0;
		if (o->term == NULL) /* consider named terminals only */
			continue;

		/* reset found objects for the next pin */
		PrepareNextLoop();

		ListStart(o);
		DoIt(pcb_true, pcb_true);

		count_terms(NULL, PCB->Data, 2, &number);
		if (number <= 1) {
			/* output of element name if not already done */
			if (first) {
				pcb_print_conn_subc_name(subc, FP);
				first = pcb_false;
			}

			/* write name to list */
			fputc('\t', FP);
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(o->term));
			fputc('\n', FP);
			PCB_FLAG_SET(PCB_FLAG_SELECTED, o);
			pcb_draw_obj(o);
		}
	}

	/* print separator if element has unused pins or pads */
	if (!first) {
		fputs("}\n\n", FP);
		SEPARATE(FP);
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * resets some flags for looking up the next pin/pad
 */
static void PrepareNextLoop(void)
{
	pcb_cardinal_t layer;

	/* reset found LOs for the next pin */
	for (layer = 0; layer < pcb_max_layer; layer++) {
		LineList[layer].Location = LineList[layer].Number = 0;
		ArcList[layer].Location = ArcList[layer].Number = 0;
		PolygonList[layer].Location = PolygonList[layer].Number = 0;
	}

	/* reset Padstacks */
	PadstackList.Number = PadstackList.Location = 0;
	pcb_reset_conns(pcb_false);
}

/* ---------------------------------------------------------------------------
 * prints all found connections of a pin to file FP
 * the connections are stacked in 'PVList'
 */
static void print_term_conns(FILE *FP, pcb_subc_t *subc)
{
	vtp0_t lst;
	pcb_cardinal_t cnt = 0, n;

	vtp0_init(&lst);
	count_terms(&lst, PCB->Data, (1<<15), &cnt);

	if (cnt == 0)
		return;

	/* find a starting pin, within this subc */
	for(n = 0; n < vtp0_len(&lst); n++) {
		pcb_any_obj_t *o = lst.array[n];
		if (pcb_obj_parent_subc(o) == subc) {
			fputc('\t', FP);
			pcb_print_quoted_string(FP, o->term);
			fprintf(FP, "\n\t{\n");
			lst.array[n] = NULL;
			break;
		}
	}

	for(n = 0; n < vtp0_len(&lst); n++) {
		pcb_any_obj_t *o = lst.array[n];
		if (o != NULL)
			pcb_print_conn_list_entry((char *)PCB_EMPTY(o->term), pcb_obj_parent_subc(o), pcb_false, FP);
	}

	vtp0_uninit(&lst);
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
void pcb_lookup_unused_pins(FILE * FP)
{
	/* reset all currently marked connections */
	User = pcb_true;
	pcb_reset_conns(pcb_true);
	pcb_conn_lookup_init();

	PCB_SUBC_LOOP(PCB->Data);
	{
		/* break if abort dialog returned pcb_true;
		 * passing NULL as filedescriptor discards the normal output */
		if (print_select_unused_subc_terms(subc, FP))
			break;
	}
	PCB_END_LOOP;

	if (conf_core.editor.beep_when_finished)
		pcb_gui->beep();
	pcb_conn_lookup_uninit();
	pcb_undo_inc_serial();
	User = pcb_false;
	pcb_draw();
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
