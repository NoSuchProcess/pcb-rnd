/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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
 *
 */

#include "config.h"

#include <genht/htsp.h>
#include <genht/hash.h>

#include "board.h"
#include "data.h"
#include "hid_cam.h"
#include "hid_attrib.h"
#include "compat_misc.h"
#include "layer_vis.h"
#include "plug_io.h"

htsp_t *pcb_cam_vars = NULL; /* substitute %% variables from this hash */

char *pcb_layer_to_file_name(gds_t *dest, pcb_layer_id_t lid, unsigned int flags, const char *purpose, int purpi, pcb_file_name_style_t style)
{
	const pcb_virt_layer_t *v;
	pcb_layergrp_id_t group;
	int nlayers;
	const char *single_name, *res = NULL;

	if (flags == 0)
		flags = pcb_layer_flags(PCB, lid);

	dest->used = 0;

	if (style == PCB_FNS_pcb_rnd) {
		const char *sloc, *styp;
		group = pcb_layer_get_group(PCB, lid);
		sloc = pcb_layer_type_bit2str(flags & PCB_LYT_ANYWHERE);
		styp = pcb_layer_type_bit2str(flags & (PCB_LYT_ANYTHING | PCB_LYT_VIRTUAL));
		if (sloc == NULL) sloc = "global";
		if (styp == NULL) styp = "none";
		if (purpose == NULL) purpose = "none";
		if (group < 0) {
			pcb_append_printf(dest, "%s.%s.%s.none", sloc, styp, purpose);
		}
		else
			pcb_append_printf(dest, "%s.%s.%s.%ld", sloc, styp, purpose, group);
		return dest->array;
	}

	if (flags & PCB_LYT_BOUNDARY) {
		gds_append_str(dest, "outline");
		return dest->array;
	}

	/* NOTE: long term only PCB_FNS_pcb_rnd will be supported and the rest,
	   below, will be removed */
	v = pcb_vlayer_get_first(flags, purpose, purpi);
	if (v != NULL) {
		gds_append_str(dest, v->name);
		return dest->array;
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
	gds_append_str(dest, res);
	return dest->array;
}


char *pcb_derive_default_filename_(const char *pcbfile, const char *suffix)
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
	}

	return buf;
}

void pcb_derive_default_filename(const char *pcbfile, pcb_export_opt_t *filename_attrib, const char *suffix)
{
	if (filename_attrib->default_val.str)
		free((char *)filename_attrib->default_val.str);
	filename_attrib->default_val.str = pcb_derive_default_filename_(pcbfile, suffix);
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
		else
			pcb_message(PCB_MSG_ERROR, "CAM: ignoring unknown layer supplement key '%s'\n", key);
	}
}

int pcb_cam_begin(pcb_board_t *pcb, pcb_cam_t *dst, const char *src, const pcb_export_opt_t *attr_tbl, int numa, pcb_hid_attr_val_t *options)
{
	char *curr, *next, *purpose;

	memset(dst, 0, sizeof(pcb_cam_t));

	if ((src == NULL) || (*src == '\0'))
		return 0;

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

	if (strchr(dst->fn, '%') != NULL) {
		dst->fn_template = dst->fn;
		dst->fn = NULL;
	}

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

		curr = strip(curr);
		if (*curr == '@') {
			/* named layer group */
			pcb_xform_t *xf, xf_;
			pcb_layergrp_id_t gid;
			curr++;
			gid = pcb_layergrp_by_name(pcb, curr);
			if (gid < 0) {
				const pcb_virt_layer_t *v;
				int n, vid = -1;
				for(n = 0, v = pcb_virt_layers; v->name != NULL; n++,v++) {
					if (strcmp(v->name, curr) == 0) {
						vid = n;
						break;
					}
				}
				if (vid != -1) {
					dst->vgrp_vis[vid] = 1;
					continue;
				}
			}
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
			pcb_xform_t *xf = NULL, xf_;

			if (parse_layer_type(curr, &lyt, &offs, &has_offs) != 0)
				goto err;

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
	if (dst->fn_template != NULL) {
		free(dst->fn);
		free(dst->fn_last);
	}
	dst->inst = NULL;
	if (!dst->active)
		return -1;
	layervis_restore(dst);
	dst->active = 0;
	return dst->exported_grps;
}

typedef struct  {
	pcb_cam_t *cam;
	const pcb_layergrp_t *grp;
	const pcb_virt_layer_t *vl;
	htsp_t *vars;
} cam_name_ctx_t;
static int cam_update_name_cb(void *ctx_, gds_t *s, const char **input);

void pcb_cam_begin_nolayer(pcb_board_t *pcb, pcb_cam_t *dst, const char *src, const char **fn_out)
{
	cam_name_ctx_t ctx;

	memset(dst, 0, sizeof(pcb_cam_t));
	dst->pcb = pcb;
	if (src != NULL) {
		if (strchr(src, '=') != NULL)
			pcb_message(PCB_MSG_ERROR, "global exporter --cam doesn't take '=' and layers, only a file name\n");

		dst->fn_template = pcb_strdup(src);

		memset(&ctx, 0, sizeof(ctx));
		ctx.cam = dst;
		ctx.vars = pcb_cam_vars;
		dst->fn = pcb_strdup_subst(dst->fn_template, cam_update_name_cb, &ctx, PCB_SUBST_HOME | PCB_SUBST_PERCENT | PCB_SUBST_CONF);
		*fn_out = dst->fn;
	}
}



/* convert string to integer, step input beyond the terminating % */
static int get_tune(const char **input)
{
	char *end;
	int res;

	if (**input == '%') {
		(*input)++;
		return 0;
	}
	res = atoi(*input);
	end = strchr(*input, '%');
	*input = end+1;
	return res;
}

static int cam_update_name_cb(void *ctx_, gds_t *s, const char **input)
{
	cam_name_ctx_t *ctx = ctx_;
	if (strncmp(*input, "name%", 5) == 0) {
		(*input) += 5;
		if (ctx->grp != NULL) {
			if (ctx->grp->name != NULL)
				gds_append_str(s, ctx->grp->name);
		}
		else if (ctx->vl != NULL)
			gds_append_str(s, ctx->vl->name);
	}
	else if (strncmp(*input, "top_offs", 8) == 0) {
		int tune, from_top = -1;
		pcb_layergrp_t *tcop = pcb_get_grp(&PCB->LayerGroups, PCB_LYT_TOP, PCB_LYT_COPPER);
		pcb_layergrp_id_t tcop_id = pcb_layergrp_id(PCB, tcop);

		(*input) += 8;
		tune = get_tune(input);

		pcb_layergrp_dist(PCB, pcb_layergrp_id(PCB, ctx->grp), tcop_id, PCB_LYT_COPPER, &from_top);
		pcb_append_printf(s, "%d", from_top+tune);
	}
	else if (strncmp(*input, "bot_offs", 8) == 0) {
		int tune, from_bot = -1;
		pcb_layergrp_t *bcop = pcb_get_grp(&PCB->LayerGroups, PCB_LYT_BOTTOM, PCB_LYT_COPPER);
		pcb_layergrp_id_t bcop_id = pcb_layergrp_id(PCB, bcop);

		(*input) += 8;
		tune = get_tune(input);

		pcb_layergrp_dist(PCB, pcb_layergrp_id(PCB, ctx->grp), bcop_id, PCB_LYT_COPPER, &from_bot);
		pcb_append_printf(s, "%d", from_bot+tune);
	}
	else {
		char *end = strchr(*input, '%');
		char varname[128];
		int len;

		if (end == NULL) {
			gds_append_str(s, *input);
			return 0;
		}

		if (ctx->vars != NULL) {
			len = end - *input;
			if (len < sizeof(varname)-1) {
				const char *val;
				strncpy(varname, *input, len);
				varname[len] = 0;
				val = htsp_get(ctx->vars, varname);
				if (val != NULL)
					gds_append_str(s, val);
			}
			else
				pcb_message(PCB_MSG_ERROR, "cam job error: %%%% variable name too long at: '%%%s' - did not substitute\n", *input);
		}

		*input = end+1;
	}

	return 0;
}

static int cam_update_name(pcb_cam_t *cam, pcb_layergrp_id_t gid, const pcb_virt_layer_t *vl)
{
	cam_name_ctx_t ctx;
	if (cam->fn_template == NULL)
		return 0; /* not templated */

	free(cam->fn);
	ctx.cam = cam;
	ctx.grp = pcb_get_layergrp(cam->pcb, gid);
	ctx.vl = vl;
	ctx.vars = pcb_cam_vars;
	cam->fn = pcb_strdup_subst(cam->fn_template, cam_update_name_cb, &ctx, PCB_SUBST_HOME | PCB_SUBST_PERCENT | PCB_SUBST_CONF);
	if ((cam->fn_last == NULL) || (strcmp(cam->fn, cam->fn_last) != 0)) {
		cam->fn_changed = 1;
		free(cam->fn_last);
		cam->fn_last = pcb_strdup(cam->fn);
	}
	else
		cam->fn_changed = 0;
	return 0;
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
	return cam_update_name(cam, group, NULL);;
}


void *pcb_cam_vars_alloc(void)
{
	return htsp_alloc(strhash, strkeyeq);
}

void pcb_cam_vars_free(void *ctx)
{
	htsp_t *vars = ctx;
	htsp_entry_t *e;

	for(e = htsp_first(vars); e != NULL; e = htsp_next(vars, e)) {
		free(e->key);
		free(e->value);
	}
	htsp_free(vars);
}

void *pcb_cam_vars_use(void *new_vars)
{
	void *old = pcb_cam_vars;
	pcb_cam_vars = new_vars;
	return old;
}

void pcb_cam_set_var(void *ctx, char *key, char *val)
{
	htsp_t *vars = ctx;
	htsp_entry_t *e = htsp_popentry(vars, key);
	if (e != NULL) {
		free(e->key);
		free(e->value);
	}
	htsp_set(vars, key, val);
}
