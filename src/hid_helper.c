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
#include "hid_helper.h"
#include "hid_attrib.h"
#include "hid_helper.h"
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

int pcb_cam_begin(pcb_board_t *pcb, pcb_cam_t *dst, const char *src, const pcb_hid_attribute_t *attr_tbl, int numa, pcb_hid_attr_val_t *options)
{
	char *curr, *next, *opts;

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

	/* split off opts from layers */
	opts = strchr(next, ';');
	if (opts != NULL) {
		*opts = '\0';
		opts++;
	}

	/* parse layers */
	for(curr = next; curr != NULL; curr = next) {
		pcb_layergrp_id_t gid;
		next = strchr(curr, ',');
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		curr = strip(curr);
		gid = pcb_layergrp_by_name(pcb, curr);
		if (gid < 0) {
			pcb_message(PCB_MSG_ERROR, "CAM rule: no such layer group '%s'\n", curr);
			goto err;
		}
		if (pcb->LayerGroups.grp[gid].len <= 0)
			continue;
		pcb_layervis_change_group_vis(pcb->LayerGroups.grp[gid].lid[0], 1, 0);
	}

	/* parse options */
	for(curr = opts; curr != NULL; curr = next) {
		next = strchr(curr, ';');
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		curr = strip(curr);
pcb_trace(" opt='%s'\n", curr);
	}

	dst->active = 1;
	return 0;
	err:;
	layervis_restore(dst);
	free(dst->inst);
	dst->inst = NULL;
	return -1;
}

void pcb_cam_end(pcb_cam_t *dst)
{
	free(dst->inst);
	dst->inst = NULL;
	if (!dst->active)
		return;
	layervis_restore(dst);
	dst->active = 0;
}

