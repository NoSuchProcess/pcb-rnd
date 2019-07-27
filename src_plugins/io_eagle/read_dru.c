/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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
 *
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <genvector/gds_char.h>

#include "read_dru.h"
#include "conf_core.h"

int pcb_eagle_dru_test_parse(FILE *f)
{
	char buff[256], *s;
	rewind(f);
	s = fgets(buff, sizeof(buff)-1, f);
	rewind(f);
	buff[sizeof(buff)-1] = '\0';

	if (s == NULL)
		return 0;

	/* first line is a description */
	if (strncmp(s, "description", 11) != 0)
		return 0;
	s += 11;

	/* it may contain a [lang] suffix */
	if (*s == '[') {
		s = strchr(s, ']');
		if (s == NULL)
			return 0;
		s++;
	}

	/* there may be whitespace */
	while(isspace(*s)) s++;

	/* and the key/value separator is an '=' */
	if (*s != '=')
		return 0;

	return 1;
}

/* eat up whitespace after the '=' */
static void eat_up_ws(FILE *f)
{
	for(;;) {
		int c = fgetc(f);
		if (c == EOF)
			return;
		if (!isspace(c)) {
			ungetc(c, f);
			return;
		}
	}
}

void pcb_eagle_dru_parse_line(FILE *f, gds_t *buff, char **key, char **value)
{
	long n, keyo = -1, valo = -1;
	gds_truncate(buff, 0);
	*key = NULL;
	*value = NULL;
	for(;;) {
		int c = fgetc(f);
		if (c == EOF)
			break;
		if ((c == '\r') || (c == '\n')) {
			if (buff->used == 0) /* ignore leading newlines */
				continue;
			break;
		}
		if (isspace(c) && (keyo < 0)) /* ignore leading spaces */
			continue;

		/* have key, don't have val, found sep */
		if ((keyo >= 0) && (valo < 0) && (c == '=')) {
			for(n = buff->used-1; n >= 0; n--) {
				if (!isspace(buff->array[n]))
					break;
				buff->array[n] = '\0';
			}
			gds_append(buff, '\0');
			valo = buff->used;
			eat_up_ws(f);
		}
		else
			gds_append(buff, c);

		/* set key offset */
		if (keyo < 0)
			keyo = 0;
	}

	gds_append(buff, '\0');

	if (keyo >= 0)
		*key = buff->array + keyo;
	if (valo >= 0)
		*value = buff->array + valo;
}


#ifndef PCB_EAGLE_DRU_PARSER_TEST

#include "safe_fs.h"
#include "board.h"
#include "layer_grp.h"
#include "error.h"

int io_eagle_test_parse_dru(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	if (typ != PCB_IOT_PCB)
		return 0; /* support only boards because DRU can be loaded onto a board only */
	return pcb_eagle_dru_test_parse(f);
}

static void bump_up_str(const char *key, const char *val, const char *cpath, pcb_coord_t curr_val)
{
	pcb_bool succ;
	double d;

	d = pcb_get_value(val, NULL, NULL, &succ);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "Invalid coord value for key %s: '%s'\n", key, val);
		return;
	}
	if (d > curr_val)
		pcb_conf_set(CFR_DESIGN, "design/min_drill", -1, val, POL_OVERWRITE);
}

int io_eagle_read_pcb_dru(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	FILE *f;
	char *efn;
	gds_t buff;
	pcb_layergrp_id_t gid;
	int n, num_layers = 0;
	static const char prefix[] = "io_eagle::dru::";
	char tmp[256];

	f = pcb_fopen_fn(&PCB->hidlib, Filename, "r", &efn);
	if (f == NULL)
		return -1;

	pcb_conf_set(CFR_DESIGN, "design/bloat", -1, "0", POL_OVERWRITE);
	pcb_conf_set(CFR_DESIGN, "design/min_wid", -1, "0", POL_OVERWRITE);
	pcb_conf_set(CFR_DESIGN, "design/min_drill", -1, "0", POL_OVERWRITE);

	memcpy(tmp, prefix, sizeof(prefix));

	gds_init(&buff);
	while(!(feof(f))) {
		char *k, *v;
		pcb_eagle_dru_parse_line(f, &buff, &k, &v);
		if (k == NULL)
			continue;
		if (strcmp(k, "layerSetup") == 0) {
			v = strchr(v, '*');
			if (v != NULL) {
				v++;
				num_layers = atoi(v);
			}
		}
		else if (strcmp(k, "mdWireWire") == 0)
			bump_up_str(k, v, "design/bloat", conf_core.design.bloat);
		else if (strcmp(k, "mdWirePad") == 0)
			bump_up_str(k, v, "design/bloat", conf_core.design.bloat);
		else if (strcmp(k, "mdWireVia") == 0)
			bump_up_str(k, v, "design/bloat", conf_core.design.bloat);
		else if (strcmp(k, "mdPadPad") == 0)
			bump_up_str(k, v, "design/bloat", conf_core.design.bloat);
		else if (strcmp(k, "mdPadVia") == 0)
			bump_up_str(k, v, "design/bloat", conf_core.design.bloat);
		else if (strcmp(k, "msWidth") == 0)
			bump_up_str(k, v, "design/min_wid", conf_core.design.min_wid);
		else if (strcmp(k, "msDrill") == 0)
			bump_up_str(k, v, "design/min_drill", conf_core.design.min_drill);
		else {
			int len = strlen(k);
			if (len < sizeof(tmp) - sizeof(prefix)) {
				memcpy(tmp + sizeof(prefix) - 1, k, len+1);
				pcb_attribute_put(&pcb->Attributes, tmp, v);
			}
		}
	}

	/* set up layers */
	pcb_layer_group_setup_default(pcb);
	if (pcb_layergrp_list(pcb, PCB_LYT_COPPER | PCB_LYT_TOP, &gid, 1))
		pcb_layer_create(pcb, gid, "top_copper");
	if (pcb_layergrp_list(pcb, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &gid, 1))
		pcb_layer_create(pcb, gid, "bottom_copper");
	num_layers--;
	for(n = 0; n < num_layers; n++) {
		pcb_layergrp_t *grp = pcb_get_grp_new_intern(pcb, -1);
		sprintf(tmp, "signal_%d", n);
		pcb_layer_create(pcb, grp - pcb->LayerGroups.grp, tmp);
	}
	pcb_layer_group_setup_silks(pcb);

	fclose(f);
	return 0;
}


#endif
