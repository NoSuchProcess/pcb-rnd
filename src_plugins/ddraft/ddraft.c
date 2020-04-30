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

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <genvector/gds_char.h>
#include <genvector/vtp0.h>

#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/plugins.h>
#include "event.h"
#include <librnd/core/actions.h>
#include "funchash_core.h"
#include "search.h"
#include "centgeo.h"
#include <librnd/core/misc_util.h>
#include "board.h"
#include "data_it.h"
#include "obj_line.h"
#include "undo.h"
#include "fields_sphash.h"
#include "draw_wireframe.h"
#include "conf_core.h"
#include <librnd/core/conf_hid.h>
#include <librnd/core/vtc0.h>

static const char *ddraft_cookie = "ddraft plugin";
static int pcb_ddraft_tool;

typedef struct {
	pcb_line_t line;
	int line_valid;

	vtc0_t annot_lines; /* each 4 coords specify a line */
} pcb_ddraft_attached_t;

pcb_ddraft_attached_t pcb_ddraft_attached;

void pcb_ddraft_attached_reset(void)
{
	pcb_ddraft_attached.line_valid = 0;
	vtc0_truncate(&pcb_ddraft_attached.annot_lines, 0);
}

#define EDGE_TYPES (PCB_OBJ_LINE | PCB_OBJ_ARC)
#define CUT_TYPES (PCB_OBJ_LINE | PCB_OBJ_ARC)

static void list_by_flag(pcb_data_t *data, vtp0_t *dst, unsigned long types, unsigned long flag)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	
	for(o = pcb_data_first(&it, data, types); o != NULL; o = pcb_data_next(&it))
		if (PCB_FLAG_TEST(flag, o))
			vtp0_append(dst, o);
}

#include "trim.c"
#include "constraint.c"

static long do_trim_split(vtp0_t *edges, int kwobj, int trim)
{
	pcb_objtype_t type;
	void *ptr1, *ptr2, *ptr3;
	rnd_coord_t x, y;
	long res = 0;

	switch(kwobj) {
		case F_Object:
			for(;;) {
				x = RND_MAX_COORD;
				rnd_hid_get_coords("Select an object to cut or press esc", &x, &y, 1);
				if (x == RND_MAX_COORD)
					break;

				type = pcb_search_screen(x, y, CUT_TYPES, &ptr1, &ptr2, &ptr3);
				if (type == 0) {
					rnd_message(RND_MSG_ERROR, "Can't cut that object\n");
					continue;
				}
				res += pcb_trim_split(edges, NULL, (pcb_any_obj_t *)ptr2, REMO_INVALID, x, y, trim);
				rnd_gui->invalidate_all(rnd_gui);
			}
			break;
		default:
			rnd_message(RND_MSG_ERROR, "Invalid second argument\n");
			return -1;
	}

	return res;
}

static const char pcb_acts_trim_split[] = "trim([selected|found|object], [selected|found|object])\nsplit([selected|found|object], [selected|found|object])";
static const char pcb_acth_trim_split[] = "Use one or more objects as cutting edge and trim or split other objects. First argument is the cutting edge";
static fgw_error_t pcb_act_trim_split(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *actname = argv[0].val.func->name;
	int kwcut = F_Object, kwobj = F_Object;
	pcb_objtype_t type;
	void *ptr1, *ptr2, *ptr3;
	rnd_coord_t x, y;
	vtp0_t edges;

	rnd_PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, trim_split, kwcut = fgw_keyword(&argv[1]));
	rnd_PCB_ACT_MAY_CONVARG(2, FGW_KEYWORD, trim_split, kwobj = fgw_keyword(&argv[2]));

	vtp0_init(&edges);

	if ((kwobj == kwcut) && (kwobj != F_Object)) {
		rnd_message(RND_MSG_ERROR, "Both cutting edge and objects to cut can't be '%s'\n", kwcut == F_Selected ? "selected" : "found");
		goto err;
	}

	switch(kwcut) {
		case F_Object:
			rnd_hid_get_coords("Select cutting edge object", &x, &y, 0);
			type = pcb_search_screen(x, y, EDGE_TYPES, &ptr1, &ptr2, &ptr3);
			if (type == 0) {
				rnd_message(RND_MSG_ERROR, "Invalid cutting edge object\n");
				goto err;
			}
			vtp0_append(&edges, ptr2);
			break;
		case F_Selected:
			list_by_flag(PCB->Data, &edges, EDGE_TYPES, PCB_FLAG_SELECTED);
			break;
		case F_Found:
			list_by_flag(PCB->Data, &edges, EDGE_TYPES, PCB_FLAG_FOUND);
			break;
		default:
			rnd_message(RND_MSG_ERROR, "Invalid first argument\n");
			goto err;
	}

	if (vtp0_len(&edges) < 1) {
		rnd_message(RND_MSG_ERROR, "No cutting edge found\n");
		goto err;
	}


	if (do_trim_split(&edges, kwobj, (*actname == 't')) < 0)
		goto err;

	RND_ACT_IRES(0);
	vtp0_uninit(&edges);
	return 0;

	err:;
	RND_ACT_IRES(-1);
	vtp0_uninit(&edges);
	return 0;
}


static const char pcb_acts_split_idp[] = "PcbSplit(cutting_edges, objs)";
static const char pcb_acth_split_idp[] = "Split objects (idpath or idpath list) with cutting edges (idpath or idpath list), returning the idpath list of the newly created objects";
static fgw_error_t pcb_act_split_idp(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_idpath_t *idp;
	pcb_idpath_list_t *idpl;
	gdl_iterator_t it;
	pcb_any_obj_t *obj;
	vtp0_t edges, objs, newobjs, *vect;
	long n;

	vtp0_init(&edges);
	vtp0_init(&objs);
	vtp0_init(&newobjs);

	/* read two obj or objlist args */
	for(n = 1; n < 3; n++) {
		vect = (n == 1) ? &edges : &objs;

		RND_PCB_ACT_CONVARG(n, FGW_IDPATH, split_idp, idp = fgw_idpath(&argv[n]));
		if (idp == NULL)
			goto invptr;
		if (fgw_ptr_in_domain(&rnd_fgw, &argv[n], RND_PTR_DOMAIN_IDPATH)) {
			obj = pcb_idpath2obj(PCB, idp);
			if ((obj == NULL) || ((obj->type & PCB_OBJ_CLASS_REAL) == 0))
				goto invptr;
			vtp0_append(vect, obj);
		}
		else if (fgw_ptr_in_domain(&rnd_fgw, &argv[n], RND_PTR_DOMAIN_IDPATH_LIST)) {
			idpl = (pcb_idpath_list_t *)idp;
			pcb_idpath_list_foreach(idpl, &it, idp) {
				obj = pcb_idpath2obj(PCB, idp);
				if ((obj == NULL) || ((obj->type & PCB_OBJ_CLASS_REAL) == 0))
					goto invptr;
				vtp0_append(vect, obj);
			}
		}
		else
			goto invptr;
	}

	/* execute the splits */
	for(n = 0; n < vtp0_len(&objs); n++)
		pcb_trim_split(&edges, &newobjs, objs.array[n], 0, 0, 0, 0);

	/* copy over the objects to an idpath_list that is then returned */
	idpl = calloc(sizeof(pcb_idpath_list_t), 1);
	fgw_ptr_reg(&rnd_fgw, res, RND_PTR_DOMAIN_IDPATH_LIST, FGW_PTR | FGW_STRUCT, idpl);
	for(n = 0; n < vtp0_len(&newobjs); n++) {
		idp = pcb_obj2idpath(newobjs.array[n]);
		if (idp != NULL)
			pcb_idpath_list_append(idpl, idp);
	}

	vtp0_uninit(&edges);
	vtp0_uninit(&objs);
	vtp0_uninit(&newobjs);
	return 0;

	invptr:;
	vtp0_uninit(&edges);
	vtp0_uninit(&objs);
	vtp0_uninit(&newobjs);
	return FGW_ERR_PTR_DOMAIN;
}


#include "constraint_gui.c"

#define load_arr(arr, ctr, msg, fgw_type_, fgw_val_) \
do { \
	int n; \
	if (argc-2 >= sizeof(arr) / sizeof(arr[0])) { \
		rnd_message(RND_MSG_ERROR, "constraint: Too many " msg "\n"); \
		RND_ACT_IRES(-1); \
		return 0; \
	} \
	ctr = 0; \
	for(n = 2; n < argc; n++) { \
		RND_PCB_ACT_CONVARG(n, fgw_type_, constraint, arr[ctr] = fgw_val_); \
		ctr++; \
	} \
} while(0)

static const char pcb_acts_constraint[] = "constraint(type, off)\nconstraint(type, value, [value...])";
static const char pcb_acth_constraint[] = "Configure or remove a drawing constraint";
static fgw_error_t pcb_act_constraint(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *stype = NULL;
	int type;

	rnd_PCB_ACT_MAY_CONVARG(1, FGW_STR, constraint, stype = argv[1].val.str);
	if (stype == NULL) {
		RND_ACT_IRES(constraint_gui());
		return 0;
	}
	type = ddraft_fields_sphash(stype);
	switch(type) {
		case ddraft_fields_line_angle:
			load_arr(cons.line_angle, cons.line_angle_len, "line angles", FGW_DOUBLE, argv[n].val.nat_double);
			cons_changed();
			break;
		case ddraft_fields_move_angle:
			load_arr(cons.move_angle, cons.move_angle_len, "move angles", FGW_DOUBLE, argv[n].val.nat_double);
			cons_changed();
			break;
		case ddraft_fields_line_angle_mod:
			cons.line_angle_mod = 0;
			rnd_PCB_ACT_MAY_CONVARG(2, FGW_DOUBLE, constraint, cons.line_angle_mod = argv[2].val.nat_double);
			cons_changed();
			break;
		case ddraft_fields_move_angle_mod:
			cons.move_angle_mod = 0;
			rnd_PCB_ACT_MAY_CONVARG(2, FGW_DOUBLE, constraint, cons.move_angle_mod = argv[2].val.nat_double);
			cons_changed();
			break;
		case ddraft_fields_line_length:
			load_arr(cons.line_length, cons.line_length_len, "line lengths", FGW_COORD, fgw_coord(&argv[n]));
			cons_changed();
			break;
		case ddraft_fields_move_length:
			load_arr(cons.move_length, cons.move_length_len, "move lengths", FGW_COORD, fgw_coord(&argv[n]));
			cons_changed();
			break;
		case ddraft_fields_line_length_mod:
			cons.line_length_mod = 0;
			rnd_PCB_ACT_MAY_CONVARG(2, FGW_COORD, constraint, cons.line_length_mod = fgw_coord(&argv[2]));
			cons_changed();
			break;
		case ddraft_fields_move_length_mod:
			cons.move_length_mod = 0;
			rnd_PCB_ACT_MAY_CONVARG(2, FGW_COORD, constraint, cons.move_length_mod = fgw_coord(&argv[2]));
			cons_changed();
			break;

		case ddraft_fields_reset:
			memset(&cons, 0, sizeof(cons));
			cons_changed();
			break;
		case ddraft_fields_SPHASH_INVALID:
			rnd_message(RND_MSG_ERROR, "constraint: invalid field '%s'\n", stype);
			RND_ACT_IRES(-1);
			return 0;
			break;
	}

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_perp_paral[] = "perp()";
static const char pcb_acth_perp_paral[] = "Draw a line perpendicular to another line";
static fgw_error_t pcb_act_perp_paral(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *actname = argv[0].val.func->name;
	pcb_objtype_t type;
	void *ptr1, *ptr2, *ptr3;
	rnd_coord_t x, y;
	double dx, dy;
	pcb_line_t *line;

	rnd_hid_get_coords("Select target object", &x, &y, 0);
	type = pcb_search_screen(x, y, EDGE_TYPES, &ptr1, &ptr2, &ptr3);

	if (type != PCB_OBJ_LINE) {
		rnd_hid_get_coords("Select target object", &x, &y, 1);
		type = pcb_search_screen(x, y, EDGE_TYPES, &ptr1, &ptr2, &ptr3);
	}

	if (type != PCB_OBJ_LINE) {
		rnd_message(RND_MSG_ERROR, "%s: target object must be a line\n", actname);
		RND_ACT_IRES(-1);
		return 0;
	}

	line = (pcb_line_t *)ptr2;
	dx = line->Point2.X - line->Point1.X;
	dy = line->Point2.Y - line->Point1.Y;
	if ((dx == 0.0) && (dy == 0.0)) {
		rnd_message(RND_MSG_ERROR, "%s: target line must be longer than 0\n", actname);
		RND_ACT_IRES(-1);
		return 0;
	}

	cons.line_angle_len = 2;
	cons.line_angle[0] = atan2(dy, dx) * RND_RAD_TO_DEG;
	if (actname[1] == 'e') /* perp */
		cons.line_angle[0] += 90;
	cons.line_angle[1] = fmod(cons.line_angle[0]+180, 360);
	cons_changed();

	RND_ACT_IRES(0);
	return 0;
}


static const char pcb_acts_tang[] = "tang()";
static const char pcb_acth_tang[] = "Draw a line to be tangential to a circle";
static fgw_error_t pcb_act_tang(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_objtype_t type;
	void *ptr1, *ptr2, *ptr3;
	rnd_coord_t x, y;
	double d, r, base;
	pcb_route_object_t *line;
	pcb_arc_t *arc;


	if (((pcb_crosshair.AttachedLine.State != PCB_CH_STATE_SECOND) && (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_THIRD)) || (pcb_crosshair.Route.size < 1)) {
		err_nonline:;
		rnd_message(RND_MSG_ERROR, "tang: must be in line drawing mode with the first point already set\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	line = &pcb_crosshair.Route.objects[pcb_crosshair.Route.size-1];
	if (line->type != PCB_OBJ_LINE)
		goto err_nonline;


	rnd_hid_get_coords("Select target arc", &x, &y, 0);
	type = pcb_search_screen(x, y, EDGE_TYPES, &ptr1, &ptr2, &ptr3);

	if (type != PCB_OBJ_ARC) {
		rnd_hid_get_coords("Select target arc", &x, &y, 1);
		type = pcb_search_screen(x, y, EDGE_TYPES, &ptr1, &ptr2, &ptr3);
	}

	if (type != PCB_OBJ_ARC) {
		rnd_message(RND_MSG_ERROR, "tang: target object must be an arc\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	arc = (pcb_arc_t *)ptr2;
	if (fabs((double)(arc->Height - arc->Width)) > 100) {
		rnd_message(RND_MSG_ERROR, "tang: elliptical arcs are not supported (%$mm != %$mm)\n", arc->Height, arc->Width);
		RND_ACT_IRES(-1);
		return 0;
	}

	d = rnd_distance(arc->X, arc->Y, line->point1.X, line->point1.Y);
	r = arc->Width;

	if (d <= r) {
		rnd_message(RND_MSG_ERROR, "tang: line must start outside of the circle\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	base = atan2(arc->Y - line->point1.Y, arc->X - line->point1.X);

	cons.line_angle_len = 2;
	cons.line_angle[0] = (base + asin(r / d)) * RND_RAD_TO_DEG;
	cons.line_angle[1] = (base + asin(-r / d)) * RND_RAD_TO_DEG;
	cons_changed();

	RND_ACT_IRES(0);
	return 0;
}

void ddraft_tool_draw_attached(rnd_hidlib_t *hl)
{
	int n;
	rnd_render->set_line_cap(pcb_crosshair.GC, rnd_cap_round);
	rnd_render->set_line_width(pcb_crosshair.GC, 1);
	rnd_render->set_color(pcb_crosshair.GC, rnd_color_grey33);
	for(n = 0; n < vtc0_len(&pcb_ddraft_attached.annot_lines); n += 4) {
		rnd_coord_t *c = &pcb_ddraft_attached.annot_lines.array[n];
		rnd_render->draw_line(pcb_crosshair.GC, c[0], c[1], c[2], c[3]);
	}

	if (pcb_ddraft_attached.line_valid) {
		rnd_render->set_color(pcb_crosshair.GC, &PCB_CURRLAYER(PCB)->meta.real.color);
		pcb_draw_wireframe_line(pcb_crosshair.GC,
			pcb_ddraft_attached.line.Point1.X, pcb_ddraft_attached.line.Point1.Y, pcb_ddraft_attached.line.Point2.X, pcb_ddraft_attached.line.Point2.Y,
			conf_core.design.line_thickness, 0);
	}
}

#include "cli.c"

static rnd_action_t ddraft_action_list[] = {
	{"trim", pcb_act_trim_split, pcb_acth_trim_split, pcb_acts_trim_split},
	{"split", pcb_act_trim_split, pcb_acth_trim_split, pcb_acts_trim_split},
	{"pcbsplit", pcb_act_split_idp, pcb_acth_split_idp, pcb_acts_split_idp},
	{"constraint", pcb_act_constraint, pcb_acth_constraint, pcb_acts_constraint},
	{"perp", pcb_act_perp_paral, pcb_acth_perp_paral, pcb_acts_perp_paral},
	{"paral", pcb_act_perp_paral, pcb_acth_perp_paral, pcb_acts_perp_paral},
	{"tang", pcb_act_tang, pcb_acth_tang, pcb_acts_tang},
	{"ddraft", pcb_act_ddraft, pcb_acth_ddraft, pcb_acts_ddraft}
};

/* XPM */
static const char *ddraft_xpm[] = {
"21 21 3 1",
" 	c None",
".	c #000000",
"+	c #6EA5D7",
"                     ",
"                     ",
"             .       ",
"            .        ",
"           .+        ",
"          .  +       ",
"         .    +      ",
"        .     +      ",
"       .       +     ",
"      .        +     ",
"     .++++++++++     ",
"                     ",
"      ...  ....      ",
"     .   . .   .     ",
"         . .    .    ",
"        .  .    .    ",
"       .   .    .    ",
"      .    .    .    ",
"     .     .   .     ",
"     ..... ....      ",
"                     "};

static rnd_tool_t tool_ddraft = {
	"ddraft", "2 dimensional drafting",
	NULL, 1000, ddraft_xpm, RND_TOOL_CURSOR_NAMED(NULL), 1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ddraft_tool_draw_attached,
	NULL,
	NULL,
	NULL, /* escape */
	rnd_false
};


static void mode_confchg(rnd_conf_native_t *cfg, int arr_idx)
{
	static int ddraft_tool_selected = 0;

	if (rnd_conf.editor.mode == pcb_ddraft_tool) {
		if (!ddraft_tool_selected) {
			ddraft_tool_selected = 1;
			rnd_cli_enter("ddraft", "ddraft");
			rnd_gui->open_command(rnd_gui);
		}
	}
	else if (ddraft_tool_selected) {
		rnd_cli_leave();
		ddraft_tool_selected = 0;
	}
}

int pplg_check_ver_ddraft(int ver_needed) { return 0; }

void pplg_uninit_ddraft(void)
{
	rnd_conf_hid_unreg(ddraft_cookie);
	rnd_event_unbind_allcookie(ddraft_cookie);
	rnd_remove_actions_by_cookie(ddraft_cookie);
	rnd_tool_unreg_by_cookie(ddraft_cookie);
}

static const rnd_conf_hid_callbacks_t conf_cbs_adl = { NULL, cons_gui_confchg, NULL, NULL };
static const rnd_conf_hid_callbacks_t conf_cbs_mode = { NULL, mode_confchg, NULL, NULL };

int pplg_init_ddraft(void)
{
	rnd_conf_native_t *cn;
	rnd_conf_hid_id_t confid;

	RND_API_CHK_VER;

	RND_REGISTER_ACTIONS(ddraft_action_list, ddraft_cookie)
	rnd_event_bind(PCB_EVENT_DRAW_CROSSHAIR_CHATT, cnst_enforce, NULL, ddraft_cookie);

	pcb_ddraft_tool = rnd_tool_reg(&tool_ddraft, ddraft_cookie);

	confid = rnd_conf_hid_reg(ddraft_cookie, NULL);
	cn = rnd_conf_get_field("editor/all_direction_lines");
	rnd_conf_hid_set_cb(cn, confid, &conf_cbs_adl);
	cn = rnd_conf_get_field("editor/mode");
	rnd_conf_hid_set_cb(cn, confid, &conf_cbs_mode);
	return 0;
}
