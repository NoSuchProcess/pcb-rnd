/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - load DRC rules and constants for drc_query
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

/* This file is for the drc_query specific tEDAx block ("begin drc_query_* ...",
   as documented in the pcb-rnd project) */

#include "config.h"

#include <stdio.h>

#include "board.h"
#include "parse.h"
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/error.h>
#include <librnd/core/actions.h>
#include "tdrc_query.h"

#define RULEMOD "DrcQueryRuleMod"
#define DEFMOD "DrcQueryDefMod"

int tedax_drc_query_rule_clear(pcb_board_t *pcb, const char *target_src)
{
	rnd_actionva(&pcb->hidlib, RULEMOD, "set", "clear", target_src, NULL);
	rnd_actionva(&pcb->hidlib, DEFMOD, "set", "clear", target_src, NULL);
}

int tedax_drc_query_rule_parse(pcb_board_t *pcb, FILE *f, const char *src, char *rule_name)
{
	int argc;
	char line[520], *argv[4];
	gds_t qry;

	gds_init(&qry);
	rnd_actionva(&pcb->hidlib, RULEMOD, "create", rule_name, NULL);

	while((argc = tedax_getline(f, line, sizeof(line), argv, 2)) >= 0) {
		if ((strcmp(argv[0], "type") == 0) || (strcmp(argv[0], "title") == 0) || (strcmp(argv[0], "desc") == 0)) {
			rnd_actionva(&pcb->hidlib, RULEMOD, "set", rule_name, argv[0], argv[1], NULL);
		}
		else if (strcmp(argv[0], "query") == 0) {
			gds_append_str(&qry, argv[1]);
			gds_append(&qry, '\n');
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "drc_query_rule") == 0))
			break;
		else
			rnd_message(RND_MSG_ERROR, "ignoring invalid command in drc_query_rule %s\n", argv[0]);
	}


	if (qry.used > 0) {
		rnd_actionva(&pcb->hidlib, RULEMOD, "set", rule_name, "query", qry.array, NULL);
		gds_uninit(&qry);
	}

	if (src != NULL)
		rnd_actionva(&pcb->hidlib, RULEMOD, "set", rule_name, "source", src, NULL);

	return 0;
}

int tedax_drc_query_def_parse(pcb_board_t *pcb, FILE *f, const char *src, char *rule_name)
{
	int argc;
	char line[520], *argv[4];

	rnd_actionva(&pcb->hidlib, DEFMOD, "create", rule_name, NULL);

	while((argc = tedax_getline(f, line, sizeof(line), argv, 2)) >= 0) {
		if ((strcmp(argv[0], "type") == 0) || (strcmp(argv[0], "desc") == 0) || (strcmp(argv[0], "default") == 0)) {
			rnd_actionva(&pcb->hidlib, DEFMOD, "set", rule_name, argv[0], argv[1], NULL);
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "drc_query_def") == 0))
			break;
		else
			rnd_message(RND_MSG_ERROR, "ignoring invalid command in drc_query_def %s\n", argv[0]);
	}

	if (src != NULL)
		rnd_actionva(&pcb->hidlib, DEFMOD, "set", rule_name, "source", src, NULL);

	return 0;
}

int tedax_drc_query_fload(pcb_board_t *pcb, FILE *f, const char *blk_id, const char *src, int silent)
{
	char line[520], *argv[16];
	int argc;
	long cnt = 0;

	if (tedax_seek_hdr(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	while((argc = tedax_getline(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
		if ((argc < 2) || (strcmp(argv[0], "begin") != 0))
			continue;
		if (strcmp(argv[1], "drc_query_rule") == 0) {
			if (strcmp(argv[2], "v1") != 0) {
				if (!silent)
					rnd_message(RND_MSG_ERROR, "Wrong drc_query_rule version: %s\n", argv[2]);
				continue;
			}
			if ((blk_id != NULL) && (strcmp(argv[3], blk_id) != 0))
				continue;
			if (tedax_drc_query_rule_parse(pcb, f, src, argv[3]) < 0)
				return -1;
			cnt++;
		}
		if (strcmp(argv[1], "drc_query_def") == 0) {
			if (strcmp(argv[2], "v1") != 0) {
				if (!silent)
					rnd_message(RND_MSG_ERROR, "Wrong drc_query_def version: %s\n", argv[2]);
				continue;
			}
			if ((blk_id != NULL) && (strcmp(argv[3], blk_id) != 0))
				continue;
			if (tedax_drc_query_def_parse(pcb, f, src, argv[3]) < 0)
				return -1;
		}
	}
	return (cnt == 0) ? -1 : 0;
}

int tedax_drc_query_load(pcb_board_t *pcb, const char *fn, const char *blk_id, const char *src, int silent)
{
	int res;
	FILE *f;

	f = rnd_fopen(&PCB->hidlib, fn, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_drc_query_load(): can't open %s for reading\n", fn);
		return -1;
	}
	res = tedax_drc_query_fload(pcb, f, blk_id, src, silent);
	fclose(f);
	return res;
}

static const char *drc_query_any_get(const char *act, const char *ruleid, const char *field, int *err)
{
	fgw_error_t ferr;
	fgw_arg_t res, args[4];

	args[0].type = FGW_VOID;
	args[1].type = FGW_STR; args[1].val.cstr = "get";
	args[2].type = FGW_STR; args[2].val.cstr = ruleid;
	args[3].type = FGW_STR; args[3].val.cstr = field;

	ferr = rnd_actionv_bin(&PCB->hidlib, act, &res, 4, args);
	if (ferr != 0) {
		*err = 1;
		return "-";
	}
	if (res.type != FGW_STR) {
		fgw_arg_free(&rnd_fgw, &res);
		*err = 1;
		return "-";
	}
	return res.val.cstr;
}

static const char *drc_query_rule_get(const char *ruleid, const char *field, int *err)
{
	return drc_query_any_get("DrcQueryRuleMod", ruleid, field, err);
}

static const char *drc_query_def_get(const char *ruleid, const char *field, int *err)
{
	return drc_query_any_get("DrcQueryDefMod", ruleid, field, err);
}

static void print_multiline(FILE *f, const char *prefix, const char *text)
{
	const char *curr, *next;
	while(isspace(*text)) text++;
	for(curr = text; curr != NULL; curr = next) {
		next = strchr(curr, '\n');
		if (next != NULL) {
			fprintf(f, "%s ", prefix);
			fwrite(curr, next-curr, 1, f);
			fputc('\n', f);
			while(*next == '\n') next++;
			if (*next == '\0') next = NULL;
		}
	}
}

int tedax_drc_query_def_fsave(pcb_board_t *pcb, const char *defid, FILE *f)
{
	int err = 0;

	fprintf(f, "\nbegin drc_query_def v1 ");
	tedax_fprint_escape(f, defid);
	fputc('\n', f);

	fprintf(f, "	type %s\n",     drc_query_def_get(defid, "type", &err));
	fprintf(f, "	default %s\n",  drc_query_def_get(defid, "default", &err));
	fprintf(f, "	desc %s\n",     drc_query_def_get(defid, "desc", &err));

	fprintf(f, "end drc_query_def\n");

	return err;
}


int tedax_drc_query_rule_fsave(pcb_board_t *pcb, const char *ruleid, FILE *f, rnd_bool save_def)
{
	
	int err = 0;

	/* when enabled, get a list of definitions used by the rule and save them all */
	if (save_def) {
		fgw_error_t ferr;
		fgw_arg_t res, args[4];

		args[0].type = FGW_VOID;
		args[1].type = FGW_STR; args[1].val.cstr = "get";
		args[2].type = FGW_STR; args[2].val.cstr = ruleid;
		args[3].type = FGW_STR; args[3].val.cstr = "defs";

		ferr = rnd_actionv_bin(&PCB->hidlib, "DrcQueryRuleMod", &res, 4, args);
		if (ferr == 0) {
			if ((res.type & FGW_STR) && (res.val.str != NULL) && (*res.val.str != '\0')) {
				char *lst, *curr, *next;
				for(curr = lst = rnd_strdup(res.val.str); next != NULL; curr = next) {
					next = strchr(curr, '\n');
					if (next != NULL) *next = '\0';
					if (tedax_drc_query_def_fsave(pcb, curr, f) != 0) {
						err = -1;
						break;
					}
				}
				free(lst);
			}
			fgw_arg_free(&rnd_fgw, &res);
			if (err != 0)
				return err;
		}
	}

	fprintf(f, "\nbegin drc_query_rule v1 ");
	tedax_fprint_escape(f, ruleid);
	fputc('\n', f);

	fprintf(f, "	type %s\n",  drc_query_rule_get(ruleid, "type", &err));
	fprintf(f, "	title %s\n", drc_query_rule_get(ruleid, "title", &err));
	fprintf(f, "	desc %s\n",  drc_query_rule_get(ruleid, "desc", &err));

	print_multiline(f, "	query", drc_query_rule_get(ruleid, "query", &err));

	fprintf(f, "end drc_query_rule\n");
	return err;
}

int tedax_drc_query_save(pcb_board_t *pcb, const char *ruleid, const char *fn)
{
	int res;
	FILE *f;

	if (ruleid == NULL) {
TODO("save all");
rnd_message(RND_MSG_ERROR, "Can't save all rules yet, please name one rule to save\n");
		return -1;
	}

	f = rnd_fopen_askovr(&PCB->hidlib, fn, "w", NULL);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_drc_query_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_drc_query_rule_fsave(pcb, ruleid, f, rnd_true);
	fclose(f);
	return res;
}
