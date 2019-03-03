/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  ipc-d-356 import plugin
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

#include <genht/htsp.h>
#include <genht/hash.h>

#include "board.h"
#include "data.h"
#include "safe_fs.h"

#include "conf_core.h"
#include "hid.h"
#include "compat_misc.h"
#include "actions.h"
#include "plugins.h"
#include "rtree.h"

#include "../src_plugins/lib_compat_help/pstk_help.h"

static const char *ipcd356_cookie = "ipcd356 importer";

static void set_src(pcb_attribute_list_t *a, const char *fn, long lineno)
{
	char src[8192];
	pcb_snprintf(src, sizeof(src), "ipcd356::%s:%ld", fn, lineno);
	pcb_attribute_put(a, "source", src);
}

static int netname_valid(const char *netname)
{
	if (*netname == '\0') return 0;
	if (strcmp(netname, "N/C") == 0) return 0;
	return 1;
}

static void extract_field(char *dst, const char *line, int start, int end)
{
	int n;
	line += start;
	for(n = end-start; n >= 0; n--) {
		if (*line == ' ')
			break;
		*dst++ = *line++;
	}
	*dst = '\0';
}

static int extract_dim(pcb_coord_t *dst, const char *line, int start, int end, int has_sign, int is_mil)
{
	char tmp[16];
	const char *sign = line + start;
	double d;
	pcb_bool succ;

	if (has_sign) {
		if ((*sign != '+') && (*sign != '-') && (*sign != ' ')) {
			pcb_message(PCB_MSG_WARNING, "Invalid coordinate sign: '%c':\n", *line);
			return -1;
		}
	}
	extract_field(tmp, line, start, end);
	d = pcb_get_value(tmp, is_mil ? "mil" : "mm", NULL, &succ);
	if (!succ)
		return -1;
	if (is_mil)
		d = d / 10.0;
	else
		d = d / 1000.0;
	*dst = pcb_round(d);
	return 0;
}

static int extract_int(int *dst, const char *line, int start, int end)
{
	char tmp[16], *ends;

	extract_field(tmp, line, start, end);
	*dst = strtol(tmp, &ends, 10);
	if (*ends != '\0')
		return -1;
	return 0;
}

typedef struct {
	int is_via, is_middle, is_plated, side, rot, is_tooling, mask;
	pcb_coord_t hole, width, height, cx, cy;
} test_feature_t;

static int parse_feature(char *line, test_feature_t *tf, const char *fn, long lineno, int is_mil, int is_rad, char *netname, char *refdes, char *term)
{
	if (line[2] != '7') {
		pcb_message(PCB_MSG_WARNING, "Ignoring unknown test feautre '3%c%c' in %s:%ld' - does not end in 7\n", line[1], line[2], fn, lineno);
		return -1;
	}
	if (line[26] != '-') {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - missing dash between refdes and pin number\n", fn, lineno);
		return -1;
	}
	extract_field(netname, line, 3, 16);
	extract_field(refdes,  line, 20, 25);
	extract_field(term,    line, 27, 30);
	tf->is_via = (strcmp(refdes, "VIA") == 0);
	tf->is_middle = (line[31] == 'M');
	if (line[32] == 'D') {
		if (extract_dim(&tf->hole, line, 33, 36, 0, is_mil)) {
			pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - invalid hole dimension\n", fn, lineno);
			return -1;
		}
		if (line[37] == 'P')
			tf->is_plated = 1;
		else if (line[37] == 'U')
			tf->is_plated = 0;
		else {
			pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - hole is neither plated nor unplated\n", fn, lineno);
			return -1;
		}
	}
	else {
		tf->hole = 0;
		tf->is_plated = 0;
	}
	if (line[38] != 'A') {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - missing 'A' for access\n", fn, lineno);
		return -1;
	}
	if (extract_int(&tf->side, line, 39, 40)) {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - invalid access side\n", fn, lineno);
		return -1;
	}
	if (line[41] != 'X') {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - missing 'X'\n", fn, lineno);
		return -1;
	}
	if (extract_dim(&tf->cx, line, 42, 48, 1, is_mil)) {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - invalid X dimension\n", fn, lineno);
		return -1;
	}
	if (line[49] != 'Y') {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - missing 'Y'\n", fn, lineno);
		return -1;
	}
	if (extract_dim(&tf->cy, line, 50, 56, 1, is_mil)) {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - invalid Y dimension\n", fn, lineno);
		return -1;
	}
	if (line[57] != 'X') {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - missing width 'X'\n", fn, lineno);
		return -1;
	}
	if (extract_dim(&tf->width, line, 58, 61, 0, is_mil)) {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - invalid width dimension\n", fn, lineno);
		return -1;
	}
	if (line[62] != 'Y') {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - missing height 'Y'\n", fn, lineno);
		return -1;
	}
	if (extract_dim(&tf->height, line, 63, 66, 0, is_mil)) {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - invalid height 'Y' dimension\n", fn, lineno);
		return -1;
	}
	if (line[67] != 'R') {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - missing rotation 'R'\n", fn, lineno);
		return -1;
	}
	if (extract_int(&tf->rot, line, 68, 70)) {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - invalid height 'Y' dimension\n", fn, lineno);
		return -1;
	}
	if (line[72] != 'S') {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - solder mask marker 'S'\n", fn, lineno);
		return -1;
	}
	if (extract_int(&tf->mask, line, 73, 73)) {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - invalid height 'Y' dimension\n", fn, lineno);
		return -1;
	}
	tf->is_tooling = 0;

	switch(line[1]) {
		case '3': /* tooling feature/hole */
		case '4': /* tooling hole only */
			tf->is_tooling = 1;
		case '1': /* through hole */
			if (tf->hole == 0) {
				pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - thru-hole feature without hole dia\n", fn, lineno);
				return -1;
			}
			break;
		case '2': /* SMT feature */
			if (tf->hole > 0) {
				pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%ld' - SMD feature with hole dia\n", fn, lineno);
				break;
			}
			break;
		default:
			pcb_message(PCB_MSG_WARNING, "Ignoring unknown test feautre '3%c%c' in %s:%ld - unknown second digit'\n", line[1], line[2], fn, lineno);
	}
	return 0;
}

static void create_feature(pcb_board_t *pcb, pcb_data_t *data, test_feature_t *tf, const char *fn, long lineno, const char *term)
{
	pcb_pstk_t *ps;
	pcb_pstk_shape_t sh[6];
	pcb_coord_t msk = PCB_MIL_TO_COORD(4), y;
	int i = 0, thru = (tf->hole > 0) && (tf->is_plated);

	memset(sh, 0, sizeof(sh));
	if (tf->height == 0) {
		if ((tf->side == 0) || (tf->side == 1) || thru) {
			sh[i].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER; pcb_shape_oval(&sh[i], tf->width, tf->width); i++;
		}
		if ((tf->side == 0) || (tf->side == 2) || thru) {
			sh[i].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; pcb_shape_oval(&sh[i], tf->width, tf->width); i++;
		}
		if (thru) {
			sh[i].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; pcb_shape_oval(&sh[i], tf->width, tf->width); i++;
		}
		if ((tf->mask == 0) || (tf->mask == 1)) {
			sh[i].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK; sh[i].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_oval(&sh[i], tf->width + msk, tf->width + msk); i++;
		}
		if ((tf->mask == 0) || (tf->mask == 2)) {
			sh[i].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK; sh[i].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_oval(&sh[i], tf->width + msk, tf->width + msk); i++;
		}
	}
	else {
		if ((tf->side == 0) || (tf->side == 1) || thru) {
			sh[i].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER; pcb_shape_rect(&sh[i], tf->width, tf->height); i++;
		}
		if ((tf->side == 0) || (tf->side == 2) || thru) {
			sh[i].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; pcb_shape_rect(&sh[i], tf->width, tf->height); i++;
		}
		if (thru) {
			sh[i].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; pcb_shape_rect(&sh[i], tf->width, tf->height); i++;
		}
		if ((tf->mask == 0) || (tf->mask == 1)) {
			sh[i].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK; sh[i].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_rect(&sh[i], tf->width + msk, tf->height + msk); i++;
		}
		if ((tf->mask == 0) || (tf->mask == 2)) {
			sh[i].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK; sh[i].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_rect(&sh[i], tf->width + msk, tf->height + msk); i++;
		}
	}

	y = pcb->MaxHeight - tf->cy;
	if ((y < 0) || (y > pcb->MaxHeight) || (tf->cx < 0) || (tf->cx > pcb->MaxWidth))
		pcb_message(PCB_MSG_WARNING, "Test feature ended up out of the board extents in %s:%ld - board too small please use autocrop()\n", fn, lineno);
	ps = pcb_pstk_new_from_shape(data, tf->cx, y, tf->hole, tf->is_plated, conf_core.design.bloat, sh);

	if (tf->is_middle)
		pcb_attribute_put(&ps->Attributes, "ipcd356::mid", "yes");
	if (tf->is_tooling)
		pcb_attribute_put(&ps->Attributes, "ipcd356::tooling", "yes");
	if (term)
		pcb_attribute_put(&ps->Attributes, "term", term);
}

static int ipc356_parse(pcb_board_t *pcb, FILE *f, const char *fn, htsp_t *subcs, int want_net, int want_pads)
{
	char line_[128], *line, netname[16], refdes[8], term[8];
	long lineno = 0;
	int is_mil = 1, is_rad = 0;
	test_feature_t tf;
	pcb_subc_t *sc;
	pcb_data_t *data = pcb->Data;

	while((line = fgets(line_, sizeof(line_), f)) != NULL) {
		lineno++;
		switch(*line) {
			case '\r':
			case '\n':
			case 'C': /* comment */
				break;
			case 'P': /* parameter */
				if (strncmp(line+3, "UNITS CUST", 10) == 0) {
					switch(line[14]) {
						case '0': is_mil = 1; is_rad = 0; break;
						case '1': is_mil = 0; is_rad = 0; break;
						case '2': is_mil = 1; is_rad = 1; break;
					}
					if (is_rad) {
						pcb_message(PCB_MSG_ERROR, "Unimplemented unit in %s:%ld - requested radians in rotation, the code doesn't handle that\n", fn, lineno);
						return 1;
					}
				}
				break;
			case '3': /* test feature */
				if (parse_feature(line, &tf, fn, lineno, is_mil, is_rad, netname, refdes, term) != 0)
					return 1;

				if (subcs != NULL) {
					sc = htsp_get(subcs, refdes);

					if (sc == NULL) {
						const char *nr;
						sc = pcb_subc_alloc();
						pcb_attribute_put(&sc->Attributes, "refdes", refdes);
						set_src(&sc->Attributes, fn, lineno);
						nr = pcb_attribute_get(&sc->Attributes, "refdes");
						htsp_set(subcs, (char *)nr, sc);
						pcb_subc_reg(pcb->Data, sc);
						pcb_subc_bind_globals(pcb, sc);
					}
					data = sc->data;
				}

				if (want_pads)
					create_feature(pcb, data, &tf, fn, lineno, term);
	
				if (want_net && (netname_valid(netname))) {
					char tn[36];
					sprintf(tn, "%s-%s", refdes, term);
					pcb_actionl("Netlist", "Add",  netname, tn, NULL);
				}
				break;
			case '9': /* EOF */
				if ((line[1] == '9') && (line[2] == '9'))
					return 0;
				pcb_message(PCB_MSG_ERROR, "Invalid end-of-file marker in %s:%ld - expected '999'\n", fn, lineno);
				return 1;
		}
	}

	pcb_message(PCB_MSG_ERROR, "Unexpected end of file - expected '999'\n");
	return 1;
}

static const char pcb_acts_LoadIpc356From[] = "LoadIpc356From(filename, [nonet], [nopad], [nosubc])";
static const char pcb_acth_LoadIpc356From[] = "Loads the specified IPC356-D netlist";
fgw_error_t pcb_act_LoadIpc356From(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	FILE *f;
	static char *default_file = NULL;
	const char *fname = NULL;
	int rs, n, want_subc = 1, want_net = 1, want_pads = 1;
	htsp_t subcs, *scs = NULL;
	htsp_entry_t *e;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, LoadIpc356From, fname = argv[1].val.str);

	if ((fname == NULL) || (*fname == '\0')) {
		fname = pcb_gui->fileselect("Load IPC-D-356 netlist...",
			"Pick an IPC-D-356 netlist file.\n",
			default_file, ".net", "ipcd356", PCB_HID_FSD_READ);
		if (fname == NULL) {
			PCB_ACT_IRES(1);
			return 0;
		}
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	f = pcb_fopen(fname, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't open %s for read\n", fname);
		PCB_ACT_IRES(1);
		return 0;
	}

	for(n = 2; n < argc; n++) {
		const char *s;
		PCB_ACT_MAY_CONVARG(n, FGW_STR, LoadIpc356From, s = argv[n].val.str);
		if (strcmp(s, "nonet") == 0) want_net = 0;
		if (strcmp(s, "nopad") == 0) want_pads = 0;
		if (strcmp(s, "nosubc") == 0) want_subc = 0;
	}

	if (!want_pads)
		want_subc = 0; /* can't load subcircuits if there are no padstacks - they would be all empty */

	if (want_subc) {
		htsp_init(&subcs, strhash, strkeyeq);
		scs = &subcs;
	}

	if (want_net) {
		pcb_actionl("Netlist", "Freeze", NULL);
		pcb_actionl("Netlist", "Clear", NULL);
	}

	rs = ipc356_parse(PCB, f, fname, scs, want_net, want_pads);

	if (want_net) {
		pcb_actionl("Netlist", "Sort", NULL);
		pcb_actionl("Netlist", "Thaw", NULL);
	}

	fclose(f);

	if (want_subc) {
		for (e = htsp_first(&subcs); e; e = htsp_next(&subcs, e)) {
			pcb_subc_t *sc = (pcb_subc_t *)e->value;
			pcb_subc_reg(PCB->Data, sc);
			pcb_subc_bbox(sc);
			if (PCB->Data->subc_tree == NULL)
				PCB->Data->subc_tree = pcb_r_create_tree();
			pcb_r_insert_entry(PCB->Data->subc_tree, (pcb_box_t *)sc);
			pcb_subc_rebind(PCB, sc);
		}
		htsp_uninit(&subcs);
	}

	PCB_ACT_IRES(rs);
	return 0;
}

pcb_action_t import_ipcd356_action_list[] = {
	{"LoadIpc356From", pcb_act_LoadIpc356From, pcb_acth_LoadIpc356From, pcb_acts_LoadIpc356From}
};
PCB_REGISTER_ACTIONS(import_ipcd356_action_list, ipcd356_cookie)

int pplg_check_ver_import_ipcd356(int ver_needed) { return 0; }

void pplg_uninit_import_ipcd356(void)
{
	pcb_remove_actions_by_cookie(ipcd356_cookie);
}

#include "dolists.h"
int pplg_init_import_ipcd356(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(import_ipcd356_action_list, ipcd356_cookie);
	return 0;
}
