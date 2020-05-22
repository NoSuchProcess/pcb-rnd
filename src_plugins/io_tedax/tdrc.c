/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - generic DRC import/export
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

/* This file is for the generic tEDAx DRC block ("begin drc...", as documented
   in the tEDAx project) */

#include "config.h"

#include <stdio.h>

#include "board.h"
#include "parse.h"
#include <librnd/core/safe_fs.h>
#include <librnd/core/error.h>
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include "tdrc.h"
#include "tdrc_query.h"
#include "tdrc_keys_sphash.h"


#define SOURCE "tedax_drc"

/* Old, hardwired or stock rules - so there's no interference with user
   settings */
typedef struct {
	const char *oconf;   /* full path in the conf - old drc for compatibility */
	const char *nconf;   /* full path in the conf - new drc (drc_query) */
	const char *ttype;   /* tEDAx layer type */
	const char *tkind;   /* tEDAx layer kind */
} drc_rule_t;

#define QDRC "plugins/drc_query/definitions/"

static const drc_rule_t stock_rules[] = {
	{"design/bloat",     QDRC "min_copper_clearance", "copper", "gap"},
	{"design/shrink",    QDRC "min_copper_overlap",   "copper", "overlap"},
	{"design/min_wid",   QDRC "min_copper_thickness", "copper", "min_size"},
	{"design/min_slk",   QDRC "min_silk_thickness",   "silk",   "min_size"},
	{"design/min_drill", QDRC "min_drill",            "mech",   "min_size"},
};

#define NUM_STOCK_RULES (sizeof(stock_rules) / sizeof(stock_rules[0]))

int tedax_drc_fsave(pcb_board_t *pcb, const char *drcid, FILE *f)
{
	const drc_rule_t *r;
	rnd_conf_native_t *nat, *val;
	int n;

	fprintf(f, "begin drc v1 ");
	tedax_fprint_escape(f, drcid);
	fputc('\n', f);

	for(n = 0, r = stock_rules; n < NUM_STOCK_RULES; r++,n++) {
		rnd_conf_native_t *nat;
		nat = rnd_conf_get_field(r->nconf);
		if ((nat == NULL) || (nat->prop->src == NULL))
			nat = rnd_conf_get_field(r->oconf);
		if ((nat != NULL) && (nat->prop->src != NULL))
			rnd_fprintf(f, " rule all %s %s %.06mm pcb_rnd_old_drc_from_conf\n", r->ttype, r->tkind, nat->val.coord[0]);
	}

	nat = rnd_conf_get_field("plugins/drc_query/definitions");
	if (nat != NULL) {
		gdl_iterator_t it;
		rnd_conf_listitem_t *i;
		rnd_conflist_foreach(nat->val.list, &it, i) {
			lht_node_t *def = i->prop.src;
			char *tmp, *s, *start, *cfgpath;
			int token[4], n = 0, stay = 1;

			if (strncmp(def->name, "tedax_", 6) != 0) continue;

			tmp = rnd_strdup(def->name+6);
			for(start = s = tmp; stay; s++) {
				if ((*s == '_') || (*s == '\0')) {
					if (*s == '\0') stay = 0;
					if (n < sizeof(token)/sizeof(token[0])) {
						*s = '\0';
						token[n++] = io_tedax_tdrc_keys_sphash(start);
					}
					if (stay)
						*s = ' ';
					start = s+1;
				}
			}

			if (!io_tedax_tdrc_keys_loc_isvalid(token[0])) {
				rnd_message(RND_MSG_ERROR, "invalid layer location for tEDAx DRC rule from drc_query '%s'\n", def->name);
				goto skip;
			}
			if ((token[0] != io_tedax_tdrc_keys_loc_named) && (!io_tedax_tdrc_keys_type_isvalid(token[1]))) {
				rnd_message(RND_MSG_ERROR, "invalid layer type for tEDAx DRC rule from drc_query '%s'\n", def->name);
				goto skip;
			}
			if (!io_tedax_tdrc_keys_op_isvalid(token[2])) {
				rnd_message(RND_MSG_ERROR, "invalid op for tEDAx DRC rule from drc_query '%s'\n", def->name);
				goto skip;
			}

			cfgpath = rnd_concat("design/drc/", def->name, NULL);
			val = rnd_conf_get_field(cfgpath);
			if (val == NULL)
				rnd_message(RND_MSG_ERROR, "tEDAx DRC rule: no configured value for '%s'\n", def->name);
			else if (val->type != RND_CFN_COORD)
				rnd_message(RND_MSG_ERROR, "tEDAx DRC rule: configured value for '%s' is not a coord\n", def->name);
			else
				rnd_fprintf(f, " rule %s %.08mm pcb_rnd_io_tedax_tdrc\n", tmp, val->val.coord[0]);

			free(cfgpath);

			skip:;
			free(tmp);
		}
	}

	fprintf(f, "end drc\n");
	return 0;
}

int tedax_drc_save(pcb_board_t *pcb, const char *drcid, const char *fn)
{
	int res;
	FILE *f;

	f = rnd_fopen_askovr(&PCB->hidlib, fn, "w", NULL);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_drc_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_drc_fsave(pcb, drcid, f);
	fclose(f);
	return res;
}

static int load_stock_rule(char *argv[], double d, rnd_coord_t *val)
{
	const drc_rule_t *r;
	int n;

	for(n = 0, r = stock_rules; n < NUM_STOCK_RULES; r++,n++) {
		if ((strcmp(argv[2], r->ttype) != 0) || (strcmp(argv[3], r->tkind) != 0))
			continue;
		val[n] = d;
		return 0;
	}
	return -1;
}

static const char *op_title(int op)
{
	switch(op) {
		case io_tedax_tdrc_keys_op_min_object_around_cut:  return "min. object around cut";
		case io_tedax_tdrc_keys_op_min_dist_from_boundary: return "min. distance from boundary";
		case io_tedax_tdrc_keys_op_overlap:                return "min. object overlap";
		case io_tedax_tdrc_keys_op_gap:                    return "min. object gap";
		case io_tedax_tdrc_keys_op_max_size:               return "max. object size";
		case io_tedax_tdrc_keys_op_min_size:               return "min. object size";
	}
	return NULL;
}

static char *op_cond(int op, const char **listing, const char *defid)
{
	*listing = "";
	switch(op) {
		case io_tedax_tdrc_keys_op_min_object_around_cut:
			return NULL;
		case io_tedax_tdrc_keys_op_min_dist_from_boundary:
			*listing = "let B ((@.layer.type == MECH) || (@.layer.type == BOUNDARY)) thus @\n";
			return rnd_strdup_printf("(overlap(@, B, $%s)) thus violation(DRCGRP1, @, DRCGRP2, B, DRCEXPECT, $%s)", defid, defid);
		case io_tedax_tdrc_keys_op_overlap:
			*listing = "let B @\n";
			return rnd_strdup_printf("(@.IID < B.IID) && intersect(@, B) && (@.netname == B.netname) && (!intersect(@, B, -1 * $%s)) thus violation(DRCGRP1, @, DRCGRP2, B, DRCEXPECT, $%s)", defid, defid);
		case io_tedax_tdrc_keys_op_gap:
			*listing = "let B @\n";
			return rnd_strdup_printf("(@.IID < B.IID) && (@.netname != B.netname) && intersect(@, B, $%s) thus violation(DRCGRP1, @, DRCGRP2, B, DRCEXPECT, $%s)", defid, defid);
		case io_tedax_tdrc_keys_op_max_size:
			return rnd_strdup_printf("(@.thickness > $%s)", defid);
		case io_tedax_tdrc_keys_op_min_size:
			return rnd_strdup_printf("(@.thickness != 0) && (@.thickness < $%s)", defid);
	}
	return NULL;
}

static const char *type_cond(int type)
{
	switch(type) {
		case io_tedax_tdrc_keys_type_all:    return "";
		case io_tedax_tdrc_keys_type_mech:   return "(@.layer.type == MECH) && ";
		case io_tedax_tdrc_keys_type_vcut:   return "((@.layer.type == MECH) || (@.layer.type == BOUNDARY)) && (@.layer.purpoe == \"vcut\") && ";
		case io_tedax_tdrc_keys_type_umech:  return "((@.layer.type == MECH) || (@.layer.type == BOUNDARY)) && (@.layer.purpoe ~ \"^u\") && ";
		case io_tedax_tdrc_keys_type_pmech:  return "((@.layer.type == MECH) || (@.layer.type == BOUNDARY)) && (@.layer.purpoe ~ \"^p\") && ";
		case io_tedax_tdrc_keys_type_paste:  return "(@.layer.type == PASTE) && ";
		case io_tedax_tdrc_keys_type_mask:   return "(@.layer.type == MASK) && ";
		case io_tedax_tdrc_keys_type_silk:   return "(@.layer.type == SILK) && ";
		case io_tedax_tdrc_keys_type_copper: return "(@.layer.type == COPPER) && ";
	}
	return NULL;
}

static const char *loc_cond(int loc)
{
	switch(loc) {
		case io_tedax_tdrc_keys_loc_all:    return "";
		case io_tedax_tdrc_keys_loc_outer:  return "((@.layer.position == TOP) || (@.layer.position == BOTTOM)) && ";
		case io_tedax_tdrc_keys_loc_inner:  return "(@.layer.position == INTERN) && ";
		case io_tedax_tdrc_keys_loc_bottom: return "(@.layer.position == BOTTOM) && ";
		case io_tedax_tdrc_keys_loc_top:    return "(@.layer.position == TOP) && ";
	}
	return NULL;
}

static void reg_def_rule(pcb_board_t *pcb, const char *id, const char *title, const char *type, const char *query, const char *val_)
{
	char val[128];

	rnd_snprintf(val, sizeof(val), "%smm", val_);

	rnd_actionva(&pcb->hidlib, RULEMOD, "create", id, NULL);
	rnd_actionva(&pcb->hidlib, RULEMOD, "set", id, "title", title, NULL);
	rnd_actionva(&pcb->hidlib, RULEMOD, "set", id, "type", type, NULL);
	rnd_actionva(&pcb->hidlib, RULEMOD, "set", id, "query", query, NULL);

	rnd_actionva(&pcb->hidlib, DEFMOD, "create", id, NULL);
	rnd_actionva(&pcb->hidlib, DEFMOD, "set", id, "title", title, NULL);
	rnd_actionva(&pcb->hidlib, DEFMOD, "set", id, "type", "coord", NULL);
	rnd_actionva(&pcb->hidlib, DEFMOD, "set", id, "default", val, NULL);
}

static void load_drc_query_rule(pcb_board_t *pcb, char *argv[], int loc)
{
	int op = io_tedax_tdrc_keys_sphash(argv[3]);
	const char *title = op_title(op), *clisting;
	char *sop, *defid, *stype = NULL, *query = NULL;

	if (title == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_drc_load(): invalid rule op: '%s'\n", argv[3]);
		return;
	}
	defid = rnd_concat("tedax_", argv[1], "_", argv[2], "_", argv[3], NULL);
	sop = op_cond(op, &clisting, defid);
	if (sop == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_drc_load(): can not apply op: '%s'\n", argv[3]);
		free(defid);
		return;
	}

	if (loc != io_tedax_tdrc_keys_loc_named) {
		int type = io_tedax_tdrc_keys_sphash(argv[2]);
		const char *ctype = type_cond(type);
		const char *cloc = loc_cond(loc);

		if (ctype == NULL) {
			rnd_message(RND_MSG_ERROR, "tedax_drc_load(): invalid layer type: '%s'\n", argv[2]);
			return;
		}

		if (cloc == NULL) {
			rnd_message(RND_MSG_ERROR, "tedax_drc_load(): invalid location: '%s'\n", argv[1]);
			return;
		}

		stype = rnd_concat(argv[1], " ", argv[2], " ", argv[3], NULL);
		query = rnd_concat("rule tedax\n", clisting, "assert ", ctype, cloc, sop, "\n", NULL);
		reg_def_rule(pcb, defid, title, stype, query, argv[4]);
	}
	else {
		stype = rnd_concat("name ", argv[2], " ", argv[3], NULL);
		query = rnd_strdup_printf("rule tedax\n%sassert (@.layer.name == \"%s\") && %s\n", clisting, argv[2], sop);
		reg_def_rule(pcb, defid, title, stype, query, argv[4]);
	}
	free(sop);
	free(query);
	free(stype);
	free(defid);
}

int tedax_drc_fload(pcb_board_t *pcb, FILE *f, const char *blk_id, int silent)
{
	const drc_rule_t *r;
	char line[520], *argv[16];
	int argc, n;
	rnd_coord_t val[NUM_STOCK_RULES] = {0};

	if (tedax_seek_hdr(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	if ((argc = tedax_seek_block(f, "drc", "v1", blk_id, silent, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) < 1)
		return -1;

	tedax_drc_query_rule_clear(pcb, SOURCE);

	while((argc = tedax_getline(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
		if ((strcmp(argv[0], "rule") == 0) && (argc > 4)) {
			rnd_bool succ;
			double d = rnd_get_value(argv[4], "mm", NULL, &succ);
			if (succ) {
				int loc = io_tedax_tdrc_keys_sphash(argv[1]);
				if ((loc != io_tedax_tdrc_keys_loc_all) || (load_stock_rule(argv, d, val) != 0))
					load_drc_query_rule(pcb, argv, loc);
			}
			else
				rnd_message(RND_MSG_ERROR, "ignoring invalid numeric value '%s'\n", argv[4]);;
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "drc") == 0))
			break;
		else
			rnd_message(RND_MSG_ERROR, "ignoring invalid command in drc %s\n", argv[0]);
	}

	for(n = 0, r = stock_rules; n < NUM_STOCK_RULES; r++,n++) {
		if (val[n] > 0) {
			rnd_conf_setf(RND_CFR_DESIGN, r->oconf, -1, "%$mm", val[n]);
			rnd_conf_setf(RND_CFR_DESIGN, r->nconf, -1, "%$mm", val[n]);
		}
	}
	return 0;
}

int tedax_drc_load(pcb_board_t *pcb, const char *fn, const char *blk_id, int silent)
{
	int res;
	FILE *f;

	f = rnd_fopen(&PCB->hidlib, fn, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_drc_load(): can't open %s for reading\n", fn);
		return -1;
	}
	res = tedax_drc_fload(pcb, f, blk_id, silent);
	fclose(f);
	return res;
}


