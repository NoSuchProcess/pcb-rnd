/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "hid_inlines.h"
#include "compat_misc.h"

typedef struct cli_node_s cli_node_t;

typedef struct {
	const char *name;
	int (*exec)(char *line, int argc, cli_node_t *argv);                 /* command line entered (finished, accepted) */
	int (*click)(char *line, int cursor, int argc, cli_node_t *argv);    /* user clicked on the GUI while editing the command line */
	int (*tab)(char *line, int cursor, int argc, cli_node_t *argv);      /* tab completion */
	int (*edit)(char *line, int cursor, int argc, cli_node_t *argv);     /* called after editing the line or moving the cursor */
} ddraft_op_t;

typedef enum cli_ntype_e {
	CLI_INVALID,
	CLI_FROM,
	CLI_TO,
	CLI_ANGLE,
	CLI_ABSOLUTE,
	CLI_RELATIVE,
	CLI_PERP,
	CLI_PARAL,
	CLI_TANGENT,
	CLI_DIST,
	CLI_OFFS,
	CLI_COORD,
	CLI_ID
} cli_ntype_t;

typedef struct cli_ntname_s {
	const char *name;
	cli_ntype_t type;
} cli_ntname_t;

static const cli_ntname_t cli_tnames[] = {
	{"from",          CLI_FROM},
	{"to",            CLI_TO},
	{"angle",         CLI_ANGLE},
	{"absolute",      CLI_ABSOLUTE},
	{"relative",      CLI_RELATIVE},
	{"perpendicular", CLI_PERP},
	{"parallel",      CLI_PARAL},
	{"tangential",    CLI_TANGENT},
	{"coord",         CLI_COORD},
	{"distance",      CLI_DIST},
	{"length",        CLI_DIST},
	{"offset",        CLI_OFFS},
	{NULL,            0}
};

struct cli_node_s {
	cli_ntype_t type;
	int begin, end; /* cursor pos */
	int invalid;

	pcb_coord_t x, y;
	pcb_angle_t angle, offs;
	pcb_cardinal_t id;
};

static const cli_ntype_t find_type(const char *type, int typelen)
{
	const cli_ntname_t *p, *found = NULL;

	if (typelen < 1)
		return CLI_INVALID;

	for(p = cli_tnames; p->name != NULL; p++) {
		if (pcb_strncasecmp(p->name, type, typelen) == 0) {
			if (found != NULL)
				return CLI_INVALID; /* multiple match */
			found = p;
		}
	}

	if (found == NULL)
		return CLI_INVALID;
	return found->type;
}

#define APPEND(type_, end_) \
do { \
	memset(&dst[i], 0, sizeof(cli_node_t)); \
	dst[i].type = type_; \
	dst[i].begin = s - line; \
	dst[i].end = end_ - line; \
	i++; \
	if (i >= dstlen) \
		return i; \
} while(0)

static int cli_parse(cli_node_t *dst, int dstlen, const char *line)
{
	char *s = strchr(line, ' '), *next; /* skip the instruction */
	int i;

	for(i = 0; s != NULL; s = next) {
		while(isspace(*s)) s++;
		switch(*s) {
			case '@':
				next = s+1;
				APPEND(CLI_RELATIVE, next);
				/* TODO: read the coords */
				continue;
			case '*':
				next = s+1;
				APPEND(CLI_ABSOLUTE, next);
				/* TODO: read the coords */
				continue;
			case '<':
				dst[i].angle = strtod(s+1, &next);
				APPEND(CLI_ANGLE, next);
				dst[i-1].invalid = (next == s);
				continue;
			case '%':
				dst[i].offs = strtod(s+1, &next);
				APPEND(CLI_OFFS, next);
				dst[i-1].invalid = (next == s);
				continue;
			case '#':
				dst[i].angle = strtol(s+1, &next, 10);
				APPEND(CLI_ANGLE, next);
				dst[i-1].invalid = (next == s);
				continue;
			case '~':
				next = s+1;
				APPEND(CLI_DIST, next);
				/* TODO: read the coords */
				break;
			default:
				next = strchr(s, ' ');
				APPEND(find_type(s, next-s), next);
		}
	}

	return i;
}

#undef APPEND

#include "cli_line.c"

static const ddraft_op_t ddraft_ops[] = {
	{"line",    line_exec,    line_click,    line_tab,     line_edit},
	{NULL,      NULL,         NULL,          NULL,         NULL}
};

static const ddraft_op_t *find_op(const char *op, int oplen)
{
	const ddraft_op_t *p, *found = NULL;

	if (oplen < 1)
		return NULL;

	for(p = ddraft_ops; p->name != NULL; p++) {
		if (pcb_strncasecmp(p->name, op, oplen) == 0) {
			if (found != NULL)
				return NULL; /* multiple match */
			found = p;
		}
	}

	return found;
}

static const char pcb_acts_ddraft[] = "ddraft([command])";
static const char pcb_acth_ddraft[] = "Enter 2d drafting CLI mode or execute command";
static fgw_error_t pcb_act_ddraft(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *args, *cmd, *line, sline[1024], *op;
	const char *cline = NULL;
	int cursor, len, oplen;
	const ddraft_op_t *opp;
	cli_node_t nd[128];
	int ndlen;

	if (argc == 1) {
		pcb_cli_enter("ddraft", "ddraft");
		PCB_ACT_IRES(0);
		return 0;
	}

	PCB_ACT_CONVARG(1, FGW_STR, ddraft, cmd = argv[1].val.str);
	if (strcmp(cmd, "/exit") == 0) {
		pcb_cli_leave();
		PCB_ACT_IRES(0);
		return 0;
	}

	/* make a safe copy of the command line, either on stack or on heap if too large */
	if ((cmd != NULL) && (*cmd != '/'))
		cline = cmd;
	else
		cline = pcb_hid_command_entry(NULL, &cursor);
	if (cline == NULL)
		cline = "";
	len = strlen(cline);
	if (len >= sizeof(cline))
		line = malloc(len+1);
	else
		line = sline;
	memcpy(line, cline, len+1);

	/* split op and arguments; recalculate cursor so it's within the arguments */
	op = line;
	args = strpbrk(op, " \t");
	if (args != NULL)
		oplen = args - op;
	else
		oplen = len;

	if (oplen < 1) { /* empty command is a nop */
		PCB_ACT_IRES(0);
		goto ret0;
	}

	/* look up op */
	opp = find_op(op, oplen);
	if (opp == NULL) {
		if (strcmp(cmd, "/edit") != 0)
			pcb_message(PCB_MSG_ERROR, "ddraft: unknown operator '%s'\n", op);
		PCB_ACT_IRES(-1);
		goto ret0;
	}

	ndlen = cli_parse(nd, sizeof(nd) / sizeof(nd[0]), line);

	if (*cmd == '/') {
		if (strcmp(cmd, "/click") == 0)
			PCB_ACT_IRES(opp->click(line, cursor, ndlen, nd));
		else if (strcmp(cmd, "/tab") == 0)
			PCB_ACT_IRES(opp->tab(line, cursor, ndlen, nd));
		else if (strcmp(cmd, "/edit") == 0)
			PCB_ACT_IRES(opp->edit(line, cursor, ndlen, nd));
		else
			PCB_ACT_IRES(0); /* ignore anything unhandled */
		goto ret0;
	}

	PCB_ACT_IRES(opp->exec(line, ndlen, nd));

	ret0:;
	if (line != sline)
		free(line);
	return 0;
}
