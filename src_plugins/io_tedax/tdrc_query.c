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
#include <librnd/core/error.h>
#include <librnd/core/actions.h>
#include "tdrc_query.h"

#define RULEMOD "DrcQueryRuleMod"
#define DEFMOD "DrcQueryDefMod"

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


