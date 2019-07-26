/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin: trim objects
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

#include "hid_dad.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */

	/* widget IDs */
	int alldir;
	int line_angle, move_angle;
	int line_length, move_length;
	int line_angle_mod, move_angle_mod;
	int line_length_mod, move_length_mod;

	int inhibit_confchg;
} cnstgui_ctx_t;

cnstgui_ctx_t cnstgui_ctx;

static void cnstgui_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	cnstgui_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(cnstgui_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}


#define c2g_array(name, fmt) \
do { \
	int n, l, len = sizeof(buff)-1; \
	char *end = buff; \
	*end = 0; \
	for(n = 0; n < cons.name ## _len; n++) { \
		if (n > 0) { \
			*end++ = ' '; \
			len--; \
		} \
		l = pcb_snprintf(end, len, fmt, cons.name[n]); \
		len -= l; \
		end += l; \
	} \
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.name, str, pcb_strdup(buff)); \
} while(0)

#define c2g_float(name) \
do { \
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.name, dbl, cons.name); \
} while(0)

#define c2g_coord(name) \
do { \
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.name, crd, cons.name); \
} while(0)

/* copy all cons fields into the GUI struct */
static void cons_changed(void)
{
	char buff[1024];

	c2g_array(line_angle, "%f");
	c2g_float(line_angle_mod);
	c2g_array(line_length, "%.08$$mH");
	c2g_coord(line_length_mod);

	c2g_array(move_angle, "%f");
	c2g_float(move_angle_mod);
	c2g_array(move_length, "%.08$$mH");
	c2g_coord(move_length_mod);
}

#define g2c_array(name, conv) \
do { \
	const char *inp = cnstgui_ctx.dlg[cnstgui_ctx.name].val.str; \
	char *curr, *next; \
	cons.name ## _len = 0; \
	if (inp == NULL) break; \
	strncpy(buff, inp, sizeof(buff)); \
	for(curr = buff; curr != NULL; curr = next) { \
		while(isspace(*curr)) curr++; \
		if (*curr == '\0') break; \
		next = strpbrk(curr, ",; "); \
		if (next != NULL) { \
			*next = '\0'; \
			next++; \
			while(isspace(*next)) next++;  \
		} \
		cons.name[cons.name ## _len++] = conv; \
	} \
} while(0)

#define g2c_scalar(name, valty) \
do { \
	cons.name = cnstgui_ctx.dlg[cnstgui_ctx.name].val.valty; \
} while(0)


/* copy all GUI fields into the cons struct */
static void gui2cons(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	char *end, buff[1024];
	pcb_bool succ;

	g2c_array(line_angle, strtod(curr, &end));
	g2c_scalar(line_angle_mod, dbl);
	g2c_array(line_length, pcb_get_value(curr, NULL, NULL, &succ));
	g2c_scalar(line_length_mod, crd);

	g2c_array(move_angle, strtod(curr, &end));
	g2c_scalar(move_angle_mod, dbl);
	g2c_array(move_length, pcb_get_value(curr, NULL, NULL, &succ));
	g2c_scalar(move_length_mod, crd);
}

static void reset_line(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.line_angle, str, pcb_strdup(""));
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.line_angle_mod, dbl, 0);
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.line_length, str, pcb_strdup(""));
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.line_length_mod, crd, 0);
	gui2cons(hid_ctx, caller_data, attr);
}

static void reset_move(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.move_angle, str, pcb_strdup(""));
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.move_angle_mod, dbl, 0);
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.move_length, str, pcb_strdup(""));
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.move_length_mod, crd, 0);
	gui2cons(hid_ctx, caller_data, attr);
}

static void set_paral(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_actionl("paral", NULL);
}

static void set_perp(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_actionl("perp", NULL);
}


static void set_tang(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_actionl("tang", NULL);
}

void cons_gui_confchg(conf_native_t *cfg, int arr_idx)
{
	if (!cnstgui_ctx.active || cnstgui_ctx.inhibit_confchg)
		return;

	cnstgui_ctx.inhibit_confchg++;
	PCB_DAD_SET_VALUE(cnstgui_ctx.dlg_hid_ctx, cnstgui_ctx.alldir, lng, conf_core.editor.all_direction_lines);
	cnstgui_ctx.inhibit_confchg--;
}

static void set_alldir(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	const char *nv;
	if (cnstgui_ctx.inhibit_confchg)
		return;

	cnstgui_ctx.inhibit_confchg++;
	nv = (cnstgui_ctx.dlg[cnstgui_ctx.alldir].val.lng) ? "1" : "0";
	conf_set(CFR_DESIGN, "editor/all_direction_lines", -1, nv, POL_OVERWRITE);
	cnstgui_ctx.inhibit_confchg--;
}



int constraint_gui(void)
{
	const char *tab_names[] = {"line", "move", NULL};
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	if (cnstgui_ctx.active)
		return 0; /* do not open another */

	PCB_DAD_BEGIN_VBOX(cnstgui_ctx.dlg);
		PCB_DAD_COMPFLAG(cnstgui_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABBED(cnstgui_ctx.dlg, tab_names);

			/* line */
			PCB_DAD_BEGIN_VBOX(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_LABEL(cnstgui_ctx.dlg, "All direction lines:");
					PCB_DAD_BOOL(cnstgui_ctx.dlg, "");
						cnstgui_ctx.alldir = PCB_DAD_CURRENT(cnstgui_ctx.dlg);
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, set_alldir);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "Constraints work well only when\nthe all-direction-lines is enabled");
				PCB_DAD_END(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_LABEL(cnstgui_ctx.dlg, "Fixed angles:");
					PCB_DAD_STRING(cnstgui_ctx.dlg);
						cnstgui_ctx.line_angle = PCB_DAD_CURRENT(cnstgui_ctx.dlg);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "space separated list of valid line angles in degree");
					PCB_DAD_BUTTON(cnstgui_ctx.dlg, "apply");
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, gui2cons);
				PCB_DAD_END(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_LABEL(cnstgui_ctx.dlg, "Angle modulo:");
					PCB_DAD_REAL(cnstgui_ctx.dlg, "");
						cnstgui_ctx.line_angle_mod = PCB_DAD_CURRENT(cnstgui_ctx.dlg);
						PCB_DAD_MINVAL(cnstgui_ctx.dlg, 0);
						PCB_DAD_MAXVAL(cnstgui_ctx.dlg, 180);
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, gui2cons);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "if non-zero:\nline angle must be an integer multiply of this value");
				PCB_DAD_END(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_LABEL(cnstgui_ctx.dlg, "Fixed lengths:");
					PCB_DAD_STRING(cnstgui_ctx.dlg);
						cnstgui_ctx.line_length = PCB_DAD_CURRENT(cnstgui_ctx.dlg);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "space separated list of valid line lengths");
					PCB_DAD_BUTTON(cnstgui_ctx.dlg, "apply");
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, gui2cons);
				PCB_DAD_END(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_LABEL(cnstgui_ctx.dlg, "Length modulo:");
					PCB_DAD_COORD(cnstgui_ctx.dlg, "");
						cnstgui_ctx.line_length_mod = PCB_DAD_CURRENT(cnstgui_ctx.dlg);
						PCB_DAD_MINVAL(cnstgui_ctx.dlg, 0);
						PCB_DAD_MAXVAL(cnstgui_ctx.dlg, PCB_MM_TO_COORD(1000));
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, gui2cons);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "if non-zero:\nline length must be an integer multiply of this value");
				PCB_DAD_END(cnstgui_ctx.dlg);

				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_BUTTON(cnstgui_ctx.dlg, "perpendicular to line");
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, set_perp);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "sets the angle constraint perpendicular to a line\nselected on the drawing");
					PCB_DAD_BUTTON(cnstgui_ctx.dlg, "parallel with line");
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, set_paral);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "sets the angle constraint parallel to a line\nselected on the drawing");
				PCB_DAD_END(cnstgui_ctx.dlg);

				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_BUTTON(cnstgui_ctx.dlg, "tangential to arc");
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, set_tang);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "sets the angle constraint to be tangential to the circle of an arc\nselected on the drawing");
				PCB_DAD_END(cnstgui_ctx.dlg);

				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_BUTTON(cnstgui_ctx.dlg, "Reset");
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, reset_line);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "clear line constraint values");
				PCB_DAD_END(cnstgui_ctx.dlg);

			PCB_DAD_END(cnstgui_ctx.dlg);

			/* move */
			PCB_DAD_BEGIN_VBOX(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_LABEL(cnstgui_ctx.dlg, "Fixed angles:");
					PCB_DAD_STRING(cnstgui_ctx.dlg);
						cnstgui_ctx.move_angle = PCB_DAD_CURRENT(cnstgui_ctx.dlg);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "space separated list of valid move vector angles in degree");
					PCB_DAD_BUTTON(cnstgui_ctx.dlg, "apply");
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, gui2cons);
				PCB_DAD_END(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_LABEL(cnstgui_ctx.dlg, "Angle modulo:");
					PCB_DAD_REAL(cnstgui_ctx.dlg, "");
						cnstgui_ctx.move_angle_mod = PCB_DAD_CURRENT(cnstgui_ctx.dlg);
						PCB_DAD_MINVAL(cnstgui_ctx.dlg, 0);
						PCB_DAD_MAXVAL(cnstgui_ctx.dlg, 180);
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, gui2cons);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "if non-zero:\nmove vector angle must be an integer multiply of this value");
				PCB_DAD_END(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_LABEL(cnstgui_ctx.dlg, "Fixed lengths:");
					PCB_DAD_STRING(cnstgui_ctx.dlg);
						cnstgui_ctx.move_length = PCB_DAD_CURRENT(cnstgui_ctx.dlg);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "space separated list of valid move distances");
					PCB_DAD_BUTTON(cnstgui_ctx.dlg, "apply");
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, gui2cons);
				PCB_DAD_END(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_LABEL(cnstgui_ctx.dlg, "Length modulo:");
					PCB_DAD_COORD(cnstgui_ctx.dlg, "");
						cnstgui_ctx.move_length_mod = PCB_DAD_CURRENT(cnstgui_ctx.dlg);
						PCB_DAD_MINVAL(cnstgui_ctx.dlg, 0);
						PCB_DAD_MAXVAL(cnstgui_ctx.dlg, PCB_MM_TO_COORD(1000));
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, gui2cons);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "if non-zero:\nmove distance must be an integer multiply of this value");
				PCB_DAD_END(cnstgui_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
					PCB_DAD_BUTTON(cnstgui_ctx.dlg, "Reset");
						PCB_DAD_CHANGE_CB(cnstgui_ctx.dlg, reset_move);
						PCB_DAD_HELP(cnstgui_ctx.dlg, "clear move constraint values");
				PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_END(cnstgui_ctx.dlg);

		PCB_DAD_END(cnstgui_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(cnstgui_ctx.dlg, clbtn);
	PCB_DAD_END(cnstgui_ctx.dlg);

	/* set up the context */
	cnstgui_ctx.active = 1;

	PCB_DAD_NEW("constraint", cnstgui_ctx.dlg, "Drawing constraints", &cnstgui_ctx, pcb_false, cnstgui_close_cb);

	cons_changed();

	return 0;
}
