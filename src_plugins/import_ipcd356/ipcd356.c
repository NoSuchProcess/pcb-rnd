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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <genht/htsp.h>
#include <genht/hash.h>

#include "board.h"
#include "data.h"
#include "safe_fs.h"

#include "hid.h"
#include "action_helper.h"
#include "compat_misc.h"
#include "hid_actions.h"
#include "plugins.h"

static const char *ipcd356_cookie = "ipcd356 importer";

static void extract_field(char *dst, const char *line, int start, int end)
{
	int n;
	line += start;
	for(n = end-start; n > 0; n--) {
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
	if (!is_mil)
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

static int ipc356_parse(pcb_board_t *pcb, FILE *f, const char *fn, htsp_t *subcs)
{
	char line_[128], *line, netname[16], refdes[8], term[8];
	int lineno = 0, is_mil = 1, is_rad = 0;
	test_feature_t tf;
	pcb_any_obj_t *sc = NULL, *o;

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
						pcb_message(PCB_MSG_ERROR, "Unimplemented unit in %s:%d - requested radians in rotation, the code doesn't handle that\n", fn, lineno);
						return 1;
					}
				}
				break;
			case '3': /* test feature */
				if (line[2] != '7') {
					pcb_message(PCB_MSG_WARNING, "Ignoring unknown test feautre '3%c%c' in %s:%d' - does not end in 7\n", line[1], line[2], fn, lineno);
					break;
				}
				if (line[26] != '-') {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - missing dash between refdes and pin number\n", fn, lineno);
					break;
				}
				extract_field(netname, line, 3, 16);
				extract_field(refdes,  line, 20, 25);
				extract_field(term,    line, 27, 30);
				tf.is_via = (strcmp(refdes, "VIA") == 0);
				tf.is_middle = (line[31] == 'M');
				if (line[32] == 'D') {
					if (extract_dim(&tf.hole, line, 33, 36, 0, is_mil)) {
						pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - invalid hole dimension\n", fn, lineno);
						break;
					}
					if (line[37] == 'P')
						tf.is_plated = 1;
					else if (line[37] == 'U')
						tf.is_plated = 0;
					else {
						pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - hole is neither plated nor unplated\n", fn, lineno);
						break;
					}
				}
				else {
					tf.hole = 0;
					tf.is_plated = 0;
				}
				if (line[38] != 'A') {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - missing 'A' for access\n", fn, lineno);
					break;
				}
				if (extract_int(&tf.side, line, 40, 41)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - invalid hole dimension\n", fn, lineno);
					break;
				}
				if (line[41] != 'X') {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - missing 'X'\n", fn, lineno);
					break;
				}
				if (extract_dim(&tf.cx, line, 42, 48, 1, is_mil)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - invalid X dimension\n", fn, lineno);
					break;
				}
				if (line[49] != 'Y') {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - missing 'Y'\n", fn, lineno);
					break;
				}
				if (extract_dim(&tf.cy, line, 50, 56, 1, is_mil)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - invalid Y dimension\n", fn, lineno);
					break;
				}
				if (line[57] != 'X') {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - missing width 'X'\n", fn, lineno);
					break;
				}
				if (extract_dim(&tf.width, line, 58, 61, 0, is_mil)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - invalid width dimension\n", fn, lineno);
					break;
				}
				if (line[62] != 'Y') {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - missing height 'Y'\n", fn, lineno);
					break;
				}
				if (extract_dim(&tf.height, line, 63, 66, 0, is_mil)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - invalid height 'Y' dimension\n", fn, lineno);
					break;
				}
				if (line[67] != 'R') {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - missing rotation 'R'\n", fn, lineno);
					break;
				}
				if (extract_int(&tf.rot, line, 68, 70)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - invalid height 'Y' dimension\n", fn, lineno);
					break;
				}
				if (line[72] != 'S') {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - solder mask marker 'S'\n", fn, lineno);
					break;
				}
				if (extract_int(&tf.mask, line, 73, 73)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - invalid height 'Y' dimension\n", fn, lineno);
					break;
				}
				tf.is_tooling = 0;

				switch(line[1]) {
					case '3': /* tooling feature/hole */
					case '4': /* tooling hole only */
						tf.is_tooling = 1;
					case '1': /* through hole */
						if (tf.hole == 0) {
							pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - thru-hole feature without hole dia\n", fn, lineno);
							break;
						}
						break;
					case '2': /* SMT feature */
						if (tf.hole > 0) {
							pcb_message(PCB_MSG_WARNING, "Ignoring invalid test feautre in %s:%d' - SMD feature with hole dia\n", fn, lineno);
							break;
						}
						break;
					default:
						pcb_message(PCB_MSG_WARNING, "Ignoring unknown test feautre '3%c%c' in %s:%d - unknown second digit'\n", line[1], line[2], fn, lineno);
				}

				if (subcs != NULL) {
					sc = htsp_get(subcs, refdes);
					if (sc == NULL) {
						
					}
				}

				break;
			case '9': /* EOF */
				if ((line[1] == '9') && (line[2] == '9'))
					return 0;
				pcb_message(PCB_MSG_ERROR, "Invalid end-of-file marker in %s:%d - expected '999'\n", fn, lineno);
				return 1;
		}
	}

	pcb_message(PCB_MSG_ERROR, "Unexpected end of file - expected '999'\n");
	return 1;
}

static const char pcb_acts_LoadIpc356From[] = "LoadIpc356From(filename)";
static const char pcb_acth_LoadIpc356From[] = "Loads the specified IPC356-D netlist";
int pcb_act_LoadIpc356From(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	FILE *f;
	int res;

	f = pcb_fopen(argv[0], "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't open %s for read\n", argv[0]);
		return 1;
	}
	res = ipc356_parse(PCB, f, argv[0], NULL);
	fclose(f);
	return res;
}

pcb_hid_action_t import_ipcd356_action_list[] = {
	{"LoadIpc356From", 0, pcb_act_LoadIpc356From, pcb_acth_LoadIpc356From, pcb_acts_LoadIpc356From}
};
PCB_REGISTER_ACTIONS(import_ipcd356_action_list, ipcd356_cookie)

int pplg_check_ver_import_ipcd356(int ver_needed) { return 0; }

void pplg_uninit_import_ipcd356(void)
{
	pcb_hid_remove_actions_by_cookie(ipcd356_cookie);
}

#include "dolists.h"
int pplg_init_import_ipcd356(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(import_ipcd356_action_list, ipcd356_cookie);
	return 0;
}
