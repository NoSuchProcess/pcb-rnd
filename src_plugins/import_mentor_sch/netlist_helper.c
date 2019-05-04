/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  netlist build helper
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <genht/hash.h>

#include "netlist_helper.h"
#include "compat_misc.h"
#include "pcb-printf.h"
#include "error.h"
#include "actions.h"
#include "safe_fs.h"


nethlp_ctx_t *nethlp_new(nethlp_ctx_t *prealloc)
{
	if (prealloc == NULL) {
		prealloc = malloc(sizeof(nethlp_ctx_t));
		prealloc->alloced = 1;
	}
	else
		prealloc->alloced = 0;

	prealloc->part_rules = NULL;
	htsp_init(&prealloc->id2refdes, strhash, strkeyeq);
	return prealloc;
}

void nethlp_destroy(nethlp_ctx_t *nhctx)
{
	htsp_entry_t *e;
	nethlp_rule_t *r, *next;

	for(r = nhctx->part_rules; r != NULL; r = next) {
		next = r->next;
		re_se_free(r->key);
		re_se_free(r->val);
		free(r->new_key);
		free(r->new_val);
		free(r);
	}

	for (e = htsp_first(&nhctx->id2refdes); e; e = htsp_next(&nhctx->id2refdes, e)) {
		free(e->key);
		free(e->value);
	}
	htsp_uninit(&nhctx->id2refdes);
	if (nhctx->alloced)
		free(nhctx);
}

#define ltrim(str) while(isspace(*str)) str++;
#define rtrim(str) \
do { \
	char *__s__; \
	for(__s__ = str + strlen(str) - 1; __s__ >= str; __s__--) { \
		if ((*__s__ != '\r') && (*__s__ != '\n')) \
			break; \
		*__s__ = '\0'; \
	} \
} while(0)

static int part_map_split(char *s, char *argv[], int maxargs)
{
	int argc;
	int n;

	ltrim(s);
	if ((*s == '#') || (*s == '\0'))
		return 0;
	rtrim(s);

	for(argc = n = 0; n < maxargs; n++) {
		argv[argc] = s;
		if (s != NULL) {
			argc++;
			s = strchr(s, '|');
			if (s != NULL) {
				*s = '\0';
				s++;
			}
		}
	}
	return argc;
}

static int part_map_parse(nethlp_ctx_t *nhctx, int argc, char *argv[], const char *fn, int lineno)
{
	char *end;
	nethlp_rule_t *r;
	re_se_t *kr, *vr;
	int prio;

	if (argc != 5) {
		pcb_message(PCB_MSG_ERROR, "Loading part map: wrong number of fields %d in %s:%d - expected 5 - ignoring this rule\n", argc, fn, lineno);
		return -1;
	}
	if (*argv[0] != '*') {
		prio = strtol(argv[0], &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "Loading part map: invaid priority '%s' in %s:%d - ignoring this rule\n", argv[0], fn, lineno);
			return -1;
		}
	}
	else
		prio = nethlp_prio_always;
	kr = re_se_comp(argv[1]);
	if (kr == NULL) {
		pcb_message(PCB_MSG_ERROR, "Loading part map: can't compile attribute name regex in %s:%d - ignoring this rule\n", fn, lineno);
		return -1;
	}
	vr = re_se_comp(argv[2]);
	if (vr == NULL) {
		re_se_free(kr);
		pcb_message(PCB_MSG_ERROR, "Loading part map: can't compile attribute value regex in %s:%d - ignoring this rule\n", fn, lineno);
		return -1;
	}

	r = malloc(sizeof(nethlp_rule_t));
	r->prio = prio;
	r->key = kr;
	r->val = vr;
	r->new_key = pcb_strdup(argv[3]);
	r->new_val = pcb_strdup(argv[4]);
	r->next = nhctx->part_rules;
	nhctx->part_rules = r;

	return 0;
}

int nethlp_load_part_map(nethlp_ctx_t *nhctx, const char *fn)
{
	FILE *f;
	int cnt, argc, lineno;
	char line[1024], *argv[8];

	f = pcb_fopen(NULL, fn, "r");
	if (f == NULL)
		return -1;

	lineno = 0;
	while(fgets(line, sizeof(line), f) != NULL) {
		lineno++;
		argc = part_map_split(line, argv, 6);
		if ((argc > 0) && (part_map_parse(nhctx, argc, argv, fn, lineno) == 0)) {
/*			printf("MAP %d '%s' '%s' '%s' '%s' '%s'\n", argc, argv[0], argv[1], argv[2], argv[3], argv[4]);*/
			cnt++;
		}
	}

	fclose(f);
	return cnt;
}


nethlp_elem_ctx_t *nethlp_elem_new(nethlp_ctx_t *nhctx, nethlp_elem_ctx_t *prealloc, const char *id)
{
	if (prealloc == NULL) {
		prealloc = malloc(sizeof(nethlp_elem_ctx_t));
		prealloc->alloced = 1;
	}
	else
		prealloc->alloced = 0;
	prealloc->nhctx = nhctx;
	prealloc->id = pcb_strdup(id);
	htsp_init(&prealloc->attr, strhash, strkeyeq);
	return prealloc;
}

void nethlp_elem_refdes(nethlp_elem_ctx_t *ectx, const char *refdes)
{
	htsp_set(&ectx->nhctx->id2refdes, pcb_strdup(ectx->id), pcb_strdup(refdes));
}

void nethlp_elem_attr(nethlp_elem_ctx_t *ectx, const char *key, const char *val)
{
	htsp_set(&ectx->attr, pcb_strdup(key), pcb_strdup(val));
}

static void elem_map_apply(nethlp_elem_ctx_t *ectx, nethlp_rule_t *r)
{
	char *dst;
	re_se_backref(r->val, &dst, r->new_val);
/*	printf("Map add: '%s' -> '%s'\n", r->new_key, dst);*/
	htsp_set(&ectx->attr, pcb_strdup(r->new_key), pcb_strdup(dst));
}

void nethlp_elem_done(nethlp_elem_ctx_t *ectx)
{
	htsp_entry_t *e;
	char *refdes, *footprint, *value;

	/* apply part map */
	for (e = htsp_first(&ectx->attr); e; e = htsp_next(&ectx->attr, e)) {
		nethlp_rule_t *r, *best_rule = NULL;
		int best_prio = 0;

		for(r = ectx->nhctx->part_rules; r != NULL; r = r->next) {
			if (r->prio == nethlp_prio_always) {
				if (re_se_exec(r->key, e->key) && re_se_exec(r->val, e->value)) {
					/* always apply - backref is already set up for val */
					elem_map_apply(ectx, r);
				}
			}
			else if ((r->prio >= best_prio) && re_se_exec(r->key, e->key) && re_se_exec(r->val, e->value)) {
				best_prio = r->prio;
				best_rule = r;
			}
		}
		if (best_rule != NULL) {
			re_se_exec(best_rule->val, e->value); /* acquire the back refs again */
			elem_map_apply(ectx, best_rule);
		}
	}

	refdes = htsp_get(&ectx->nhctx->id2refdes, ectx->id);
	if (refdes != NULL) {
		/* look up hardwired attributes */
		footprint = htsp_get(&ectx->attr, "pcb-rnd-footprint");
		if (footprint == NULL) footprint = htsp_get(&ectx->attr, "footprint");
		if (footprint == NULL) footprint = htsp_get(&ectx->attr, "Footprint");
		if (footprint == NULL) footprint = "";

		value = htsp_get(&ectx->attr, "pcb-rnd-value");
		if (value == NULL) value = htsp_get(&ectx->attr, "value");
		if (value == NULL) value = htsp_get(&ectx->attr, "Value");
		if (value == NULL) value = "";

		/* create elemet */
		pcb_actionl("ElementList", "Need", refdes, footprint, value, NULL);
/*		printf("Elem '%s' -> %s:%s:%s\n", ectx->id, refdes, footprint, value);*/
	}
	else
		pcb_message(PCB_MSG_ERROR, "Ignoring part %s: no refdes\n", ectx->id);

	/* free */
	for (e = htsp_first(&ectx->attr); e; e = htsp_next(&ectx->attr, e)) {
		free(e->key);
		free(e->value);
	}
	htsp_uninit(&ectx->attr);
	free(ectx->id);
	if (ectx->alloced)
		free(ectx);
}


nethlp_net_ctx_t *nethlp_net_new(nethlp_ctx_t *nhctx, nethlp_net_ctx_t *prealloc, const char *netname)
{
	if (prealloc == NULL) {
		prealloc = malloc(sizeof(nethlp_net_ctx_t));
		prealloc->alloced = 1;
	}
	else
		prealloc->alloced = 0;
	prealloc->nhctx = nhctx;

	prealloc->netname = pcb_strdup(netname);
	return prealloc;
}

void nethlp_net_add_term(nethlp_net_ctx_t *nctx, const char *part, const char *pin)
{
	char *refdes = htsp_get(&nctx->nhctx->id2refdes, part);
	char term[256];
	if (refdes == NULL) {
		pcb_message(PCB_MSG_ERROR, "nethelper: can't resolve refdes of part %s\n", part);
	}
	pcb_snprintf(term, sizeof(term), "%s-%s", refdes, pin);
	pcb_actionl("Netlist", "Add",  nctx->netname, term, NULL);
}

void nethlp_net_destroy(nethlp_net_ctx_t *nctx)
{
	free(nctx->netname);
	if (nctx->alloced)
		free(nctx);
}

