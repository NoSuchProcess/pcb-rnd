/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "data.h"
#include "error.h"

#include "layer_addr.h"

static int parse_layer_type(char *type, pcb_layer_type_t *lyt, int *offs, int *has_offs)
{
	char *soffs, *end, *nxt, *cur;
	pcb_layer_type_t l;

	*lyt = 0;
	*offs = *has_offs = 0;

	soffs = strchr(type, ':');
	if (soffs != NULL) {
		*soffs = '\0';
		*offs = strtol(soffs+1, &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "CAM rule: invalid offset '%s'\n", soffs);
			return 1;
		}
		*has_offs = 1;
	}

	for(cur = type; cur != NULL; cur = nxt) {
		nxt = strchr(cur, '-');
		if (nxt != NULL) {
			*nxt = '\0';
			nxt++;
		}
		l = pcb_layer_type_str2bit(cur);
		if (l == 0) {
			pcb_message(PCB_MSG_ERROR, "CAM rule: invalid layer type '%s'\n", cur);
			return 1;
		}
		(*lyt) |= l;
	}
	return 0;
}

void pcb_parse_layer_supplements(char **spk, char **spv, int spc,   char **purpose, pcb_xform_t **xf, pcb_xform_t *xf_)
{
	int n;

	*purpose = NULL;
	memset(xf_, 0, sizeof(pcb_xform_t));

	for(n = 0; n < spc; n++) {
		char *key = spk[n], *val = spv[n];

		if (strcmp(key, "purpose") == 0)
			*purpose = val;
		else if (strcmp(key, "bloat") == 0) {
			pcb_bool succ;
			double v = pcb_get_value(val, NULL, NULL, &succ);
			if (succ) {
				xf_->bloat = v;
				*xf = xf_;
			}
			else
				pcb_message(PCB_MSG_ERROR, "CAM: ignoring invalid layer supplement value '%s' for bloat\n", val);
		}
		else if (strcmp(key, "partial") == 0) {
			xf_->partial_export = 1;
			*xf = xf_;
		}
		else if (strcmp(key, "faded") == 0) {
			xf_->layer_faded = 1;
			*xf = xf_;
		}
		else
			pcb_message(PCB_MSG_ERROR, "CAM: ignoring unknown layer supplement key '%s'\n", key);
	}
}


int pcb_layergrp_list_by_addr(pcb_board_t *pcb, char *curr, pcb_layergrp_id_t gids[PCB_MAX_LAYERGRP], char **spk, char **spv, int spc, int *vid, pcb_xform_t **xf, pcb_xform_t *xf_in, const char *err_prefix)
{
	pcb_layergrp_id_t gid, lgids[PCB_MAX_LAYERGRP];
	int gids_max = PCB_MAX_LAYERGRP;
	char *purpose;

	if (vid != NULL)
		*vid = -1;

	if (*curr == '@') {
		/* named layer group */
		curr++;
		gid = pcb_layergrp_by_name(pcb, curr);
		if (gid < 0) {
			if (vid != NULL) {
				const pcb_virt_layer_t *v;
				int n;
				pcb_parse_layer_supplements(spk, spv, spc, &purpose, xf, xf_in);
				for(n = 0, v = pcb_virt_layers; v->name != NULL; n++,v++) {
					if (strcmp(v->name, curr) == 0) {
						*vid = n;
						return 0;
					}
				}
			}
			return 0;
		}
		if (gid < 0) {
			if (err_prefix != NULL)
				pcb_message(PCB_MSG_ERROR, "%sno such layer group '%s'\n", curr, err_prefix);
			return -1;
		}
		if (pcb->LayerGroups.grp[gid].len <= 0)
			return 0;
		pcb_parse_layer_supplements(spk, spv, spc, &purpose, xf, xf_in);
		gids[0] = gid;
		return 1;
	}
	else {
		/* by layer type */
		int offs, has_offs;
		pcb_layer_type_t lyt;
		const pcb_virt_layer_t *vl;

		if (parse_layer_type(curr, &lyt, &offs, &has_offs) != 0)
			return -1;

		pcb_parse_layer_supplements(spk, spv, spc, &purpose, xf, xf_in);

		vl = pcb_vlayer_get_first(lyt, purpose, -1);
		if ((lyt & PCB_LYT_VIRTUAL) && (vl == NULL)) {
			if (err_prefix != NULL)
				pcb_message(PCB_MSG_ERROR, "%sno virtual layer with purpose '%s'\n", err_prefix, purpose);
			return -1;
		}
		if (vl == NULL) {
			if (has_offs) {
				int len = pcb_layergrp_listp(pcb, lyt, lgids, sizeof(lgids)/sizeof(lgids[0]), -1, purpose);
				if (offs < 0)
					offs = len + offs;
				else
					offs--;
				if ((offs >= 0) && (offs < len)) {
					gids[0] = lgids[offs];
					return 1;
				}
			}
			else {
				return pcb_layergrp_listp(pcb, lyt, gids, gids_max, -1, purpose);
			}
		}
		else {
			if (vid != NULL)
				*vid = vl->new_id - PCB_LYT_VIRTUAL - 1;
		}
	}
	return 0;
}

pcb_layer_id_t pcb_layer_str2id(pcb_data_t *data, const char *str)
{
	char *end;
	pcb_layer_id_t id;
	if (*str == '#') {
		id = strtol(str+1, &end, 10);
		if ((*end == '\0') && (id >= 0) && (id < data->LayerN))
			return id;
	}
TODO("layer: do the same that cam does; test with propedit");
	return -1;
}

pcb_layergrp_id_t pcb_layergrp_str2id(pcb_board_t *pcb, const char *str)
{
	char *end;
	pcb_layer_id_t id;
	if (*str == '#') {
		id = strtol(str+1, &end, 10);
		if ((*end == '\0') && (id >= 0) && (id < pcb->LayerGroups.len))
			return id;
	}
TODO("layer: do the same that cam does; test with propedit");
	return -1;
}
