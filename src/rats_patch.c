/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015..2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
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
#include <librnd/hid/hid.h>
#include <librnd/hid/hid_dad.h>
#include "funchash_core.h"
#include "search.h"
#include "undo.h"
#include "conf_core.h"
#include "netlist.h"
#include "event.h"
#include "obj_subc_parent.h"
#include "data_it.h"

static const char core_ratspatch_cookie[] = "core-rats-patch";

const char *pcb_netlist_names[PCB_NUM_NETLISTS] = {
	"input",
	"edited"
};

static int is_subc_on_netlist(pcb_netlist_t *netlist, const char *refdes)
{
	htsp_entry_t *e;
	for(e = htsp_first(netlist); e != NULL; e = htsp_next(netlist, e)) {
		pcb_net_t *net = e->value;
		pcb_net_term_t *term;
		for(term = pcb_termlist_first(&net->conns); term != NULL; term = pcb_termlist_next(term)) {
			if (strcmp(term->refdes, refdes) == 0)
				return 1;
		}
	}
	return 0;
}

static int is_subc_created_on_patch(pcb_ratspatch_line_t *head, const char *refdes)
{
	pcb_ratspatch_line_t *rp;
	int created = 0;

	for(rp = head; rp != NULL; rp = rp->next) {
		if ((rp->op == RATP_COMP_ADD) && (strcmp(rp->id, refdes) == 0))
			created = 1;
		else if ((rp->op == RATP_COMP_DEL) && (strcmp(rp->id, refdes) == 0))
			created = 0;
	}

	return created;
}


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

	/* do not append invalid entries (with no ID) */
	if (id == NULL)
		return;

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
	case RATP_CHANGE_COMP_ATTRIB:
		seek_op = RATP_CHANGE_COMP_ATTRIB;
		break;
	case RATP_CHANGE_NET_ATTRIB:
		seek_op = RATP_CHANGE_NET_ATTRIB;
		break;
	case RATP_COMP_ADD:
		seek_op = RATP_COMP_DEL;
		break;
	case RATP_COMP_DEL:
		seek_op = RATP_COMP_ADD;
		break;
	}

	/* keep the list clean: remove the last operation that becomes obsolete by the new one */
	for (n = pcb->NetlistPatchLast; n != NULL; n = n->prev) {
		if ((n->op == seek_op) && (strcmp(n->id, id) == 0)) {
			switch (op) {
				case RATP_CHANGE_COMP_ATTRIB:
				case RATP_CHANGE_NET_ATTRIB:
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
				case RATP_COMP_ADD:
				case RATP_COMP_DEL:
					if (strcmp(n->id, id) != 0)
						break;
					rats_patch_remove(pcb, n, 1);
					goto quit;
			}
		}
	}

quit:;
	pcb_ratspatch_append(pcb, op, id, a1, a2, 0);
}

void rats_patch_remove(pcb_board_t *pcb, pcb_ratspatch_line_t *n, int do_free)
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

static int rats_patch_apply_comp_attrib(pcb_board_t *pcb, pcb_ratspatch_line_t *patch)
{
		pcb_subc_t *subc = pcb_subc_by_refdes(pcb->Data, patch->id);
	if (subc == NULL)
		return 1;
	if ((patch->arg2.attrib_val == NULL) || (*patch->arg2.attrib_val == '\0'))
		pcb_attribute_remove(&subc->Attributes, patch->arg1.attrib_name);
	else
		pcb_attribute_put(&subc->Attributes, patch->arg1.attrib_name, patch->arg2.attrib_val);
	return 0;
}

static int rats_patch_apply_net_attrib(pcb_board_t *pcb, pcb_ratspatch_line_t *patch)
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

static int rats_patch_apply_comp_del(pcb_board_t *pcb, pcb_ratspatch_line_t *patch)
{
	TODO("warning if refs are not removed");
	return 0;
}

int pcb_ratspatch_apply(pcb_board_t *pcb, pcb_ratspatch_line_t *patch)
{
	switch (patch->op) {
		case RATP_ADD_CONN:
			return rats_patch_apply_conn(pcb, patch, 0);
		case RATP_DEL_CONN:
			return rats_patch_apply_conn(pcb, patch, 1);
		case RATP_CHANGE_COMP_ATTRIB:
			return rats_patch_apply_comp_attrib(pcb, patch);
		case RATP_CHANGE_NET_ATTRIB:
			return rats_patch_apply_net_attrib(pcb, patch);
		case RATP_COMP_ADD:
			/* this doesn't cause any direct netlist change */
			return 0;
		case RATP_COMP_DEL:
			return rats_patch_apply_comp_del(pcb, patch);
	}
	return 0;
}

void pcb_ratspatch_make_edited(pcb_board_t *pcb)
{
	pcb_ratspatch_line_t *n;

	pcb_netlist_uninit(&(pcb->netlist[PCB_NETLIST_EDITED]));
	pcb_netlist_init(&(pcb->netlist[PCB_NETLIST_EDITED]));
	pcb_netlist_copy(pcb, &(pcb->netlist[PCB_NETLIST_EDITED]), &(pcb->netlist[PCB_NETLIST_INPUT]));

	for(n = pcb->NetlistPatches; n != NULL; n = n->next)
		pcb_ratspatch_apply(pcb, n);
}

/* Returns non-zero if n is already done on pcb; useful after a forward annotation */
int pcb_rats_patch_already_done(pcb_board_t *pcb, pcb_ratspatch_line_t *n)
{
	switch(n->op) {
		case RATP_ADD_CONN:
			if (pcb_net_is_term_on_net_by_name(pcb, &pcb->netlist[PCB_NETLIST_INPUT], n->arg1.net_name, n->id) == 1) {
				rnd_message(RND_MSG_INFO, "rats patch: 'add connection' %s to %s is done on forward annotation, removing.\n", n->id, n->arg1.net_name);
				return 1;
			}
			break;
		case RATP_DEL_CONN:
			if (pcb_net_is_term_on_net_by_name(pcb, &pcb->netlist[PCB_NETLIST_INPUT], n->arg1.net_name, n->id) < 1) {
				rnd_message(RND_MSG_INFO, "rats patch: 'del connection' %s to %s is done on forward annotation, removing.\n", n->id, n->arg1.net_name);
				return 1;
			}
			break;
		case RATP_CHANGE_COMP_ATTRIB:
			{
				pcb_subc_t *subc = pcb_subc_by_refdes(pcb->Data, n->id);
				if (subc != NULL) {
					char *want = n->arg2.attrib_val;
					char *av = pcb_attribute_get(&subc->Attributes, n->arg1.attrib_name);
					if (((av == NULL) || (*av == '\0')) && ((want == NULL) || (*want == '\0'))) {
						rnd_message(RND_MSG_INFO, "rats patch: comp attrib %s del from %s is done on forward annotation, removing.\n", n->arg1.attrib_name, n->id);
						return 1;
					}
					if ((av != NULL) && (want != NULL) && (strcmp(av, want) == 0)) {
						rnd_message(RND_MSG_INFO, "rats patch: comp attrib %s change for %s is done on forward annotation, removing.\n", n->arg1.attrib_name, n->id);
						return 1;
					}
				}
			}
			break;
		case RATP_CHANGE_NET_ATTRIB:
			{
				pcb_net_t *net = pcb_net_get(pcb, &pcb->netlist[PCB_NETLIST_EDITED], n->id, 0);
				if (net != NULL) {
					char *want = n->arg2.attrib_val;
					char *av = pcb_attribute_get(&net->Attributes, n->arg1.attrib_name);
					if (((av == NULL) || (*av == '\0')) && ((want == NULL) || (*want == '\0'))) {
						rnd_message(RND_MSG_INFO, "rats patch: net attrib %s del from %s is done on forward annotation, removing.\n", n->arg1.attrib_name, n->id);
						return 1;
					}
					if ((av != NULL) && (want != NULL) && (strcmp(av, want) == 0)) {
						rnd_message(RND_MSG_INFO, "rats patch: net attrib %s change for %s is done on forward annotation, removing.\n", n->arg1.attrib_name, n->id);
						return 1;
					}
				}
			}
			break;

		case RATP_COMP_ADD:
			if (is_subc_on_netlist(&pcb->netlist[PCB_NETLIST_INPUT], n->id)) {
				rnd_message(RND_MSG_INFO, "rats patch: subcircuit %s has already been added, removing patch.\n", n->id);
				return 1;
			}
			break;
		case RATP_COMP_DEL:
			if (!is_subc_on_netlist(&pcb->netlist[PCB_NETLIST_INPUT], n->id)) {
				rnd_message(RND_MSG_INFO, "rats patch: subcircuit %s has already been deleted, removing patch.\n", n->id);
				return 1;
			}
			break;
	}
	return 0;
}

int pcb_rats_patch_cleanup_patches(pcb_board_t *pcb)
{
	pcb_ratspatch_line_t *n, *next;
	int cnt = 0;

	for(n = pcb->NetlistPatches; n != NULL; n = next) {
		next = n->next;
		if (pcb_rats_patch_already_done(pcb, n)) {
			rats_patch_remove(pcb, n, 1);
			cnt++;
		}
	}

	return cnt;
}

/**** high level connections ****/
long pcb_ratspatch_addconn_term(pcb_board_t *pcb, pcb_net_t *net, pcb_any_obj_t *obj)
{
	pcb_subc_t *subc = pcb_gobj_parent_subc(obj->parent_type, &obj->parent);
	pcb_net_term_t *term;
	char *termname;

	if (subc == NULL)
		return 0;

	termname = rnd_strdup_printf("%s-%s", subc->refdes, obj->term);
	term = pcb_net_find_by_obj(&pcb->netlist[PCB_NETLIST_EDITED], obj);
	if (term != NULL) {
		rnd_message(RND_MSG_ERROR, "Can not add %s to net %s because terminal is already part of a net\n", termname, net->name);
		free(termname);
		return 0;
	}

	pcb_ratspatch_append_optimize(pcb, RATP_ADD_CONN, termname, net->name, NULL);
	free(termname);
	pcb_ratspatch_make_edited(pcb);
	pcb_netlist_changed(0);
	return 1;
}

long pcb_ratspatch_addconn_selected(pcb_board_t *pcb, pcb_data_t *parent, pcb_net_t *net)
{
	long cnt = 0;
	pcb_any_obj_t *obj;
	pcb_data_it_t it;

	for(obj = pcb_data_first(&it, parent, PCB_OBJ_CLASS_REAL); obj != NULL; obj = pcb_data_next(&it)) {
		if ((obj->term != NULL) && PCB_FLAG_TEST(PCB_FLAG_SELECTED, obj)) /* pick up terminals */
			cnt += pcb_ratspatch_addconn_term(pcb, net, obj);

		if (obj->type == PCB_OBJ_SUBC) { /* recurse to subc */
			pcb_subc_t *subc = (pcb_subc_t *)obj;
			cnt += pcb_ratspatch_addconn_selected(pcb, subc->data, net);
		}
	}

	return cnt;
}

long pcb_ratspatch_delconn_term(pcb_board_t *pcb, pcb_any_obj_t *obj)
{
	pcb_subc_t *subc = pcb_gobj_parent_subc(obj->parent_type, &obj->parent);
	pcb_net_term_t *term;
	pcb_net_t *net;
	char *termname;

	if (subc == NULL)
		return 0;

	termname = rnd_strdup_printf("%s-%s", subc->refdes, obj->term);
	term = pcb_net_find_by_obj(&pcb->netlist[PCB_NETLIST_EDITED], obj);
	if (term == NULL)
		return 0;

	net = term->parent.net;
	if (net == NULL)
		return 0;

	assert(term->parent_type == PCB_PARENT_NET);

	pcb_ratspatch_append_optimize(pcb, RATP_DEL_CONN, termname, net->name, NULL);
	free(termname);
	pcb_ratspatch_make_edited(pcb);
	pcb_netlist_changed(0);
	return 1;

}

long pcb_ratspatch_delconn_selected(pcb_board_t *pcb, pcb_data_t *parent)
{
	long cnt = 0;
	pcb_any_obj_t *obj;
	pcb_data_it_t it;

	for(obj = pcb_data_first(&it, parent, PCB_OBJ_CLASS_REAL); obj != NULL; obj = pcb_data_next(&it)) {
		if ((obj->term != NULL) && PCB_FLAG_TEST(PCB_FLAG_SELECTED, obj)) /* pick up terminals */
			cnt += pcb_ratspatch_delconn_term(pcb, obj);

		if (obj->type == PCB_OBJ_SUBC) { /* recurse to subc */
			pcb_subc_t *subc = (pcb_subc_t *)obj;
			cnt += pcb_ratspatch_delconn_selected(pcb, subc->data);
		}
	}

	return cnt;
}

/**** high level subc ****/


int rats_patch_is_subc_referenced(pcb_board_t *pcb, const char *refdes)
{
	return is_subc_created_on_patch(pcb->NetlistPatches, refdes) || is_subc_on_netlist(&pcb->netlist[PCB_NETLIST_EDITED], refdes);
}


int rats_patch_add_subc(pcb_board_t *pcb, pcb_subc_t *subc, int undoable)
{
	if ((subc->refdes == NULL) || (*subc->refdes == '\0'))
		return -1;

	if (rats_patch_is_subc_referenced(pcb, subc->refdes))
		return 0; /* already on */

	pcb_ratspatch_append(pcb, RATP_COMP_ADD, subc->refdes, NULL, NULL, undoable);

	return 0;
}

int rats_patch_del_subc(pcb_board_t *pcb, pcb_subc_t *subc, int force, int undoable)
{
	if ((subc->refdes == NULL) || (*subc->refdes == '\0'))
		return -1;

	if (!force) {
		if (!rats_patch_is_subc_referenced(pcb, subc->refdes))
			return 0; /* already removed */
	}

	pcb_ratspatch_append(pcb, RATP_COMP_DEL, subc->refdes, NULL, NULL, undoable);

	return 0;
}

void rats_patch_break_subc_conns(pcb_board_t *pcb, pcb_subc_t *subc, int undoable)
{
	pcb_netlist_t *netlist = &pcb->netlist[PCB_NETLIST_EDITED];
	htsp_entry_t *e;

	for(e = htsp_first(netlist); e != NULL; e = htsp_next(netlist, e)) {
		pcb_net_t *net = e->value;
		pcb_net_term_t *term, *next;
		for(term = pcb_termlist_first(&net->conns); term != NULL; term = next) {
			next = pcb_termlist_next(term);
			if (strcmp(term->refdes, subc->refdes) == 0) {
				char *pinname = rnd_strdup_printf("%s-%s", term->refdes, term->term);
				pcb_ratspatch_append_optimize(pcb, RATP_DEL_CONN, pinname, net->name, NULL);
				pcb_net_term_del(net, term);
				free(pinname);
			}
		}
	}
}



/**** export ****/

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
			case RATP_CHANGE_COMP_ATTRIB:
			case RATP_CHANGE_NET_ATTRIB:
			case RATP_COMP_DEL:
			case RATP_COMP_ADD:
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
		case RATP_CHANGE_COMP_ATTRIB:
			cb(ctx, PCB_RPE_COMP_ATTR_CHG, n->id, n->arg1.attrib_name, n->arg2.attrib_val);
			break;
		case RATP_CHANGE_NET_ATTRIB:
			cb(ctx, PCB_RPE_NET_ATTR_CHG, n->id, n->arg1.attrib_name, n->arg2.attrib_val);
			break;
		case RATP_COMP_DEL:
			cb(ctx, PCB_RPE_COMP_DEL, n->id, NULL, NULL);
			break;
		case RATP_COMP_ADD:
			cb(ctx, PCB_RPE_COMP_ADD, n->id, NULL, NULL);
			break;
		}
	}
	return 0;
}

typedef struct {
	FILE *f;
	const char *q, *po, *pc, *line_prefix;
} fexport_old_bap_t;

static void fexport_old_bap_cb(void *ctx_, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val)
{
	fexport_old_bap_t *ctx = ctx_;
	switch(ev) {
		case PCB_RPE_INFO_BEGIN:     fprintf(ctx->f, "%snet_info%s%s%s%s", ctx->line_prefix, ctx->po, ctx->q, netn, ctx->q); break;
		case PCB_RPE_INFO_TERMINAL:  fprintf(ctx->f, " %s%s%s", ctx->q, val, ctx->q); break;
		case PCB_RPE_INFO_END:       fprintf(ctx->f, "%s\n", ctx->pc); break;
		case PCB_RPE_CONN_ADD:       fprintf(ctx->f, "%sadd_conn%s%s%s%s %s%s%s%s\n", ctx->line_prefix, ctx->po, ctx->q, val, ctx->q, ctx->q, netn, ctx->q, ctx->pc); break;
		case PCB_RPE_CONN_DEL:       fprintf(ctx->f, "%sdel_conn%s%s%s%s %s%s%s%s\n", ctx->line_prefix, ctx->po, ctx->q, val, ctx->q, ctx->q, netn, ctx->q, ctx->pc); break;
		case PCB_RPE_COMP_ATTR_CHG:
			fprintf(ctx->f, "%schange_attrib%s%s%s%s %s%s%s %s%s%s%s\n",
				ctx->line_prefix, ctx->po,
				ctx->q, netn, ctx->q,
				ctx->q, key, ctx->q,
				ctx->q, val, ctx->q,
				ctx->pc);
		case PCB_RPE_NET_ATTR_CHG:
			rnd_message(RND_MSG_ERROR, "Can not save net attrib change in old or bap netlist patch format for back annotation\n");
			break;

		case PCB_RPE_COMP_ADD:
			rnd_message(RND_MSG_ERROR, "Can not save comp_add (subcircuit added) in old or bap netlist patch format for back annotation\n");
			break;

		case PCB_RPE_COMP_DEL:
			rnd_message(RND_MSG_ERROR, "Can not save comp_del (subcircuit removed) in old or bap netlist patch format for back annotation\n");
			break;
	}
}

static int fputs_tdx_(FILE *f, const char *s, int *len, int replace_term_sep)
{
	char *sep = NULL;
	if (replace_term_sep) {
		/* need to replace the last '-' so that subc-in-subc hierarchy like
		   U1-R3-2 results in U1-R3 2; but normally the hierarchy separator
		   can not be - */
		sep = strrchr(s, '-');
	}

	for(; *s != '\0'; s++) {
		int c;

		if (s != sep) {
			c = *s;
			if ((*s == ' ') || (*s == '\t')) { /* escape spaces */
				if (*len < 511)
					fputc('\\', f);
				(*len)++;
			}
			else if ((c < 32) || (c > 126)) {
				rnd_message(RND_MSG_ERROR, "tEDAx output: replaced invalid character with undescrore\n");
				c = '_';
			}
		}
		else
			c = ' '; /* replace last dash with space for terminals */

		if (*len >= 511) {
			rnd_message(RND_MSG_ERROR, "tEDAx output: line too long (truncated)\n");
			return -1;
		}
		fputc(c, f);
		(*len)++;
	}
	return 0;
}

static int fputs_tdx(FILE *f, const char *s, int *len)
{
	return fputs_tdx_(f, s, len, 0);
}

static int fputs_tdx_term(FILE *f, const char *s, int *len)
{
	return fputs_tdx_(f, s, len, 1);
}


typedef struct {
	FILE *f;
	int ver;

	/* temp */
	char *netn;
	unsigned int in_block:1;
} fexport_tedax_t;

static void fexport_tedax_start_block(fexport_tedax_t *ctx, const char *name)
{
	int len;

	assert(!ctx->in_block);

	if (name == NULL)
		name = "not named";

	len = fprintf(ctx->f, "begin backann v%d ", ctx->ver);
	fputs_tdx(ctx->f, name, &len);
	fprintf(ctx->f, "\n");
	ctx->in_block = 1;
}

static void fexport_tedax_end_block(fexport_tedax_t *ctx)
{
	assert(ctx->in_block);
	fprintf(ctx->f, "end backann\n");
	ctx->in_block = 0;
}


static void fexport_tedax_cb(void *ctx_, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val)
{
	int len = 0;
	fexport_tedax_t *ctx = ctx_;


	switch(ev) {
		case PCB_RPE_INFO_BEGIN:
			ctx->netn = rnd_strdup(netn);
			if (!ctx->in_block) fexport_tedax_start_block(ctx, NULL);
			break;
		case PCB_RPE_INFO_TERMINAL:
			len = fprintf(ctx->f, "	net_info ");
			fputs_tdx(ctx->f, ctx->netn, &len);
			len += fprintf(ctx->f, " ");
			fputs_tdx_term(ctx->f, val, &len);
			fprintf(ctx->f, "\n");
			break;
		case PCB_RPE_INFO_END:
			free(ctx->netn);
			break;
		case PCB_RPE_CONN_ADD:
			if (!ctx->in_block) fexport_tedax_start_block(ctx, NULL);
			len = fprintf(ctx->f, "	add_conn ");
			fputs_tdx(ctx->f, netn, &len);
			len += fprintf(ctx->f, " ");
			fputs_tdx_term(ctx->f, val, &len);
			fprintf(ctx->f, "\n");
			break;
		case PCB_RPE_CONN_DEL:
			if (!ctx->in_block) fexport_tedax_start_block(ctx, NULL);
			len = fprintf(ctx->f, "	del_conn ");
			fputs_tdx(ctx->f, netn, &len);
			len += fprintf(ctx->f, " ");
			fputs_tdx_term(ctx->f, val, &len);
			fprintf(ctx->f, "\n");
			break;
		case PCB_RPE_COMP_ATTR_CHG:
			if (!ctx->in_block) fexport_tedax_start_block(ctx, NULL);
			if ((strcmp(key, "footprint") == 0) || (strcmp(key, "value") == 0))
				len = fprintf(ctx->f, "	fattr_comp ");
			else
				len = fprintf(ctx->f, "	attr_comp ");
			goto save_attr_val;
		case PCB_RPE_NET_ATTR_CHG:
			if (ctx->ver < 2) {
				rnd_message(RND_MSG_ERROR, "Can not save net attrib change in tedax backann v1 format; please use v2 for this export\n");
				break;
			}
			if (!ctx->in_block) fexport_tedax_start_block(ctx, NULL);
			len = fprintf(ctx->f, "	attr_conn ");
			save_attr_val:;
			fputs_tdx(ctx->f, netn, &len);
			len += fprintf(ctx->f, " ");
			fputs_tdx(ctx->f, key, &len);
			len += fprintf(ctx->f, " ");
			fputs_tdx(ctx->f, val, &len);
			fprintf(ctx->f, "\n");
			break;

		case PCB_RPE_COMP_ADD:
			if (ctx->ver < 2) {
				rnd_message(RND_MSG_ERROR, "Can not save comp_add in tedax backann v1 format; please use v2 for this export\n");
				break;
			}
			if (!ctx->in_block) fexport_tedax_start_block(ctx, NULL);
			len = fprintf(ctx->f, "	add_comp ");
			fputs_tdx(ctx->f, netn, &len);
			fprintf(ctx->f, "\n");
			break;

		case PCB_RPE_COMP_DEL:
			if (ctx->ver < 2) {
				rnd_message(RND_MSG_ERROR, "Can not save comp_del in tedax backann v1 format; please use v2 for this export\n");
				break;
			}
			if (!ctx->in_block) fexport_tedax_start_block(ctx, NULL);
			len = fprintf(ctx->f, "	del_comp ");
			fputs_tdx(ctx->f, netn, &len);
			fprintf(ctx->f, "\n");
			break;
	}
}

int pcb_ratspatch_fexport(pcb_board_t *pcb, FILE *f, pcb_ratspatch_fmt_t fmt)
{
	int res;
	fexport_old_bap_t ctx_old;
	fexport_tedax_t ctx_tdx = {0};

	ctx_old.f = ctx_tdx.f = f;

	switch(fmt) {
		case PCB_RPFM_PCB:
			ctx_old.q = "\"";
			ctx_old.po = "(";
			ctx_old.pc = ")";
			ctx_old.line_prefix = "\t";
			return pcb_rats_patch_export(pcb, pcb->NetlistPatches, 0, fexport_old_bap_cb, &ctx_old);
		case PCB_RPFM_BAP:
			ctx_old.q = "";
			ctx_old.po = " ";
			ctx_old.pc = "";
			ctx_old.line_prefix = "";
			return pcb_rats_patch_export(pcb, pcb->NetlistPatches, 1, fexport_old_bap_cb, &ctx_old);
		case PCB_RPFM_BACKANN_V1:
			ctx_tdx.ver = 1;
			save_tedax:;
			fprintf(f, "tEDAx v1\n");
			res = pcb_rats_patch_export(pcb, pcb->NetlistPatches, 1, fexport_tedax_cb, &ctx_tdx);
			if (!ctx_tdx.in_block) {
				/* happens when the list is empty - export an empty block just for the headers */
				fexport_tedax_start_block(&ctx_tdx, "empty netlist patch");
			}
			fexport_tedax_end_block(&ctx_tdx);
			return res;
		case PCB_RPFM_BACKANN_V2:
			ctx_tdx.ver = 2;
			goto save_tedax;
		default:
			rnd_message(RND_MSG_ERROR, "Unknown file format\n");
			return -1;
	}

	return -1;
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

static int act_replace_footprint_src(char **fpname, pcb_subc_t **news)
{
	int len, bidx;
	const char *what, *what2;

	if (*fpname == NULL) {
		*fpname = rnd_hid_prompt_for(&PCB->hidlib, "Footprint name to use for replacement:", "", "Footprint");
		if (*fpname == NULL) {
			rnd_message(RND_MSG_ERROR, "No footprint name supplied\n");
			return 1;
		}
	}
	else
		*fpname = rnd_strdup(*fpname);

	if (strcmp(*fpname, "@buffer") != 0) {
		/* check if the footprint is available */
		bidx = PCB_MAX_BUFFER-1;
		pcb_buffer_load_footprint(&pcb_buffers[bidx], *fpname, NULL);
		what = "Footprint file ";
		what2 = *fpname;
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
						pcb_ratspatch_append_optimize(PCB, RATP_CHANGE_COMP_ATTRIB, placed->refdes, "footprint", fpname);
					changed = 1;
				}
			}
			PCB_END_LOOP;
			break;
		case F_Object:
			placed = pcb_subc_replace(PCB, olds, news, rnd_true, dumb);
			if (placed != NULL) {
				if (!dumb && (placed->refdes != NULL))
					pcb_ratspatch_append_optimize(PCB, RATP_CHANGE_COMP_ATTRIB, placed->refdes, "footprint", fpname);
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
	if (act_replace_footprint_src(&fpname, &news) != 0) {
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

static const char pcb_acts_SavePatch[] = "SavePatch([fmt], filename)";
static const char pcb_acth_SavePatch[] = "Save netlist patch for back annotation. File format can be specified in fmt, which should be one of: @bap (for the old gschem/sch-rnd format), @backannv1 or @backannv2 (for tEDAx), @pcb (for the old gEDA/PCB format, read by pcb-rnd only)";
static fgw_error_t pcb_act_SavePatch(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fn = NULL;
	const char *sfmt = "bap";
	pcb_ratspatch_fmt_t fmt = -1;
	FILE *f;

	RND_ACT_MAY_CONVARG(1, FGW_STR, SavePatch, fn = argv[1].val.str);
	if ((fn != NULL) && (*fn == '@')) {
		sfmt = fn+1;
		RND_ACT_MAY_CONVARG(2, FGW_STR, SavePatch, fn = argv[2].val.str);
	}

	if (fn == NULL) {
		char *default_file;
		static const char *fmts[] = {"bap", "backannv2", "backannv1", "pcb", NULL};
		rnd_hid_dad_subdialog_t fmtsub_local = {0}, *fmtsub = &fmtsub_local;
		int wfmt, fmt_id;

		if (PCB->hidlib.loadname != NULL) {
			char *end;
			int len;
			len = strlen(PCB->hidlib.loadname);
			default_file = malloc(len + 8);
			memcpy(default_file, PCB->hidlib.loadname, len + 1);
			end = strrchr(default_file, '.');
			if ((end == NULL) || ((rnd_strcasecmp(end, ".rp") != 0) && (rnd_strcasecmp(end, ".pcb") != 0)))
				end = default_file + len;
			strcpy(end, ".bap");
		}
		else
			default_file = rnd_strdup("unnamed.bap");


		RND_DAD_BEGIN_HBOX(fmtsub->dlg);
			RND_DAD_LABEL(fmtsub->dlg, "File format:");
			RND_DAD_ENUM(fmtsub->dlg, fmts);
				wfmt = RND_DAD_CURRENT(fmtsub->dlg);
		RND_DAD_END(fmtsub->dlg);

		fn = rnd_hid_fileselect(rnd_gui, "Save netlist patch as ...",
			"Choose a file to save netlist patch to\n"
			"for back annotation\n", default_file, ".bap", NULL, "patch", 0, fmtsub);

		fmt_id = fmtsub->dlg[wfmt].val.lng;
		if ((fmt_id >= 0) && (fmt_id < sizeof(fmts)/sizeof(fmts[0])))
			sfmt = fmts[fmt_id];

		free(default_file);
	}

	f = rnd_fopen(&PCB->hidlib, fn, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't open netlist patch file %s for writing\n", fn);
		RND_ACT_IRES(-1);
		return 0;
	}

	if (strcmp(sfmt, "backannv1") == 0) fmt = PCB_RPFM_BACKANN_V1;
	else if (strcmp(sfmt, "backannv2") == 0) fmt = PCB_RPFM_BACKANN_V2;
	else if (strcmp(sfmt, "pcb") == 0) fmt = PCB_RPFM_PCB;
	else if (strcmp(sfmt, "bap") == 0) fmt = PCB_RPFM_BAP;
	else {
		rnd_message(RND_MSG_ERROR, "Unknown back annotation file format '%s'\n", sfmt);
		RND_ACT_IRES(-1);
		return 0;
	}

	pcb_ratspatch_fexport(PCB, f, fmt);
	fclose(f);

	RND_ACT_IRES(0);
	return 0;
}

static rnd_action_t rats_patch_action_list[] = {
	{"ReplaceFootprint", pcb_act_ReplaceFootprint, pcb_acth_ReplaceFootprint, pcb_acts_ReplaceFootprint},
	{"SavePatch", pcb_act_SavePatch, pcb_acth_SavePatch, pcb_acts_SavePatch}
};

static void rats_patch_netlist_import(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_rats_patch_cleanup_patches((pcb_board_t *)hidlib);
}


void pcb_rats_patch_init2(void)
{
	RND_REGISTER_ACTIONS(rats_patch_action_list, NULL);
	rnd_event_bind(PCB_EVENT_NETLIST_IMPORTED, rats_patch_netlist_import, NULL, core_ratspatch_cookie);
}

void pcb_rats_patch_uninit(void)
{
	rnd_event_unbind_allcookie(core_ratspatch_cookie);
}
