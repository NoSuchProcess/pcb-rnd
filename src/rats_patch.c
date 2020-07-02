/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015..2020 Tibor 'Igor2' Palinkas
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

#include "rats_patch.h"
#include "genht/htsp.h"
#include "genht/hash.h"

#include "config.h"

#include <assert.h>

#include <librnd/core/actions.h>
#include "data.h"
#include <librnd/core/error.h>
#include "move.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include "funchash_core.h"
#include "search.h"
#include "undo.h"
#include "conf_core.h"
#include "netlist.h"

static void rats_patch_remove(pcb_board_t *pcb, pcb_ratspatch_line_t * n, int do_free);

static const char core_ratspatch_cookie[] = "core-rats-patch";

const char *pcb_netlist_names[PCB_NUM_NETLISTS] = {
	"input",
	"edited"
};

/*** undoable ratspatch append */

typedef struct {
	pcb_board_t *pcb;
	pcb_rats_patch_op_t op;
	char *id;
	char *a1;
	char *a2;
	int used_by_attr; /* can not free */
} undo_ratspatch_append_t;

static void pcb_ratspatch_append_(pcb_board_t *pcb, pcb_rats_patch_op_t op, char *id, char *a1, char *a2)
{
	pcb_ratspatch_line_t *n;

	n = malloc(sizeof(pcb_ratspatch_line_t));

	n->op = op;
	n->id = id;
	n->arg1.net_name = a1;
	if (a2 != NULL)
		n->arg2.attrib_val = a2;
	else
		n->arg2.attrib_val = NULL;

	/* link in */
	n->prev = pcb->NetlistPatchLast;
	if (pcb->NetlistPatches != NULL) {
		pcb->NetlistPatchLast->next = n;
		pcb->NetlistPatchLast = n;
	}
	else
		pcb->NetlistPatchLast = pcb->NetlistPatches = n;
	n->next = NULL;
}

static int undo_ratspatch_append_redo(void *udata)
{
	undo_ratspatch_append_t *a = udata;
	pcb_ratspatch_append_(a->pcb, a->op, a->id, a->a1, a->a2);
	a->used_by_attr = 1;
	return 0;
}

static int undo_ratspatch_append_undo(void *udata)
{
	undo_ratspatch_append_t *a = udata;
	pcb_ratspatch_line_t *n;

	/* because undo and redo should be sequential, we only need to remove the last entry from the rats patch list */
	n = a->pcb->NetlistPatchLast;
	assert(n->next == NULL);

	a->pcb->NetlistPatchLast = n->prev;
	if (n->prev != NULL)
		n->prev->next = NULL;

	if (a->pcb->NetlistPatches == n)
		a->pcb->NetlistPatches = NULL;

	free(n); /* do not free the fields: they are shared with the undo slot, allocated at/for the undo slot */

	a->used_by_attr = 0;
	return 0;
}

static void undo_ratspatch_append_print(void *udata, char *dst, size_t dst_len)
{
	undo_ratspatch_append_t *a = udata;
	rnd_snprintf(dst, dst_len, "ratspatch_append: op=%d '%s' '%s' '%s' (used by attr: %d)", a->op, RND_EMPTY(a->id), RND_EMPTY(a->a1), RND_EMPTY(a->a2), a->used_by_attr);
}

static void undo_ratspatch_append_free(void *udata)
{
	undo_ratspatch_append_t *a = udata;
	if (!a->used_by_attr) {
		free(a->id);
		free(a->a1);
		free(a->a2);
	}
	a->id = a->a1 = a->a2 = NULL;
}

static const uundo_oper_t undo_ratspatch_append = {
	core_ratspatch_cookie,
	undo_ratspatch_append_free,
	undo_ratspatch_append_undo,
	undo_ratspatch_append_redo,
	undo_ratspatch_append_print
};

void pcb_ratspatch_append(pcb_board_t *pcb, pcb_rats_patch_op_t op, const char *id, const char *a1, const char *a2, int undoable)
{
	undo_ratspatch_append_t *a;
	char *nid = NULL, *na1 = NULL, *na2 = NULL;

	if (id != NULL) nid = rnd_strdup(id);
	if (a1 != NULL) na1 = rnd_strdup(a1);
	if (a2 != NULL) na2 = rnd_strdup(a2);

	if (undoable) {
		a = pcb_undo_alloc(pcb, &undo_ratspatch_append, sizeof(undo_ratspatch_append_t));
		a->pcb = pcb;
		a->op = op;
		a->id = nid;
		a->a1 = na1;
		a->a2 = na2;
		a->used_by_attr = 1;
	}

	pcb_ratspatch_append_(pcb, op, nid, na1, na2);
}

static void rats_patch_free_fields(pcb_ratspatch_line_t *n)
{
	if (n->id != NULL)
		free(n->id);
	if (n->arg1.net_name != NULL)
		free(n->arg1.net_name);
	if (n->arg2.attrib_val != NULL)
		free(n->arg2.attrib_val);
}

void pcb_ratspatch_destroy(pcb_board_t *pcb)
{
	pcb_ratspatch_line_t *n, *next;

	for(n = pcb->NetlistPatches; n != NULL; n = next) {
		next = n->next;
		rats_patch_free_fields(n);
		free(n);
	}
}

void pcb_ratspatch_append_optimize(pcb_board_t *pcb, pcb_rats_patch_op_t op, const char *id, const char *a1, const char *a2)
{
	pcb_rats_patch_op_t seek_op;
	pcb_ratspatch_line_t *n;

	switch (op) {
	case RATP_ADD_CONN:
		seek_op = RATP_DEL_CONN;
		break;
	case RATP_DEL_CONN:
		seek_op = RATP_ADD_CONN;
		break;
	case RATP_CHANGE_ATTRIB:
		seek_op = RATP_CHANGE_ATTRIB;
		break;
	}

	/* keep the list clean: remove the last operation that becomes obsolete by the new one */
	for (n = pcb->NetlistPatchLast; n != NULL; n = n->prev) {
		if ((n->op == seek_op) && (strcmp(n->id, id) == 0)) {
			switch (op) {
			case RATP_CHANGE_ATTRIB:
				if (strcmp(n->arg1.attrib_name, a1) != 0)
					break;
				rats_patch_remove(pcb, n, 1);
				goto quit;
			case RATP_ADD_CONN:
			case RATP_DEL_CONN:
				if (strcmp(n->arg1.net_name, a1) != 0)
					break;
				rats_patch_remove(pcb, n, 1);
				goto quit;
			}
		}
	}

quit:;
	pcb_ratspatch_append(pcb, op, id, a1, a2, 0);
}

/* Unlink n from the list; if do_free is non-zero, also free fields and n */
static void rats_patch_remove(pcb_board_t *pcb, pcb_ratspatch_line_t * n, int do_free)
{
	/* if we are the first or last... */
	if (n == pcb->NetlistPatches)
		pcb->NetlistPatches = n->next;
	if (n == pcb->NetlistPatchLast)
		pcb->NetlistPatchLast = n->prev;

	/* extra ifs, just in case n is already unlinked */
	if (n->prev != NULL)
		n->prev->next = n->next;
	if (n->next != NULL)
		n->next->prev = n->prev;

	if (do_free) {
		rats_patch_free_fields(n);
		free(n);
	}
}

static int rats_patch_apply_conn(pcb_board_t *pcb, pcb_ratspatch_line_t *patch, int del)
{
	pcb_net_t *net;
	pcb_net_term_t *term;
	char *sep, *termid;
	int refdeslen;


	sep = strchr(patch->id, '-');
	if (sep == NULL)
		return 1; /* invalid id */
	termid = sep+1;
	refdeslen = sep - patch->id;
	if (refdeslen < 1)
		return 1; /* invalid id */

	net = pcb_net_get(pcb, &pcb->netlist[PCB_NETLIST_EDITED], patch->arg1.net_name, PCB_NETA_ALLOC);
	for(term = pcb_termlist_first(&net->conns); term != NULL; term = pcb_termlist_next(term)) {
		if ((strncmp(patch->id, term->refdes, refdeslen) == 0) && (strcmp(termid, term->term) == 0)) {
			if (del) {
				/* want to delete and it's on the list */
				pcb_net_term_del(net, term);
				return 0;
			}
			/* want to add, and pin is on the list -> already added */
			return 1;
		}
	}

	/* If we got here, pin is not on the list */
	if (del)
		return 1;

	/* Wanted to add, let's add it */
	*sep = '\0';
	term = pcb_net_term_get(net, patch->id, termid, PCB_NETA_ALLOC);
	*sep = '-';
	if (term != NULL)
		return 0;
	return 1;
}

static int rats_patch_apply_attrib(pcb_board_t *pcb, pcb_ratspatch_line_t *patch)
{
	pcb_net_t *net = pcb_net_get(pcb, &pcb->netlist[PCB_NETLIST_EDITED], patch->id, PCB_NETA_ALLOC);
	if (net == NULL)
		return 1;
	if ((patch->arg2.attrib_val == NULL) || (*patch->arg2.attrib_val == '\0'))
		pcb_attribute_remove(&net->Attributes, patch->arg1.attrib_name);
	else
		pcb_attribute_put(&net->Attributes, patch->arg1.attrib_name, patch->arg2.attrib_val);
	return 0;
}

int pcb_ratspatch_apply(pcb_board_t *pcb, pcb_ratspatch_line_t *patch)
{
	switch (patch->op) {
		case RATP_ADD_CONN:
			return rats_patch_apply_conn(pcb, patch, 0);
		case RATP_DEL_CONN:
			return rats_patch_apply_conn(pcb, patch, 1);
		case RATP_CHANGE_ATTRIB:
			return rats_patch_apply_attrib(pcb, patch);
	}
	return 0;
}

void pcb_ratspatch_make_edited(pcb_board_t *pcb)
{
	pcb_ratspatch_line_t *n;

	pcb_netlist_uninit(&(pcb->netlist[PCB_NETLIST_EDITED]));
	pcb_netlist_init(&(pcb->netlist[PCB_NETLIST_EDITED]));
	pcb_netlist_copy(pcb, &(pcb->netlist[PCB_NETLIST_EDITED]), &(pcb->netlist[PCB_NETLIST_INPUT]));

	for (n = pcb->NetlistPatches; n != NULL; n = n->next)
		pcb_ratspatch_apply(pcb, n);
}

int pcb_rats_patch_export(pcb_board_t *pcb, pcb_ratspatch_line_t *pat, rnd_bool need_info_lines, void (*cb)(void *ctx, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val), void *ctx)
{
	pcb_ratspatch_line_t *n;

	if (need_info_lines) {
		htsp_t *seen;
		seen = htsp_alloc(strhash, strkeyeq);

		/* have to print net_info lines */
		for (n = pat; n != NULL; n = n->next) {
			switch (n->op) {
			case RATP_ADD_CONN:
			case RATP_DEL_CONN:
				if (htsp_get(seen, n->arg1.net_name) == NULL) {
					{
						/* document the original (input) state */
						pcb_net_t *net = pcb_net_get(pcb, &pcb->netlist[PCB_NETLIST_INPUT], n->arg1.net_name, 0);
						if (net != NULL) {
							pcb_net_term_t *term;
							htsp_set(seen, n->arg1.net_name, net);
							cb(ctx, PCB_RPE_INFO_BEGIN, n->arg1.net_name, NULL, NULL);
							for(term = pcb_termlist_first(&net->conns); term != NULL; term = pcb_termlist_next(term)) {
								char *tmp = rnd_concat(term->refdes, "-", term->term, NULL);
								cb(ctx, PCB_RPE_INFO_TERMINAL, n->arg1.net_name, NULL, tmp);
								free(tmp);
							}
							cb(ctx, PCB_RPE_INFO_END, n->arg1.net_name, NULL, NULL);
						}
					}
				}
			case RATP_CHANGE_ATTRIB:
				break;
			}
		}
		htsp_free(seen);
	}

	/* action lines */
	for (n = pat; n != NULL; n = n->next) {
		switch (n->op) {
		case RATP_ADD_CONN:
			cb(ctx, PCB_RPE_CONN_ADD, n->arg1.net_name, NULL, n->id);
			break;
		case RATP_DEL_CONN:
			cb(ctx, PCB_RPE_CONN_DEL, n->arg1.net_name, NULL, n->id);
			break;
		case RATP_CHANGE_ATTRIB:
			cb(ctx, PCB_RPE_ATTR_CHG, n->id, n->arg1.attrib_name, n->arg2.attrib_val);
			break;
		}
	}
	return 0;
}

typedef struct {
	FILE *f;
	const char *q, *po, *pc, *line_prefix;
} fexport_t;

static void fexport_cb(void *ctx_, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val)
{
	fexport_t *ctx = ctx_;
	switch(ev) {
		case PCB_RPE_INFO_BEGIN:     fprintf(ctx->f, "%snet_info%s%s%s%s", ctx->line_prefix, ctx->po, ctx->q, netn, ctx->q); break;
		case PCB_RPE_INFO_TERMINAL:  fprintf(ctx->f, " %s%s%s", ctx->q, val, ctx->q); break;
		case PCB_RPE_INFO_END:       fprintf(ctx->f, "%s\n", ctx->pc); break;
		case PCB_RPE_CONN_ADD:       fprintf(ctx->f, "%sadd_conn%s%s%s%s %s%s%s%s\n", ctx->line_prefix, ctx->po, ctx->q, val, ctx->q, ctx->q, netn, ctx->q, ctx->pc); break;
		case PCB_RPE_CONN_DEL:       fprintf(ctx->f, "%sdel_conn%s%s%s%s %s%s%s%s\n", ctx->line_prefix, ctx->po, ctx->q, val, ctx->q, ctx->q, netn, ctx->q, ctx->pc); break;
		case PCB_RPE_ATTR_CHG:
			fprintf(ctx->f, "%schange_attrib%s%s%s%s %s%s%s %s%s%s%s\n",
				ctx->line_prefix, ctx->po,
				ctx->q, netn, ctx->q,
				ctx->q, key, ctx->q,
				ctx->q, val, ctx->q,
				ctx->pc);
	}
}

int pcb_ratspatch_fexport(pcb_board_t *pcb, FILE *f, int fmt_pcb)
{
	fexport_t ctx;
	if (fmt_pcb) {
		ctx.q = "\"";
		ctx.po = "(";
		ctx.pc = ")";
		ctx.line_prefix = "\t";
	}
	else {
		ctx.q = "";
		ctx.po = " ";
		ctx.pc = "";
		ctx.line_prefix = "";
	}
	ctx.f = f;
	return pcb_rats_patch_export(pcb, pcb->NetlistPatches, !fmt_pcb, fexport_cb, &ctx);
}

static const char pcb_acts_ReplaceFootprint[] = "ReplaceFootprint([Selected|Object], [footprint], [dumb])\n";
static const char pcb_acth_ReplaceFootprint[] = "Replace the footprint of the selected components with the footprint specified.";
/* DOC: replacefootprint.html */

static int act_replace_footprint_dst(int op, pcb_subc_t **olds)
{
	int found = 0;

	switch(op) {
		case F_Selected:
			/* check if we have elements selected and quit if not */
			PCB_SUBC_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc) && (subc->refdes != NULL)) {
					found = 1;
					break;
				}
			}
			PCB_END_LOOP;

			if (!(found)) {
				rnd_message(RND_MSG_ERROR, "ReplaceFootprint(Selected) called with no selection\n");
				return 1;
			}
			break;
		case F_Object:
			{
				void *ptr1, *ptr2, *ptr3;
				pcb_objtype_t type = pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_SUBC, &ptr1, &ptr2, &ptr3);
				if ((type != PCB_OBJ_SUBC) || (ptr1 == NULL)) {
					rnd_message(RND_MSG_ERROR, "ReplaceFootprint(Object): no subc under cursor\n");
					return 1;
				}
				*olds = ptr1;
			}
			break;

		default:
			rnd_message(RND_MSG_ERROR, "ReplaceFootprint(): invalid first argument\n");
			return 1;
	}
	return 0;
}

static int act_replace_footprint_src(char *fpname, pcb_subc_t **news)
{
	int len, bidx;
	const char *what, *what2;

	if (fpname == NULL) {
		fpname = rnd_hid_prompt_for(&PCB->hidlib, "Footprint name to use for replacement:", "", "Footprint");
		if (fpname == NULL) {
			rnd_message(RND_MSG_ERROR, "No footprint name supplied\n");
			return 1;
		}
	}
	else
		fpname = rnd_strdup(fpname);

	if (strcmp(fpname, "@buffer") != 0) {
		/* check if the footprint is available */
		bidx = PCB_MAX_BUFFER-1;
		pcb_buffer_load_footprint(&pcb_buffers[bidx], fpname, NULL);
		what = "Footprint file ";
		what2 = fpname;
	}
	else {
		what = "Buffer";
		what2 = "";
		bidx = conf_core.editor.buffer_number;
	}

	len = pcb_subclist_length(&pcb_buffers[bidx].Data->subc);
	if (len == 0) {
		rnd_message(RND_MSG_ERROR, "%s%s contains no subcircuits\n", what, what2);
		return 1;
	}

	if (len > 1) {
		rnd_message(RND_MSG_ERROR, "%s%s contains multiple subcircuits\n", what, what2);
		return 1;
	}
	*news = pcb_subclist_first(&pcb_buffers[bidx].Data->subc);
	return 0;
}


static int act_replace_footprint(int op, pcb_subc_t *olds, pcb_subc_t *news, char *fpname, int dumb)
{
	int changed = 0;
	pcb_subc_t *placed;

	pcb_undo_save_serial();
	switch(op) {
		case F_Selected:
			PCB_SUBC_LOOP(PCB->Data);
			{

				if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc) || (subc->refdes == NULL))
					continue;
				placed = pcb_subc_replace(PCB, subc, news, rnd_true, dumb);
				if (placed != NULL) {
					if (!dumb)
						pcb_ratspatch_append_optimize(PCB, RATP_CHANGE_ATTRIB, placed->refdes, "footprint", fpname);
					changed = 1;
				}
			}
			PCB_END_LOOP;
			break;
		case F_Object:
			placed = pcb_subc_replace(PCB, olds, news, rnd_true, dumb);
			if (placed != NULL) {
				if (!dumb)
					pcb_ratspatch_append_optimize(PCB, RATP_CHANGE_ATTRIB, placed->refdes, "footprint", fpname);
				changed = 1;
			}
			break;
	}


	pcb_undo_restore_serial();
	if (changed) {
		pcb_undo_inc_serial();
		rnd_gui->invalidate_all(rnd_gui);
	}
	return 0;
}



static fgw_error_t pcb_act_ReplaceFootprint(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *fpname = NULL, *dumbs = NULL;
	pcb_subc_t *olds = NULL, *news;
	int dumb = 0, op = F_Selected;

	RND_ACT_MAY_CONVARG(1, FGW_KEYWORD, ReplaceFootprint, op = fgw_keyword(&argv[1]));

	if (act_replace_footprint_dst(op, &olds) != 0) {
		RND_ACT_IRES(1);
		return 0;
	}

	/* fetch the name of the new footprint */
	RND_ACT_MAY_CONVARG(2, FGW_STR, ReplaceFootprint, fpname = rnd_strdup(argv[2].val.str));
	if (act_replace_footprint_src(fpname, &news) != 0) {
		RND_ACT_IRES(1);
		return 0;
	}

	RND_ACT_MAY_CONVARG(3, FGW_STR, ReplaceFootprint, dumbs = argv[3].val.str);
	if ((dumbs != NULL) && (strcmp(dumbs, "dumb") == 0))
		dumb = 1;


	RND_ACT_IRES(act_replace_footprint(op, olds, news, fpname, dumb));
	free(fpname);
	return 0;
}

static const char pcb_acts_SavePatch[] = "SavePatch(filename)";
static const char pcb_acth_SavePatch[] = "Save netlist patch for back annotation.";
static fgw_error_t pcb_act_SavePatch(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fn = NULL;
	FILE *f;

	RND_ACT_MAY_CONVARG(1, FGW_STR, SavePatch, fn = argv[1].val.str);

	if (fn == NULL) {
		char *default_file;

		if (PCB->hidlib.filename != NULL) {
			char *end;
			int len;
			len = strlen(PCB->hidlib.filename);
			default_file = malloc(len + 8);
			memcpy(default_file, PCB->hidlib.filename, len + 1);
			end = strrchr(default_file, '.');
			if ((end == NULL) || (rnd_strcasecmp(end, ".pcb") != 0))
				end = default_file + len;
			strcpy(end, ".bap");
		}
		else
			default_file = rnd_strdup("unnamed.bap");

		fn = rnd_gui->fileselect(rnd_gui, "Save netlist patch as ...",
			"Choose a file to save netlist patch to\n"
			"for back annotation\n", default_file, ".bap", NULL, "patch", 0, NULL);

		free(default_file);
	}

	f = rnd_fopen(&PCB->hidlib, fn, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't open netlist patch file %s for writing\n", fn);
		RND_ACT_IRES(-1);
		return 0;
	}
	pcb_ratspatch_fexport(PCB, f, 0);
	fclose(f);

	RND_ACT_IRES(0);
	return 0;
}

static rnd_action_t rats_patch_action_list[] = {
	{"ReplaceFootprint", pcb_act_ReplaceFootprint, pcb_acth_ReplaceFootprint, pcb_acts_ReplaceFootprint},
	{"SavePatch", pcb_act_SavePatch, pcb_acth_SavePatch, pcb_acts_SavePatch}
};

void pcb_rats_patch_init2(void)
{
	RND_REGISTER_ACTIONS(rats_patch_action_list, NULL);
}


