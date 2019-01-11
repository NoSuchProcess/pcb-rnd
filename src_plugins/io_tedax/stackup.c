/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - stackup import/export
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
#include <stdio.h>
#include <math.h>
#include <genht/htsp.h>
#include <genht/htip.h>
#include <genht/hash.h>

#include "stackup.h"
#include "parse.h"
#include "compat_misc.h"
#include "safe_fs.h"
#include "error.h"
#include "plug_io.h"

void tedax_stackup_init(tedax_stackup_t *ctx)
{
	htsp_init(&ctx->n2g, strhash, strkeyeq);
	vtp0_init(&ctx->g2n);
}

void tedax_stackup_uninit(tedax_stackup_t *ctx)
{
	htsp_uninit(&ctx->n2g);
	vtp0_uninit(&ctx->g2n);
}


static void gen_layer_name(char dst[65], const char *name, int prefix)
{
	char *d = dst;
	int rem = 64, l;

	if (prefix != 0) {
		l = sprintf(dst, "%d_", prefix);
		rem -= l;
		d += l;
	}
	for(;(*name != '\0') && (rem > 0);rem--,name++) {
		if (isalnum(*name) || (*name == '-') || (*name == '.') || (*name == '_'))
			*d = *name;
		else
			*d = '_';
		d++;
	}
	*d = '\0';
}

static const char ANY_PURP[] = "any";

typedef enum { TDX_OUTER=1, TDX_INNER=2, TDX_ALL=4, TDX_VIRTUAL=8 } tedax_loc_t;

typedef struct {
	const char *typename;
	const char *purpose;
	pcb_layer_type_t type;
	int force_virtual;
	tedax_loc_t loc;
} tedax_layer_t;

static const tedax_layer_t layertab[] = {
	{"copper",    NULL,        PCB_LYT_COPPER,    0, TDX_OUTER | TDX_INNER},
	{"insulator", NULL,        PCB_LYT_SUBSTRATE, 0, TDX_INNER},
	{"silk",      NULL,        PCB_LYT_SILK,      0, TDX_OUTER},
	{"paste",     NULL,        PCB_LYT_PASTE,     0, TDX_OUTER},
	{"mask",      NULL,        PCB_LYT_MASK,      0, TDX_OUTER},
	{"umech",     "uroute",    PCB_LYT_BOUNDARY,  0, TDX_ALL},
	{"umech",     "udrill",    PCB_LYT_BOUNDARY,  0, TDX_ALL},
	{"umech",     "uroute",    PCB_LYT_MECH,      0, TDX_ALL},
	{"umech",     "udrill",    PCB_LYT_MECH,      0, TDX_ALL},
	{"pmech",     "proute",    PCB_LYT_BOUNDARY,  0, TDX_ALL},
	{"pmech",     "pdrill",    PCB_LYT_BOUNDARY,  0, TDX_ALL},
	{"pmech",     "proute",    PCB_LYT_MECH,      0, TDX_ALL},
	{"pmech",     "pdrill",    PCB_LYT_MECH,      0, TDX_ALL},
	{"vcut",      "vcut",      PCB_LYT_BOUNDARY,  0, TDX_ALL},
	{"vcut",      "vcut",      PCB_LYT_MECH,      0, TDX_ALL},
	{"doc",       ANY_PURP,    PCB_LYT_DOC,       1, TDX_OUTER | TDX_INNER | TDX_ALL},
	{NULL}
};

static void save_prop(pcb_board_t *pcb, FILE *f, const char *name, const char *key, const char *val)
{
	if ((strlen(name) + strlen(key) + strlen(val)*2 + 32) >= 512) {
		pcb_io_incompat_save(pcb->Data, NULL, "stackup", "Property value string too long", val);
		return;
	}
	fprintf(f, "  lprop %s material %s ", name, key);
	tedax_fprint_escape(f, val);
	fputc('\n', f);
}


static void save_user_props(pcb_board_t *pcb, FILE *f, pcb_layergrp_t *grp, const char *name)
{
	int n;
	pcb_attribute_t *a;
	char *mat = NULL, *thermk = NULL, *thick = NULL, *fab_color = NULL, *dielect = NULL;

	for(n = 0, a = grp->Attributes.List; n < grp->Attributes.Number; n++,a++) {
		char *key = a->name, *val = a->value;

		if ((strcmp(key, "material") == 0) && (mat == NULL))
			mat = val;
		else if (strcmp(key, "tedax::material")== 0)
			mat = val;
		else if ((strcmp(key, "thermk") == 0) && (thermk == NULL))
			thermk = val;
		else if (strcmp(key, "tedax::thermk")== 0)
			thermk = val;
		else if ((strcmp(key, "fab-color") == 0) && (fab_color == NULL))
			fab_color = val;
		else if (strcmp(key, "tedax::fab-color")== 0)
			fab_color = val;
		else if ((strcmp(key, "thickness") == 0) && (thick == NULL))
			thick = val;
		else if (strcmp(key, "tedax::thickness")== 0)
			thick = val;
		else if ((strcmp(key, "dielect") == 0) && (dielect == NULL))
			dielect = val;
		else if (strcmp(key, "tedax::dielect")== 0)
			dielect = val;
	}


	if (thermk != NULL)
		save_prop(pcb, f, name, "thermk", thermk);
	if (fab_color != NULL) {
		if (grp->ltype & (PCB_LYT_TOP | PCB_LYT_BOTTOM)) {
			save_prop(pcb, f, name, "fab-color", fab_color);
		}
		else {
			char *title = pcb_strdup_printf("Unsupported group fab-color: %s", grp->name);
			pcb_io_incompat_save(pcb->Data, NULL, "stackup", title, "Only outer layer groups should have fab-color.");
			free(title);
		}
	}
	if (dielect != NULL) {
		if (grp->ltype & PCB_LYT_SUBSTRATE) {
			save_prop(pcb, f, name, "dielect", dielect);
		}
		else {
			char *title = pcb_strdup_printf("Unsupported group dielect: %s", grp->name);
			pcb_io_incompat_save(pcb->Data, NULL, "stackup", title, "Group type should not have dielect constant - only substrate layers should.");
			free(title);
		}
	}
	if (mat != NULL) {
		if (grp->ltype & PCB_LYT_SUBSTRATE) {
			save_prop(pcb, f, name, "material", mat);
		}
		else {
			char *title = pcb_strdup_printf("Unsupported group material: %s", grp->name);
			pcb_io_incompat_save(pcb->Data, NULL, "stackup", title, "Group type should not have a material - only substrate layers should.");
			free(title);
		}
	}

	if (thick != NULL) {
		if (grp->ltype & (PCB_LYT_SUBSTRATE | PCB_LYT_COPPER)) {
			pcb_bool succ;
			double th = pcb_get_value(thick, NULL, NULL, &succ);
			if (succ) {
				char tmp[64];
				pcb_sprintf(tmp, "%mu", (pcb_coord_t)th);
				save_prop(pcb, f, name, "thickness", tmp);
			}
			else {
				char *title = pcb_strdup_printf("Invalid thickness value: %s", grp->name);
				pcb_io_incompat_save(pcb->Data, NULL, "stackup", title, "Thicnkess value must be a numeric with an unit suffix, e.g. 0.7mm");
				free(title);
			}
		}
		else {
			char *title = pcb_strdup_printf("Unsupported group thickness: %s", grp->name);
			pcb_io_incompat_save(pcb->Data, NULL, "stackup", title, "Group type should not have a thickness - only substrate and copper layers should.");
			free(title);
		}
	}
}

static const tedax_layer_t *tedax_layer_lookup_by_type(pcb_board_t *pcb, const pcb_layergrp_t *grp, const char **lloc)
{
	const tedax_layer_t *t;

	for(t = layertab; t->typename != NULL; t++) {
		int loc_accept = 0;
		if ((grp->ltype & t->type) != t->type)
			continue;
		if (t->purpose != NULL)
			if ((grp->purpose == NULL) || (strcmp(grp->purpose, t->purpose) != 0))
				continue;

		loc_accept |= ((t->loc & TDX_OUTER) && (grp->ltype & (PCB_LYT_TOP | PCB_LYT_BOTTOM)));
		loc_accept |= ((t->loc & TDX_INNER) && (grp->ltype & PCB_LYT_INTERN));
		loc_accept |= ((t->loc & TDX_ALL) && (grp->ltype & PCB_LYT_ANYWHERE) == 0);
		loc_accept |= ((t->loc & TDX_VIRTUAL) && (grp->ltype & PCB_LYT_VIRTUAL));

		if (!loc_accept)
			continue;

		/* found a match */
		if (!t->force_virtual) {
			if (grp->ltype & PCB_LYT_TOP) *lloc = "top";
			else if (grp->ltype & PCB_LYT_INTERN) *lloc = "inner";
			else if (grp->ltype & PCB_LYT_BOTTOM) *lloc = "bottom";
			else if (grp->ltype & PCB_LYT_VIRTUAL) *lloc = "virtual";
			else if ((grp->ltype & PCB_LYT_ANYWHERE) == 0) *lloc = "all";
			else {
				char *title = pcb_strdup_printf("Unsupported group loc: %s", grp->name);
				pcb_io_incompat_save(pcb->Data, NULL, "stackup", title, "The group is omitted from the output.");
				free(title);
				continue;
			}
		}
		else
			*lloc = "virtual";
		return t;
	}
	return NULL;
}

static int tedax_layer_set_by_str(pcb_board_t *pcb, pcb_layergrp_t *grp, const char *lloc, const char *typename)
{
	const tedax_layer_t *t;

	grp->ltype = 0;
	if (strcmp(lloc, "top") == 0) grp->ltype |= PCB_LYT_TOP;
	else if (strcmp(lloc, "inner") == 0) grp->ltype |= PCB_LYT_INTERN;
	else if (strcmp(lloc, "bottom") == 0) grp->ltype |= PCB_LYT_BOTTOM;
	else if (strcmp(lloc, "virtual") == 0) grp->ltype |= PCB_LYT_VIRTUAL;
	else if (strcmp(lloc, "all") == 0) {}
	else
		pcb_message(PCB_MSG_ERROR, "invalid layer location: %s\n", lloc);

	for(t = layertab; t->typename != NULL; t++) {
		if (strcmp(typename, t->typename) == 0) {
			grp->ltype |= t->type;
			grp->purpose = NULL;
			if (t->purpose != NULL)
				pcb_layergrp_set_purpose(grp, t->purpose);
			return 0;
		}
	}

	pcb_message(PCB_MSG_ERROR, "invalid layer type: %s\n", typename);
	return -1;
}

int tedax_stackup_fsave(tedax_stackup_t *ctx, pcb_board_t *pcb, const char *stackid, FILE *f)
{
	int prefix = 0;
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *grp;

	fprintf(f, "begin stackup v1 ");
	tedax_fprint_escape(f, stackid);
	fputc('\n', f);

	vtp0_enlarge(&ctx->g2n, pcb->LayerGroups.len+1);
	for(gid = 0, grp = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,grp++) {
		char tname[66], *tn;
		const char *lloc;
		pcb_layer_t *ly = NULL;
		const tedax_layer_t *lt = tedax_layer_lookup_by_type(pcb, grp, &lloc);

		if (lt == NULL) {
			char *title = pcb_strdup_printf("Unsupported group: %s", grp->name);
			pcb_io_incompat_save(pcb->Data, NULL, "stackup", title, "Layer type/purpose/location is not supported by tEDAx, layer will be omitted from the save.");
			free(title);
			continue;
		}

		gen_layer_name(tname, grp->name, 0);
		if (htsp_has(&ctx->n2g, tname)) {
			prefix++;
			gen_layer_name(tname, grp->name, prefix);
		}

		if (grp->len > 1) {
			char *title = pcb_strdup_printf("Multilayer group: %s", grp->name);
			pcb_io_incompat_save(pcb->Data, NULL, "stackup", title, "All layers are merged into a single layer");
			free(title);
		}


		tn = pcb_strdup(tname);
		htsp_set(&ctx->n2g, tn, grp);
		vtp0_set(&ctx->g2n, gid, tn);

		fprintf(f, " layer %s %s %s\n", tn, lloc, lt->typename);

		if (grp->len > 0)
			ly = pcb_get_layer(PCB->Data, grp->lid[0]);
		if (ly != NULL)
			fprintf(f, "  lprop %s display-color #%02x%02x%02x\n", tn, ly->meta.real.color.r, ly->meta.real.color.g, ly->meta.real.color.b);
		save_user_props(pcb, f, grp, tn);
	}

	fprintf(f, "end stackup\n");
	return 0;
}

int tedax_stackup_save(pcb_board_t *pcb, const char *stackid, const char *fn)
{
	int res;
	FILE *f;
	tedax_stackup_t ctx;

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_stackup_save(): can't open %s for writing\n", fn);
		return -1;
	}
	tedax_stackup_init(&ctx);
	fprintf(f, "tEDAx v1\n");
	res = tedax_stackup_fsave(&ctx, pcb, stackid, f);
	fclose(f);
	tedax_stackup_uninit(&ctx);
	return res;
}

static pcb_layergrp_t *get_grp_by_name(tedax_stackup_t *ctx, pcb_board_t *pcb, const char *name)
{
	pcb_layergrp_t *grp = htsp_get(&ctx->n2g, name);

	if (grp == NULL) {
		char *nn;

		grp = pcb_get_grp_new_raw(pcb);
		grp->name = pcb_strdup(name);
		nn = pcb_strdup(name);
		htsp_set(&ctx->n2g, nn, grp);
		vtp0_set(&ctx->g2n, (grp - pcb->LayerGroups.grp), nn);
	}
	return grp;
}

int tedax_stackup_parse(tedax_stackup_t *ctx, pcb_board_t *pcb, FILE *f, char *buff, int buff_size, char *argv[], int argv_size)
{
	int argc;
	pcb_layers_reset(pcb);

	while((argc = tedax_getline(f, buff, buff_size, argv, argv_size)) >= 0) {
		pcb_layergrp_t *grp;
		if (strcmp(argv[0], "layer") == 0) {
			grp = get_grp_by_name(ctx, pcb, argv[1]);
			tedax_layer_set_by_str(pcb, grp, argv[2], argv[3]);
			if (!(grp->ltype & PCB_LYT_SUBSTRATE))
				pcb_layer_create(pcb, grp - pcb->LayerGroups.grp, pcb_strdup(argv[1]));

		}
		else if (strcmp(argv[0], "lprop") == 0) {
			grp = get_grp_by_name(ctx, pcb, argv[1]);
			if (strcmp(argv[2], "display-color") == 0) {
				if (grp->len > 0) {
					pcb_layer_t *ly = pcb_get_layer(pcb->Data, grp->lid[0]);
					if (ly != NULL)
						pcb_color_load_str(&ly->meta.real.color, argv[3]);
				}
			}
			else
				pcb_attribute_put(&grp->Attributes, argv[2], argv[3]);
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "stackup") == 0))
			break;
	}
	return 0;
}


int tedax_stackup_fload(tedax_stackup_t *ctx, pcb_board_t *pcb, FILE *f, const char *blk_id, int silent)
{
	char line[520];
	char *argv[16];

	if (tedax_seek_hdr(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	if (tedax_seek_block(f, "stackup", "v1", blk_id, silent, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	return tedax_stackup_parse(ctx, pcb, f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]));
}


int tedax_stackup_load(pcb_board_t *pcb, const char *fn, const char *blk_id, int silent)
{
	int res;
	FILE *f;
	tedax_stackup_t ctx;

	f = pcb_fopen(fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_stackup_load(): can't open %s for reading\n", fn);
		return -1;
	}
	tedax_stackup_init(&ctx);
	res = tedax_stackup_fload(&ctx, pcb, f, blk_id, silent);
	fclose(f);
	tedax_stackup_uninit(&ctx);
	return res;
}
