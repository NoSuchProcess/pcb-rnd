/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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
 *
 */

#include "config.h"
#include "board.h"
#include "data.h"
#include "hid_cam.h"
#include "hid_attrib.h"
#include "compat_misc.h"
#include "layer_vis.h"
#include "plug_io.h"

char *pcb_layer_to_file_name(char *dest, pcb_layer_id_t lid, unsigned int flags, const char *purpose, int purpi, pcb_file_name_style_t style)
{
	const pcb_virt_layer_t *v;
	pcb_layergrp_id_t group;
	int nlayers;
	const char *single_name, *res = NULL;

	if (flags == 0)
		flags = pcb_layer_flags(PCB, lid);

	if (flags & PCB_LYT_BOUNDARY) {
		strcpy(dest, "outline");
		return dest;
	}

	v = pcb_vlayer_get_first(flags, purpose, purpi);
	if (v != NULL) {
		strcpy(dest, v->name);
		return dest;
	}

	
	group = pcb_layer_get_group(PCB, lid);
	nlayers = PCB->LayerGroups.grp[group].len;
	single_name = pcb_layer_name(PCB->Data, lid);

	if (flags & PCB_LYT_TOP) {
		if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 2))
			res = single_name;
		if (flags & PCB_LYT_SILK)
			res = "topsilk";
		else if (flags & PCB_LYT_MASK)
			res = "topmask";
		else if (flags & PCB_LYT_PASTE)
			res = "toppaste";
		else
			res = "top";
	}
	else if (flags & PCB_LYT_BOTTOM) {
		if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 2))
			res = single_name;
		if (flags & PCB_LYT_SILK)
			res = "bottomsilk";
		else if (flags & PCB_LYT_MASK)
			res = "bottommask";
		else if (flags & PCB_LYT_PASTE)
			res = "bottompaste";
		else
		res = "bottom";
	}
	else {
		static char buf[PCB_DERIVE_FN_SUFF_LEN];
		if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 1))
			res = single_name;
		sprintf(buf, "group%ld", group);
		res = buf;
	}

	assert(res != NULL);
	strcpy(dest, res);
	return dest;
}


void pcb_derive_default_filename(const char *pcbfile, pcb_hid_attribute_t * filename_attrib, const char *suffix)
{
	char *buf;
	const char *pf;

	if (pcbfile == NULL)
		pf = "unknown.pcb";
	else
		pf = pcbfile;

	buf = (char *) malloc(strlen(pf) + strlen(suffix) + 1);
	if (buf) {
		size_t bl;
		pcb_plug_io_t *i;
		strcpy(buf, pf);
		bl = strlen(buf);
		for(i = pcb_plug_io_chain; i != NULL; i = i->next) {
			if (i->default_extension != NULL) {
				int slen = strlen(i->default_extension);
				if (bl > slen && strcmp(buf + bl - slen, i->default_extension) == 0) {
					buf[bl - slen] = 0;
					break;
				}
			}
		}
		strcat(buf, suffix);
		if (filename_attrib->default_val.str_value)
			free((void *) filename_attrib->default_val.str_value);
		filename_attrib->default_val.str_value = buf;
	}
}

/* remove leading and trailing whitespace */
static char *strip(char *s)
{
	char *end;
	while(isspace(*s)) s++;
	end = s + strlen(s) - 1;
	while((end >= s) && (isspace(*end))) {
		*end = '\0';
		end--;
	}
	return s;
}

static void layervis_save_and_reset(pcb_cam_t *cam)
{
	pcb_layer_id_t n;
	for(n = 0; n < cam->pcb->Data->LayerN; n++) {
		cam->orig_vis[n] = cam->pcb->Data->Layer[n].meta.real.vis;
		cam->pcb->Data->Layer[n].meta.real.vis = 0;
	}
}

static void layervis_restore(pcb_cam_t *cam)
{
	pcb_layer_id_t n;
	for(n = 0; n < cam->pcb->Data->LayerN; n++)
		cam->pcb->Data->Layer[n].meta.real.vis = cam->orig_vis[n];
}

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

static char *lp_err = "";

/* Parse a layer directive: split at comma, curr will end up holding the
   layer name. If there were transformations in (), they are split and
   listed in tr up to at most *spc entries. Returns NULL on error or
   pointer to the next layer directive. */
static char *parse_layer(char *curr, char **spk, char **spv, int *spc)
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
				if (*spc >= trmax)
					return lp_err;
				lasta = strip(lasta);
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
		return lp_err;

	out:;

	if (*s != '\0') {
		*s = '\0';
		s++;
		return s;
	}

	return NULL; /* no more layers */
}


static void parse_layer_supplements(char **spk, char **spv, int spc,   char **purpose, pcb_xform_t **xf, pcb_xform_t *xf_)
{
	int n;

	*purpose = NULL;
	memset(xf_, 0, sizeof(pcb_xform_t));

	for(n = 0; n < spc; n++) {
		char *key = spk[n], *val = spv[n];
		pcb_trace(" [%d] '%s' '%s'\n", n, spk[n], spv[n]);
		if (strcmp(key, "purpose") == 0)
			*purpose = val;
		if (strcmp(key, "bloat") == 0) {
			pcb_bool succ;
			double v = pcb_get_value(val, NULL, NULL, &succ);
			if (succ) {
				xf_->bloat = v;
				*xf = xf_;
			}
			else
				pcb_message(PCB_MSG_ERROR, "CAM: ignoring invalid layer supplement value '%s' for bloat\n", val);
		}
		else
			pcb_message(PCB_MSG_ERROR, "CAM: ignoring unknown layer supplement key '%s'\n", key);
	}
}

int pcb_cam_begin(pcb_board_t *pcb, pcb_cam_t *dst, const char *src, const pcb_hid_attribute_t *attr_tbl, int numa, pcb_hid_attr_val_t *options)
{
	char *curr, *next, *purpose;

	if ((src == NULL) || (*src == '\0'))
		return 0;

	memset(dst, 0, sizeof(pcb_cam_t));
	dst->pcb = pcb;
	dst->inst = pcb_strdup(src);
	layervis_save_and_reset(dst);


	/* Syntax: fn=layergrp,layergrp,layergrp;--opt=val;--opt=val */
	/* parse: get file name name */
	next = strchr(dst->inst, '=');
	if (next == NULL) {
		pcb_message(PCB_MSG_ERROR, "CAM rule missing '='\n");
		goto err;
	}
	*next = '\0';
	next++;
	dst->fn = strip(dst->inst);

	while(isspace(*next))
		next++;

	/* parse layers */
	for(curr = next; curr != NULL; curr = next) {
		char *spk[64], *spv[64];
		int spc = sizeof(spk) / sizeof(spk[0]);
		next = parse_layer(curr, spk, spv, &spc);
		if (next == lp_err) {
			pcb_message(PCB_MSG_ERROR, "CAM rule: invalid layer transformation\n");
			goto err;
		}

{
}

		curr = strip(curr);
		if (*curr == '@') {
			/* named layer group */
			pcb_xform_t *xf, xf_;
			pcb_layergrp_id_t gid;
			curr++;
			gid = pcb_layergrp_by_name(pcb, curr);
			if (gid < 0) {
				pcb_message(PCB_MSG_ERROR, "CAM rule: no such layer group '%s'\n", curr);
				goto err;
			}
			if (pcb->LayerGroups.grp[gid].len <= 0)
				continue;
			pcb_layervis_change_group_vis(pcb->LayerGroups.grp[gid].lid[0], 1, 0);
			dst->grp_vis[gid] = 1;

			parse_layer_supplements(spk, spv, spc, &purpose, &xf, &xf_);

			dst->xform[gid] = &dst->xform_[gid];
			memcpy(&dst->xform_[gid], &xf_, sizeof(pcb_xform_t));
		}
		else {
			/* by layer type */
			int offs, has_offs;
			pcb_layer_type_t lyt;
			const pcb_virt_layer_t *vl;
			pcb_xform_t *xf, xf_;

			if (parse_layer_type(curr, &lyt, &offs, &has_offs) != 0)
				goto err;

#warning TODO: extend the syntax for purpose
			parse_layer_supplements(spk, spv, spc, &purpose, &xf, &xf_);

			vl = pcb_vlayer_get_first(lyt, purpose, -1);
			if (vl == NULL) {
				pcb_layergrp_id_t gids[PCB_MAX_LAYERGRP];
				int n, len = pcb_layergrp_listp(dst->pcb, lyt, gids, sizeof(gids)/sizeof(gids[0]), -1, purpose);
				if (has_offs) {
					if (offs < 0)
						offs = len - offs;
					else
						offs--;
					if ((offs >= 0) && (offs < len)) {
						pcb_layergrp_id_t gid = gids[offs];
						pcb_layervis_change_group_vis(pcb->LayerGroups.grp[gid].lid[0], 1, 0);
						dst->grp_vis[gid] = 1;
						if (xf != NULL) {
							dst->xform[gid] = &dst->xform_[gid];
							memcpy(&dst->xform_[gid], &xf_, sizeof(pcb_xform_t));
						}
					}
				}
				else {
					for(n = 0; n < len; n++) {
						pcb_layergrp_id_t gid = gids[n];
						pcb_layervis_change_group_vis(pcb->LayerGroups.grp[gid].lid[0], 1, 0);
						dst->grp_vis[gid] = 1;
						if (xf != NULL) {
							dst->xform[gid] = &dst->xform_[gid];
							memcpy(&dst->xform_[gid], &xf_, sizeof(pcb_xform_t));
						}
					}
				}
				
			}
			else {
				int vid = vl->new_id - PCB_LYT_VIRTUAL - 1;
				dst->vgrp_vis[vid] = 1;
				if (xf != NULL) {
					dst->vxform[vid] = &dst->vxform_[vid];
					memcpy(&dst->vxform_[vid], &xf_, sizeof(pcb_xform_t));
				}

			}
		}
	}

	dst->active = 1;
	return 0;
	err:;
	layervis_restore(dst);
	free(dst->inst);
	dst->inst = NULL;
	return -1;
}

int pcb_cam_end(pcb_cam_t *dst)
{
	free(dst->inst);
	dst->inst = NULL;
	if (!dst->active)
		return -1;
	layervis_restore(dst);
	dst->active = 0;
	return dst->exported_grps;
}


int pcb_cam_set_layer_group_(pcb_cam_t *cam, pcb_layergrp_id_t group, const char *purpose, int purpi, unsigned int flags, pcb_xform_t **xform)
{
	const pcb_virt_layer_t *vl;

	if (!cam->active)
		return 0;

	vl = pcb_vlayer_get_first(flags, purpose, purpi);
	if (vl == NULL) {
		if (group == -1)
			return 1;

		if (!cam->grp_vis[group])
			return 1;

		if (cam->xform[group] != NULL)
			*xform = cam->xform[group];
	}
	else {
		int vid = vl->new_id - PCB_LYT_VIRTUAL - 1;
		if (!cam->vgrp_vis[vid])
			return 1;

		if (cam->vxform[vid] != NULL)
			*xform = cam->xform[vid];
	}

	cam->exported_grps++;
	return 0;
}

