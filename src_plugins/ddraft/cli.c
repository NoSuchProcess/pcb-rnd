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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "hid_inlines.h"
#include "compat_misc.h"
#include "conf_core.h"
#include "hidlib_conf.h"

#define CLI_MAX_INS_LEN 128

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
	CLI_CENTER,
	CLI_START,
	CLI_END,

	CLI_DIST,
	CLI_OFFS,
	CLI_COORD
} cli_ntype_t;

#define case_object \
	case CLI_PERP: case CLI_PARAL: case CLI_TANGENT: case CLI_CENTER: \
	case CLI_START: case CLI_END

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
	{"center",        CLI_CENTER},
	{"start",         CLI_START},
	{"end",           CLI_END},
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

	pcb_coord_t x, y, dist;
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

static const char *find_rev_type(cli_ntype_t type)
{
	const cli_ntname_t *p;

	for(p = cli_tnames; p->name != NULL; p++)
		if (p->type == type)
			return p->name;

	return "INVALID";
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

#define iscrd(c) ((c == '+') || (c == '-') || (c == '.')  || isdigit(c))
#define last_type ((i > 0) ? dst[i-1].type : CLI_INVALID)

static int cli_parse(cli_node_t *dst, int dstlen, const char *line)
{
	char tmp[128];
	char *sep, *s = strchr(line, ' '), *next; /* skip the instruction */
	int i;
	pcb_bool succ;

	if (s == NULL)
		return 0;

	for(i = 0;; s = next) {
		while(isspace(*s)) s++;
		if (*s == '\0')
			break;
		switch(*s) {
			case '@':
				next = s+1;
				APPEND(CLI_RELATIVE, next);
				continue;
			case '*':
				next = s+1;
				APPEND(CLI_ABSOLUTE, next);
				continue;
			case '<':
				next = s+1;
				APPEND(CLI_ANGLE, next);
				continue;
			case '%':
				APPEND(CLI_OFFS, next);
				dst[i-1].offs = strtod(s+1, &next);
				dst[i-1].invalid = (next == s);
				continue;
			case '~':
				next = s+1;
				APPEND(CLI_DIST, next);
				break;
			default:
				next = strchr(s, ' ');
				if (next == NULL)
					next = s+strlen(s);
				if (iscrd(*s)) {
					switch(last_type) {
						case CLI_INVALID:
						case CLI_FROM:
						case CLI_TO:
							APPEND(CLI_ABSOLUTE, s);
						case CLI_RELATIVE:
						case CLI_ABSOLUTE:
							APPEND(CLI_COORD, next);
							sep = strchr(s, ',');
							if ((sep == NULL) || ((sep - s) > sizeof(tmp)-2)) {
								dst[i-1].invalid = 1;
								break;
							}
							memcpy(tmp, s, sep-s);
							tmp[sep-s] = '\0';
							dst[i-1].x = pcb_get_value_ex(tmp, NULL, NULL, NULL, pcbhl_conf.editor.grid_unit->suffix, &succ);
							if (!succ)
								dst[i-1].invalid = 1;

							sep++;
							if ((next - sep) > sizeof(tmp)-2) {
								dst[i-1].invalid = 1;
								break;
							}
							memcpy(tmp, sep, next-sep);
							tmp[next-sep] = '\0';
							dst[i-1].y = pcb_get_value_ex(tmp, NULL, NULL, NULL, pcbhl_conf.editor.grid_unit->suffix, &succ);
							if (!succ)
								dst[i-1].invalid = 1;
							break;
						case CLI_DIST:
							dst[i-1].dist = pcb_get_value_ex(s, NULL, NULL, NULL, pcbhl_conf.editor.grid_unit->suffix, &succ);
							dst[i-1].invalid = !succ;
							dst[i-1].end = next - line;
							break;
						case CLI_ANGLE:
							dst[i-1].angle = strtod(s, &next);
							dst[i-1].invalid = (dst[i-1].angle > 360.0) || (dst[i-1].angle < -360.0);
							dst[i-1].end = next - line;
							break;
						case_object :
							dst[i-1].id = strtol(s, &next, 10);
							dst[i-1].end = next - line;
							break;
						default:
							APPEND(CLI_INVALID, next);
					}
				}
				else
					APPEND(find_type(s, next-s), next);
		}
		if ((next == NULL) || (*next == '\0'))
			break;
	}

	return i;
}

#undef APPEND
#undef iscrd
#undef last_type

static int cli_cursor_arg(int argc, cli_node_t *argv, int cursor)
{
	int n;
	for(n = 0; n < argc; n++)
		if ((cursor >= argv[n].begin) && (cursor <= argv[n].end))
			return n;
	return -1;
}

static void cli_str_remove(char *str, int from, int to)
{
	if (from >= to)
		return;
	memmove(str+from, str+to, strlen(str+to)+1);
}


static int cli_str_insert(char *str, int from, char *ins, int enforce_space_before)
{
	int inslen = strlen(ins), remain = strlen(str+from), extra = 0;
	if (enforce_space_before && (str[from-1] != ' '))
		extra=1;
	memmove(str+from+inslen+extra, str+from, remain+1);
	if (extra)
		str[from] = ' ';
	memcpy(str+from+extra, ins, inslen);
	return from + inslen + extra;
}

static int cli_apply_coord(cli_node_t *argv, int start, int end, pcb_coord_t *ox, pcb_coord_t *oy, int annot)
{
	int n, relative = 0, have_angle = 0, have_dist = 0, len = (start > 1);
	double angle = 0, dist = 0, lx, ly, x = *ox, y = *oy;
	pcb_coord_t tx, ty;

	for(n = start; n < end; n++) {
		int moved = 0;
		pcb_any_obj_t *to = NULL;

		/* look up the target object for instructions referencing an object */
		switch(argv[n].type) {
			case_object :
				if (argv[n].id > 0) {
					void *p1, *p2, *p3;
					int res;
					res = pcb_search_obj_by_id_(PCB->Data, &p1, &p2, &p3, argv[n].id, PCB_OBJ_LINE);
					if (res == 0)
						res = pcb_search_obj_by_id_(PCB->Data, &p1, &p2, &p3, argv[n].id, PCB_OBJ_ARC);
					if (res == 0)
						res = pcb_search_obj_by_id_(PCB->Data, &p1, &p2, &p3, argv[n].id, PCB_OBJ_POLY);
					if (res != 0)
						to = p2;
				}
				if (to == NULL)
					return -1;
				break;
			default:;
		}

		switch(argv[n].type) {
			case CLI_ABSOLUTE:
				relative = 0;
				len = 0;
				lx = ly = 0;
				break;
			case CLI_RELATIVE:
				relative = 1;
				break;

			case CLI_ANGLE:
				have_angle = 1;
				if (relative) {
					angle += argv[n].angle;
					relative = 0;
				}
				else
					angle = argv[n].angle;
				goto apply;

			case CLI_DIST:
				have_dist = 1;
				if (relative) {
					dist += argv[n].dist;
					relative = 0;
				}
				else
					dist = argv[n].dist;
				goto apply;

			case CLI_COORD:
				if (relative) {
					x += argv[n].x;
					y += argv[n].y;
					relative = 0;
					moved = 1;
				}
				else {
					x = argv[n].x;
					y = argv[n].y;
					len = 0;
					lx = ly = 0;
					moved = 1;
				}
				break;

			case CLI_PERP:
				pcb_message(PCB_MSG_ERROR, "perp not yet implemented\n");
				return -1;
			case CLI_PARAL:
				pcb_message(PCB_MSG_ERROR, "paral not yet implemented\n");
				return -1;
			case CLI_TANGENT:
				pcb_message(PCB_MSG_ERROR, "tangent not yet implemented\n");
				return -1;

			case CLI_CENTER:
				switch(to->type) {
					case PCB_OBJ_LINE:
						x = (((pcb_line_t *)to)->Point1.X + ((pcb_line_t *)to)->Point2.X)/2;
						y = (((pcb_line_t *)to)->Point1.Y + ((pcb_line_t *)to)->Point2.Y)/2;
						break;
					case PCB_OBJ_ARC:
						x = ((pcb_arc_t *)to)->X;
						y = ((pcb_arc_t *)to)->Y;
						break;
					default:
						return -1;
					}
				break;
			case CLI_START:
				switch(to->type) {
					case PCB_OBJ_LINE:
						x = ((pcb_line_t *)to)->Point1.X;
						y = ((pcb_line_t *)to)->Point1.Y;
						break;
					case PCB_OBJ_ARC:
						pcb_arc_get_end((pcb_arc_t *)to, 0, &tx, &ty);
						x = tx;
						y = ty;
						break;
					default:
						return -1;
				}
				break;
			case CLI_END:
				switch(to->type) {
					case PCB_OBJ_LINE:
						x = ((pcb_line_t *)to)->Point2.X;
						y = ((pcb_line_t *)to)->Point2.Y;
						break;
					case PCB_OBJ_ARC:
						pcb_arc_get_end((pcb_arc_t *)to, 1, &tx, &ty);
						x = tx;
						y = ty;
						break;
					default:
						return -1;
				}
				break;

			default:
				goto over;
			apply:;
				if (have_angle && have_dist) {
					x += cos(angle / PCB_RAD_TO_DEG) * dist;
					y += sin(angle / PCB_RAD_TO_DEG) * dist;
					have_angle = have_dist = 0;
					moved = 1;
				}
				break;
		}
		if (argv[n].invalid)
			return -1;

		if (moved) {
			if ((annot) && (len > 0)) {
				pcb_coord_t *c = vtc0_alloc_append(&pcb_ddraft_attached.annot_lines, 4);
				c[0] = pcb_round(lx);
				c[1] = pcb_round(ly);
				c[2] = pcb_round(x);
				c[3] = pcb_round(y);
			}
			angle = atan2(y - ly, x - lx) * PCB_RAD_TO_DEG;
			lx = x;
			ly = y;
			len++;
		}
	}

	over:;
		if (have_angle || have_dist) /* could not apply */
			return -1;
	*ox = pcb_round(x);
	*oy = pcb_round(y);
	return n;
}

static void cli_print_args(int argc, cli_node_t *argv)
{
	int n;
	for(n = 0; n < argc; n++) {
		pcb_trace(" [%d] %s/%d {%d..%d}", n, find_rev_type(argv[n].type), argv[n].type, argv[n].begin, argv[n].end);
		if (!argv[n].invalid) {
			switch(argv[n].type) {
				case CLI_COORD: pcb_trace(": %$mm,%$mm", argv[n].x, argv[n].y); break;
				case CLI_ANGLE: pcb_trace(": %f", argv[n].angle); break;
				case CLI_DIST: pcb_trace(": %$mm", argv[n].dist); break;
				case CLI_OFFS: pcb_trace(": %f", argv[n].offs); break;
				case_object: pcb_trace(": %ld", (long)argv[n].id); break;
				default:;
			}
			pcb_trace("\n");
		}
		else
			pcb_trace(": INVALID\n");
	}
}

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
		pcb_tool_select_by_id(&PCB->hidlib, pcb_ddraft_tool);
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

	while(isspace(*cline)) cline++;
	if ((*cline == '\n') || (*cline == '#') || (*cline == '\0')) { /* empty or comment */
		PCB_ACT_IRES(0);
		return 0;
	}

	len = strlen(cline);
	if (len >= sizeof(cline) - CLI_MAX_INS_LEN)
		line = malloc(len + 1 + CLI_MAX_INS_LEN);
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

	/* look up op */
	opp = find_op(op, oplen);
	if (opp == NULL) {
		if (strcmp(cmd, "/edit") != 0)
			pcb_message(PCB_MSG_ERROR, "ddraft: unknown operator '%s'\n", op);
		PCB_ACT_IRES(-1);
		goto ret0;
	}

	reparse:;
	ndlen = cli_parse(nd, sizeof(nd) / sizeof(nd[0]), line);

	if (*cmd == '/') {
		if (strcmp(cmd, "/click") == 0) {
			PCB_ACT_IRES(opp->click(line, cursor, ndlen, nd));
			cmd = "/edit";
			goto reparse;
		}
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
