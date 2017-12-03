/*
 *				COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Contact addresses for paper mail and Email:
 *	Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *	Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <genvector/gds_char.h>

#include "read_dru.h"

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

int io_eagle_test_parse_pcb_dru(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f)
{
	return pcb_eagle_dru_test_parse(f);
}

int io_eagle_read_pcb_dru(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	FILE *f;
	char *efn;
	gds_t buff;
	pcb_layergrp_id_t gid;
	int n, num_layers = 0;

	f = pcb_fopen_fn(Filename, "r", &efn);
	if (f == NULL)
		return -1;


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
		pcb_trace("DRU '%s'='%s'\n", k, v);
	}

	/* set up layers */
	pcb_layer_group_setup_default(&pcb->LayerGroups);
	if (pcb_layergrp_list(pcb, PCB_LYT_COPPER | PCB_LYT_TOP, &gid, 1))
		pcb_layer_create(pcb, gid, "top_copper");
	if (pcb_layergrp_list(pcb, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &gid, 1))
		pcb_layer_create(pcb, gid, "bottom_copper");
	num_layers--;
	for(n = 0; n < num_layers; n++) {
		char buff[32];
		pcb_layergrp_t *grp = pcb_get_grp_new_intern(pcb, -1);
		sprintf(buff, "signal_%d", n);
		pcb_layer_create(pcb, grp - pcb->LayerGroups.grp, buff);
	}
	pcb_layer_group_setup_silks(pcb);

	fclose(f);
}


#endif
