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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
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

char *pcb_layer_to_file_name(char *dest, pcb_layer_id_t lid, unsigned int flags, pcb_file_name_style_t style)
{
	const pcb_virt_layer_t *v;
	pcb_layergrp_id_t group;
	int nlayers;
	const char *single_name, *res = NULL;

	if (flags == 0)
		flags = pcb_layer_flags(PCB, lid);

	if (flags & PCB_LYT_OUTLINE) {
		strcpy(dest, "outline");
		return dest;
	}

	v = pcb_vlayer_get_first(flags);
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


void pcb_derive_default_filename(const char *pcbfile, pcb_hid_attribute_t * filename_attrib, const char *suffix, char **memory)
{
	char *buf;
	char *pf;

	if (pcbfile == NULL)
		pf = pcb_strdup("unknown.pcb");
	else
		pf = pcb_strdup(pcbfile);

	if (!pf || (memory && filename_attrib->default_val.str_value != *memory))
		return;

	buf = (char *) malloc(strlen(pf) + strlen(suffix) + 1);
	if (memory)
		*memory = buf;
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

	free(pf);
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

int pcb_cam_begin(pcb_board_t *pcb, pcb_cam_t *dst, const char *src, const pcb_hid_attribute_t *attr_tbl, int numa, pcb_hid_attr_val_t *options)
{
	char *curr, *next;

	if (src == NULL)
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
pcb_trace("CAM FN='%s'\n", dst->fn);

	while(isspace(*next))
		next++;

	/* parse layers */
	for(curr = next; curr != NULL; curr = next) {

		next = strchr(curr, ',');
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		curr = strip(curr);
		if (*curr == '@') {
			/* named layer */
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
		}
		else {
			/* by layer type */
			int offs, has_offs;
			pcb_layer_type_t lyt;
			pcb_virt_layer_t *vl;

			if (parse_layer_type(curr, &lyt, &offs, &has_offs) != 0)
				goto err;

			vl = pcb_vlayer_get_first(lyt);
			if (vl == NULL) {
				pcb_layergrp_id_t gids[PCB_MAX_LAYERGRP];
				int n, len = pcb_layergrp_list(dst->pcb, lyt, gids, sizeof(gids)/sizeof(gids[0]));
				if (has_offs) {
					if (offs < 0)
						offs = len - offs;
					else
						offs--;
					if ((offs >= 0) && (offs < len)) {
						pcb_layergrp_id_t gid = gids[offs];
						pcb_layervis_change_group_vis(pcb->LayerGroups.grp[gid].lid[0], 1, 0);
						dst->grp_vis[gid] = 1;
					}
				}
				else {
					for(n = 0; n < len; n++) {
						pcb_layergrp_id_t gid = gids[n];
						pcb_layervis_change_group_vis(pcb->LayerGroups.grp[gid].lid[0], 1, 0);
						dst->grp_vis[gid] = 1;
					}
				}
				
			}
			else {
				int vid = vl->new_id - PCB_LYT_VIRTUAL - 1;
				dst->vgrp_vis[vid] = 1;
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
		return;
	layervis_restore(dst);
	dst->active = 0;
	return dst->exported_grps;
}


int pcb_cam_set_layer_group_(pcb_cam_t *cam, pcb_layergrp_id_t group, unsigned int flags)
{
	pcb_virt_layer_t *vl;

	if (!cam->active)
		return 0;

	vl = pcb_vlayer_get_first(flags);
	if (vl == NULL) {
		if (group == -1)
			return 1;

		if (!cam->grp_vis[group])
			return 1;
	}
	else {
		int vid = vl->new_id - PCB_LYT_VIRTUAL - 1;
		if (!cam->vgrp_vis[vid])
			return 1;
	}

	cam->exported_grps++;
	return 0;
}

