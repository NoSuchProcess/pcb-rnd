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

#include <genvector/gds_char.h>

#include "board.h"
#include "data.h"
#include <librnd/core/error.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/misc_util.h>

#include "layer_addr.h"

char *pcb_parse_layergrp_err = "";
char *pcb_parse_layergrp_address(char *curr, char **spk, char **spv, int *spc)
{
	char *s, *lasta, *eq;
	int level = 0, trmax = *spc;
	*spc = 0;

	for(s = curr; *s != '\0'; s++) {
		switch(*s) {
			case '(':
				if (level == 0) {
					*s = '\0';
					lasta = s+1;
				}
				level++;
				break;
			case ')':
				if (level > 0)
					level--;
				if (level == 0)
					goto append;
				break;
			case ',':
				if (level == 0)
					goto out;
				append:;
				*s = '\0';
				if (*spc >= trmax)
					return pcb_parse_layergrp_err;
				lasta = pcb_str_strip(lasta);
				spk[*spc] = lasta;
				eq = strchr(lasta, '=');
				if (eq != NULL) {
					*eq = '\0';
					eq++;
				}
				spv[*spc] = eq;
				(*spc)++;
				*s = '\0';
				lasta = s+1;
		}
	}

	if (level > 0)
		return pcb_parse_layergrp_err;

	out:;

	if (*s != '\0') {
		*s = '\0';
		s++;
		return s;
	}

	return NULL; /* no more layers */
}

static int parse_layer_type(const char *type, pcb_layer_type_t *lyt, int *offs, int *has_offs)
{
	const char *soffs, *nxt, *cur;
	char *end;
	pcb_layer_type_t l;

	*lyt = 0;
	*offs = *has_offs = 0;

	soffs = strchr(type, ':');
	if (soffs != NULL) {
		*offs = strtol(soffs+1, &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "CAM rule: invalid offset '%s'\n", soffs);
			return 1;
		}
		*has_offs = 1;
	}

	for(cur = type; cur != NULL; cur = nxt) {
		nxt = strpbrk(cur, "-:(/");
		if (nxt != NULL)
			l = pcb_layer_type_strn2bit(cur, nxt - cur);
		else
			l = pcb_layer_type_str2bit(cur);
		if (nxt != NULL) {
			if (*nxt != '-')
				nxt = NULL;
			else
				nxt++;
		}
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
	if (xf_ != NULL)
		memset(xf_, 0, sizeof(pcb_xform_t));

	for(n = 0; n < spc; n++) {
		char *key = spk[n], *val = spv[n];

		if (strcmp(key, "purpose") == 0)
			*purpose = val;
		else if (strcmp(key, "bloat") == 0) {
			pcb_bool succ;
			double v = pcb_get_value(val, NULL, NULL, &succ);
			if (succ) {
				if (xf_ != NULL)
					xf_->bloat = v;
				if (xf != NULL)
					*xf = xf_;
			}
			else
				pcb_message(PCB_MSG_ERROR, "CAM: ignoring invalid layer supplement value '%s' for bloat\n", val);
		}
		else if (strcmp(key, "faded") == 0) {
			if (xf_ != NULL) xf_->layer_faded = 1;
			if (xf != NULL) *xf = xf_;
		}
		else if (strcmp(key, "partial") == 0) {
			if (xf_ != NULL) xf_->partial_export = 1;
			if (xf != NULL) *xf = xf_;
		}
		else if (strcmp(key, "wireframe") == 0) {
			if (xf_ != NULL) xf_->wireframe = 1;
			if (xf != NULL) *xf = xf_;
		}
		else if (strcmp(key, "thin_draw") == 0) {
			if (xf_ != NULL) xf_->thin_draw = 1;
			if (xf != NULL) *xf = xf_;
		}
		else if (strcmp(key, "thin_draw_poly") == 0) {
			if (xf_ != NULL) xf_->thin_draw_poly = 1;
			if (xf != NULL) *xf = xf_;
		}
		else if (strcmp(key, "check_planes") == 0) {
			if (xf_ != NULL) xf_->check_planes = 1;
			if (xf != NULL) *xf = xf_;
		}
		else if (strcmp(key, "flag_color") == 0) {
			if (xf_ != NULL) xf_->flag_color = 1;
			if (xf != NULL) *xf = xf_;
		}
		else if (strcmp(key, "hide_floaters") == 0) {
			if (xf_ != NULL) xf_->hide_floaters = 1;
			if (xf != NULL) *xf = xf_;
		}
		else
			pcb_message(PCB_MSG_ERROR, "CAM: ignoring unknown layer supplement key '%s'\n", key);
	}
}


int pcb_layergrp_list_by_addr(pcb_board_t *pcb, const char *curr, pcb_layergrp_id_t gids[PCB_MAX_LAYERGRP], char **spk, char **spv, int spc, int *vid, pcb_xform_t **xf, pcb_xform_t *xf_in, const char *err_prefix)
{
	pcb_layergrp_id_t gid, lgids[PCB_MAX_LAYERGRP];
	int gids_max = PCB_MAX_LAYERGRP;
	char *purpose;

	if (vid != NULL)
		*vid = -1;

	if (*curr == '#') {
		char *end;
		gid = strtol(curr+1, &end, 10);
		if ((*end == '\0') && (gid >= 0) && (gid < pcb->LayerGroups.len)) {
			gids[0] = gid;
			return 1;
		}
		if (err_prefix != NULL)
			pcb_message(PCB_MSG_ERROR, "%sinvalid layer group number\n", err_prefix);
		return -1;
	}

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

static pcb_layer_id_t layer_str2id_data(pcb_board_t *pcb, pcb_data_t *data, const char *str)
{
	char *end;
	pcb_layer_id_t id;

	if (*str == '#') {
		id = strtol(str+1, &end, 10);
		if ((*end == '\0') && (id >= 0) && (id < data->LayerN))
			return id;
	}
	if (*str == '@') {
		for(id = 0; id < data->LayerN; id++) {
			pcb_layer_t *ly = pcb_get_layer(data, id);
			if (ly == NULL) continue;
			if (strcmp(ly->name, str+1) == 0)
				return id;
		}
	}
	if (*str == '&') {
		str++;
		if (pcb_strcasecmp(str, "current") == 0)
			return PCB_CURRLID(pcb);
	}
	return -1;
}

static pcb_layer_id_t layer_str2id_grp(pcb_board_t *pcb, pcb_layergrp_t *grp, const char *str)
{
	char *end;
	long id, n;

	if (*str == '#') {
		str++;
		id = strtol(str, &end, 10);
		if ((*end == '+') || (*end == '-')) { /* search the nth positive or negative layer, count from 1 */
			int pos = (id > 0);
			if (!pos) id = -id;
			n = pos ? 0 : grp->len-1;
			for(; (pos ? (n < grp->len) : (n >= 0));) {
				pcb_layer_t *ly = pcb_get_layer(pcb->Data, grp->lid[n]);
				if (ly == NULL) continue;
				if ((*end == '-') && (ly->comb & PCB_LYC_SUB)) id--;
				else if ((*end == '+') && !(ly->comb & PCB_LYC_SUB)) id--;
				if (id == 0)
					return grp->lid[n];
				if (pos) n++;
				else n--;
			}
			return -1; /* not enough layers */
		}
		/* simply the nth layer ID, counting from 1 */
		if ((*end == '\0') && (id > 0) && (id <= grp->len))
			return grp->lid[id - 1];
		if ((*end == '\0') && (id < 0) && (id >= -((long)grp->len)))
			return grp->lid[id + grp->len];
		return -1;
	}
	if (*str == '@') {
		for(n = 0; n < grp->len; n++) {
			pcb_layer_t *ly = pcb_get_layer(pcb->Data, grp->lid[n]);
			if (ly == NULL) continue;
			if (strcmp(ly->name, str+1) == 0)
				return grp->lid[n];
		}
	}
	return -1;
}

pcb_layer_id_t pcb_layer_str2id(pcb_data_t *data, const char *str)
{
	char *sep;
	pcb_board_t *pcb = NULL;

	if (data->parent_type == PCB_PARENT_BOARD)
		pcb = data->parent.board;

	sep = strchr(str, '/');
	if (sep != NULL) {
		pcb_layergrp_id_t gid;
		pcb_layergrp_t *grp;
		if (pcb == NULL)
			return -1; /* group addressing works only on a board */
		gid = pcb_layergrp_str2id(pcb, str);
		grp = pcb_get_layergrp(pcb, gid);
		if (grp == NULL)
			return -1; /* invalid group */
		return layer_str2id_grp(pcb, grp, sep+1);
	}

	return layer_str2id_data(pcb, data, str);
}

pcb_layergrp_id_t pcb_layergrp_str2id(pcb_board_t *pcb, const char *str)
{
	char *end, *tmp = NULL;
	const char *curr;
	pcb_layer_id_t gid = -1;
	char *spk[64], *spv[64];
	int spc = 0, numg;
	pcb_layergrp_id_t gids[PCB_MAX_LAYERGRP];

	if (strchr(str, '(') != NULL) {
		curr = tmp = pcb_strdup(str);
		spc = sizeof(spk) / sizeof(spk[0]);
		end = pcb_parse_layergrp_address(tmp, spk, spv, &spc);
		if (end != NULL) {
			free(tmp);
			return -1;
		}
	}
	else
		curr = str;

	numg = pcb_layergrp_list_by_addr(pcb, curr, gids, spk, spv, spc, NULL, NULL, NULL, NULL);
	if (numg > 0)
		gid = gids[0];

	free(tmp);
	return gid;
}

/*** layer/group to addr ***/

static void addr_append_loc_type(gds_t *res, pcb_layer_type_t mask)
{
	const char *tmp;
	tmp = pcb_layer_type_bit2str(mask & PCB_LYT_ANYWHERE);
	if (tmp != NULL) {
		gds_append_str(res, tmp);
		gds_append(res, '-');
	}
	tmp = pcb_layer_type_bit2str(mask & PCB_LYT_ANYTHING);
	if (tmp != NULL)
		gds_append_str(res, tmp);
}

/* append loc-type:offs */
static int pcb_layergrp_append_to_addr_offs(pcb_board_t *pcb, pcb_layergrp_t *grp, gds_t *dst)
{
	char buf[64];
	long n, np = 0, nt = 0, offs, found = 0;
	pcb_layer_type_t mask = grp->ltype & (PCB_LYT_ANYWHERE | PCB_LYT_ANYTHING);

	addr_append_loc_type(dst, grp->ltype);

	for(n = 0; n < pcb->LayerGroups.len; n++) {
		pcb_layergrp_t *ng = &pcb->LayerGroups.grp[n];
		if (ng == grp)
			found = 1;
		if ((ng->ltype & (PCB_LYT_ANYWHERE | PCB_LYT_ANYTHING)) == mask) {
			if (!found)
				np++;
			nt++;
		}
	}
	/* np is the number of matching layer groups before grp, nt is the number
	   of matching groups total */
	if (mask & PCB_LYT_BOTTOM)
		offs = -(nt-np); /* bottom groups are counted from the bottom */
	else if (mask & PCB_LYT_TOP)
		offs = np+1; /* top groups are counted from top */
	else if (mask & PCB_LYT_INTERN) { /* intern groups are counted from the closer side */
		if (np < nt/2)
			offs = np+1;
		else
			offs = -(nt-np);
	}
	else { /* global: count from "top" */
		offs = np+1;
	}
	sprintf(buf, ":%ld", offs);
	gds_append_str(dst, buf);
	return 0;
}

/* append loc-type(purpose=...) */
static int pcb_layergrp_append_to_addr_purp(pcb_board_t *pcb, pcb_layergrp_t *grp, gds_t *dst)
{
	addr_append_loc_type(dst, grp->ltype);
	if (grp->purpose != NULL) {
		gds_append_str(dst, "(purpose=");
		gds_append_str(dst, grp->purpose);
		gds_append(dst, ')');
	}
	return 0;
}

int pcb_layergrp_append_to_addr(pcb_board_t *pcb, pcb_layergrp_t *grp, gds_t *dst)
{
	if (PCB_LAYER_IN_STACK(grp->ltype))
		return pcb_layergrp_append_to_addr_offs(pcb, grp, dst);
	return pcb_layergrp_append_to_addr_purp(pcb, grp, dst);
}

int pcb_layer_append_to_addr(pcb_board_t *pcb, pcb_layer_t *ly, gds_t *dst)
{
	pcb_layer_id_t lid;
	pcb_layergrp_t *grp;
	long n;
	char buf[64];

	if (ly->is_bound)
		return -1;

	grp = pcb_get_layergrp(pcb, ly->meta.real.grp);
	if (grp == NULL)
		return -1;

	lid = ly - pcb->Data->Layer;

	for(n = 0; n < grp->len; n++) {
		if (grp->lid[n] == lid) {
			pcb_layergrp_append_to_addr(pcb, grp, dst);
			sprintf(buf, "/#%ld", n);
			gds_append_str(dst, buf);
			return 0;
		}
	}

	return -1;
}

char *pcb_layergrp_to_addr(pcb_board_t *pcb, pcb_layergrp_t *grp)
{
	gds_t res;
	gds_init(&res);

	if (pcb_layergrp_append_to_addr(pcb, grp, &res) == 0)
		return res.array;

	gds_uninit(&res);
	return NULL;
}

char *pcb_layer_to_addr(pcb_board_t *pcb, pcb_layer_t *ly)
{
	gds_t res;
	gds_init(&res);

	if (pcb_layer_append_to_addr(pcb, ly, &res) == 0)
		return res.array;

	gds_uninit(&res);
	return NULL;
}
