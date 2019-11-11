/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, io_mentor_cell, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

#include <stdlib.h>
#include <stdio.h>
#include <qparse/qparse.h>
#include <genvector/gds_char.h>
#include <genht/htsp.h>

#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "plug_io.h"
#include "error.h"
#include "safe_fs.h"

#include "obj_subc.h"

#define ltrim(s) while(isspace(*s)) (s)++

typedef struct node_s node_t;

struct node_s {
	char **argv;
	int argc;
	int level;
	node_t *parent, *next;
	node_t *first_child, *last_child;
};

typedef struct {
	node_t *root, *curr;
} hkp_tree_t;

typedef struct {
	node_t *subtree;
} hkp_shape_t;

typedef struct {
	node_t *subtree;
} hkp_hole_t;

typedef struct {
	node_t *subtree;
} hkp_pstk_t;


typedef struct {
	pcb_board_t *pcb;

	const pcb_unit_t *unit;      /* default unit used while converting coords any given time */
	const pcb_unit_t *pstk_unit; /* default unit for the padstacks file */

	htsp_t shapes;   /* name -> hkp_shape_t */
	htsp_t holes;    /* name -> hkp_hole_t */
	htsp_t pstks;    /* name -> hkp_pstk_t */
	hkp_tree_t layout, padstacks;
} hkp_ctx_t;

/*** High level parser ***/

/* Return the idxth sibling with matching name */
static node_t *find_nth(node_t *nd, char *name, int idx)
{
	for(; nd != NULL; nd = nd->next) {
		if (strcmp(nd->argv[0], name) == 0) {
			if (idx == 0)
				return nd;
			idx--;
		}
	}
	return NULL;
}

/* parse a string it into a coord - modifies s; returns 0 on success */
static int parse_coord(hkp_ctx_t *ctx, char *s, pcb_coord_t *crd)
{
	char *end;
	pcb_bool suc;

	end = strchr(s, ',');
	if (end != NULL)
		*end = '\0';

	*crd = pcb_get_value(s, ctx->unit->suffix, NULL, &suc);
	return !suc;
}

/* split s and parse  it into (x,y) - modifies s */
static int parse_xy(hkp_ctx_t *ctx, char *s, pcb_coord_t *x, pcb_coord_t *y)
{
	char *sy;
	pcb_bool suc1, suc2;

	sy = strchr(s, ',');
	if (sy == NULL)
		return -1;

	*sy = '\0';
	sy++;

	*x = pcb_get_value(s, ctx->unit->suffix, NULL, &suc1);
	*y = pcb_get_value(sy, ctx->unit->suffix, NULL, &suc2);

	return !(suc1 && suc2);
}

#if 0
static void parse_pstk(hkp_ctx_t *ctx, pcb_subc_t *subc, char *ps, pcb_coord_t px, pcb_coord_t py, char *name)
{
	pcb_flag_t flags = pcb_no_flags();
	pcb_coord_t thickness, hole, ms;
	char *curr, *next;
	int sqpad_pending = 0;

	thickness = 0;
	hole = 0;
	ms = 0;

	for(curr = ps; curr != NULL; curr = next) {
		next = strchr(curr, ',');
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		ltrim(curr);
		if (strncmp(curr, "Pad", 3) == 0) {
			curr += 4;
			ltrim(curr);
			if (strncmp(curr, "Square", 6) == 0) {
				curr += 6;
				ltrim(curr);
				flags = pcb_flag_add(flags, PCB_FLAG_SQUARE);
				thickness = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
				sqpad_pending = 1;
			}
			if (strncmp(curr, "Rectangle", 9) == 0) {
				char *hs;
				pcb_coord_t w, h;
				curr += 9;
				ltrim(curr);
				thickness = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
				hs = strchr(curr, 'x');
				if (hs == NULL) {
					pcb_message(PCB_MSG_ERROR, "Broken Rectangle pad: no 'x' in size\n");
					return;
				}
				*hs = '\0';
				hs++;
				w = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
				h = pcb_get_value(hs, ctx->unit->suffix, NULL, NULL);
TODO("padstack: rewrite")
#if 0
				pcb_element_pad_new_rect(elem, px+w/2, py+h/2, px-w/2, py-h/2, cl, ms, name, name, flags);
#else
				(void)w; (void)h;
#endif
			}
			else if (strncmp(curr, "Round", 5) == 0) {
				curr += 5;
				ltrim(curr);
				thickness = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
			}
			else if (strncmp(curr, "Oblong", 6) == 0) {
				pcb_message(PCB_MSG_ERROR, "Ignoring oblong pin - not yet supported\n");
				return;
			}
			else {
				pcb_message(PCB_MSG_ERROR, "Ignoring unknown pad shape: %s\n", curr);
				return;
			}
		}
		else if (strncmp(curr, "Hole Rnd", 8) == 0) {
			curr += 8;
			ltrim(curr);
			hole = pcb_get_value(curr, ctx->unit->suffix, NULL, NULL);
		}
	}

TODO("padstack: rewrite")
#if 0
	if (hole > 0) {
		pcb_element_pin_new(elem, px, py, thickness, cl, ms, hole, name, name, flags);
		sqpad_pending = 0;
	}

	if (sqpad_pending)
		pcb_element_pad_new_rect(elem, px, py, px+thickness, py+thickness, cl, ms, name, name, flags);
#else
	(void)sqpad_pending; (void)ms; (void)hole; (void)thickness;
#endif
}
#endif

#if 0
static void parse_pin(hkp_ctx_t *ctx, pcb_subc_t *subc, node_t *nd)
{
	node_t *tmp;
	pcb_coord_t px, py;

	tmp = find_nth(nd->first_child, "XY", 0);
	if (tmp == NULL) {
		pcb_message(PCB_MSG_ERROR, "Ignoring pin with no coords\n");
		return;
	}
	parse_xy(ctx, tmp->argv[1], &px, &py);

	tmp = find_nth(nd->first_child, "PADSTACK", 0);
	if (tmp != NULL)
		parse_pstk(ctx, subc, tmp->argv[1], px, py, nd->argv[1]);
}
#endif

#if 0
static void parse_silk(hkp_ctx_t *ctx, pcb_subc_t *subc, node_t *nd)
{
	node_t *tmp, *pp;

	tmp = find_nth(nd->first_child, "SIDE", 0);
	if (tmp != NULL) {
		if (strcmp(tmp->argv[1], "MNT_SIDE") != 0) {
			pcb_message(PCB_MSG_ERROR, "Ignoring subcircuit silk line not on mount-side\n");
			return;
		}
	}
	pp = find_nth(nd->first_child, "POLYLINE_PATH", 0);
	if (pp != NULL) {
		pcb_coord_t th, px, py, x, y;
		int n;
		th = PCB_MM_TO_COORD(0.5);
		tmp = find_nth(pp->first_child, "WIDTH", 0);
		if (tmp != NULL)
			pcb_get_value(tmp->argv[1], ctx->unit->suffix, NULL, NULL);
		tmp = find_nth(pp->first_child, "XY", 0);
		parse_xy(ctx, tmp->argv[1], &px, &py);
		for(n = 2; n < tmp->argc; n++) {
			parse_xy(ctx, tmp->argv[n], &x, &y);
TODO("subc: rewrite this for subcircuits")
#if 0
			pcb_element_line_new(elem, px, py, x, y, th);
#else
	(void)th;
#endif
			px = x;
			py = y;
		}
	}
}
#endif

static pcb_subc_t *parse_package(hkp_ctx_t *ctx, pcb_data_t *dt, node_t *nd)
{
#if 0
	static const char *tagnames[] = { "PACKAGE_GROUP", "MOUNT_TYPE", "NUMBER_LAYERS", "TIMESTAMP", NULL };
	const char **t;
#endif
	pcb_subc_t *subc;
	char *end, *desc = "", *refdes = "", *value = "";
	node_t *n, *tt, *attr, *tmp;
	pcb_coord_t ox, oy, tx = 0, ty = 0;
	double rot = 0;
	int on_bottom = 0, seen_oxy = 0;

	/* extract global */
	for(n = nd->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "DESCRIPTION") == 0) desc = n->argv[1];
		else if (strcmp(n->argv[0], "XY") == 0) {
			if ((parse_coord(ctx, n->argv[1], &ox) != 0) || (parse_coord(ctx, n->argv[2], &oy) != 0)) {
				pcb_message(PCB_MSG_ERROR, "Can't load package: broken placement XY coord\n");
				return NULL;
			}
			seen_oxy = 1;
		}
		else if (strcmp(n->argv[0], "ROTATION") == 0) {
			/* hkp rotation: top side positive value is CW; in pcb-rnd: that's negative */
			rot = -strtod(n->argv[1], &end);
			if (*end != '\0') {
				pcb_message(PCB_MSG_ERROR, "Can't load package: broken placement rotation value (expected numeric)\n");
				return NULL;
			}
			if ((rot < -360) || (rot > 360)) {
				pcb_message(PCB_MSG_ERROR, "Can't load package: broken placement rotation value (out of range)\n");
				return NULL;
			}
		}
		else if (strcmp(n->argv[0], "FACEMENT") == 0) {
			if (strcmp(n->argv[1], "TOP") == 0) on_bottom = 0;
			else if (strcmp(n->argv[1], "BOTTOM") == 0) on_bottom = 1;
			else {
				pcb_message(PCB_MSG_ERROR, "Can't load package: broken facement (should be TOP or BOTTOM)\n");
				return NULL;
			}
		}
		else if (strcmp(n->argv[0], "TEXT") == 0) {
			tt = find_nth(n, "TEXT_TYPE", 0);
			if ((tt != NULL) && (strcmp(tt->argv[2], "REF_DES") == 0)) {
				attr = find_nth(tt, "DISPLAY_ATTR", 0);
				if (attr != NULL) {
					tmp = find_nth(attr, "XY", 0);
					if (tmp != NULL)
						parse_xy(ctx, tmp->argv[1], &tx, &ty);
				}
			}
		}
	}

	if (!seen_oxy) {
		pcb_message(PCB_MSG_ERROR, "Can't load package: no placement XY coord\n");
		return NULL;
	}

	subc = pcb_subc_alloc();
	if (dt != NULL) {
		pcb_subc_reg(dt, subc);
		pcb_obj_id_reg(dt, subc);

		/* bind the via rtree so that vias added in this subc show up on the board */
		if (ctx->pcb != NULL)
			pcb_subc_bind_globals(ctx->pcb, subc);
	}

	pcb_subc_create_aux(subc, ox, oy, rot, on_bottom);

#if 0
	elem = pcb_element_new(ctx->pcb->Data, NULL, pcb_font(ctx->pcb, 0, 1),
		flags, desc, refdes, value, tx, ty, dir, 100, flags, 0);
#else
	(void)desc; (void)refdes; (void)value;
#endif

#if 0
	/* extract pins and silk lines */
	for(n = nd->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "PIN") == 0)
			parse_pin(ctx, subc, n);
		else if (strcmp(n->argv[0], "SILKSCREEN_OUTLINE") == 0)
			parse_silk(ctx, subc, n);
	}
#endif

#if 0
	/* extract tags */
	for(n = nd->first_child; n != NULL; n = n->next) {
		for(t = tagnames; *t != NULL; t++) {
			if (strcmp(n->argv[0], *t) == 0) {
/*				printf("TAG %s=%s\n", *t, n->argv[1]); */
			}
		}
	}
#endif


	if ((dt != NULL) && (ctx->pcb != NULL)) {
		pcb_subc_rebind(ctx->pcb, subc);
	}
	return subc;
}

static const pcb_unit_t *parse_units(const char *ust)
{
	if (strcmp(ust, "MIL") == 0) return get_unit_struct("mil");
	if (strcmp(ust, "TH") == 0)  return get_unit_struct("mil");
	if (strcmp(ust, "MM") == 0)  return get_unit_struct("mm");
	if (strcmp(ust, "IN") == 0)  return get_unit_struct("inch");
	return NULL;
}

static int parse_layout_root(hkp_ctx_t *ctx, hkp_tree_t *tree)
{
	node_t *n;

	/* extract globals */
	for(n = tree->root->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "UNITS") == 0) {
			ctx->unit = parse_units(n->argv[1]);
			if (ctx->unit == NULL) {
				pcb_message(PCB_MSG_ERROR, "Unkown unit '%s'\n", n->argv[1]);
				return -1;
			}
		}
	}

	pcb_layergrp_upgrade_to_pstk(ctx->pcb);

	/* build packages */
	for(n = tree->root->first_child; n != NULL; n = n->next)
		if (strcmp(n->argv[0], "PACKAGE_CELL") == 0)
			parse_package(ctx, ctx->pcb->Data, n);

	return 0;
}


/*** Low level parser ***/

static void dump(node_t *nd)
{
	int n;
	for(n = 0; n < nd->level; n++)
		printf(" ");
	if (nd->argc > 0) {
		printf("%s", nd->argv[0]);
		for(n = 1; n < nd->argc; n++)
			printf("|%s", nd->argv[n]);
	}
	else
		printf("<empty>");
	printf("\n");
	for(nd = nd->first_child; nd != NULL; nd = nd->next)
		dump(nd);
}

static void destroy(node_t *nd)
{
	node_t *n, *next;

	if (nd == NULL)
		return;

	qparse_free(nd->argc, &nd->argv);

	for(n = nd->first_child; n != NULL; n = next) {
		next = n->next;
		destroy(n);
	}

	free(nd);
}

/* Split up a virtual line and save it in the tree */
void save_vline(hkp_tree_t *tree, char *vline, int level)
{
	node_t *nd;

	nd = calloc(sizeof(node_t), 1);
	nd->argc = qparse2(vline, &nd->argv, QPARSE_DOUBLE_QUOTE | QPARSE_PAREN | QPARSE_MULTISEP);
	nd->level = level;

	if (level == tree->curr->level) { /* sibling */
		sibling:;
		nd->parent = tree->curr->parent;
		tree->curr->next = nd;
		nd->parent->last_child = nd;
	}
	else if (level == tree->curr->level+1) { /* first child */
		tree->curr->first_child = tree->curr->last_child = nd;
		nd->parent = tree->curr;
	}
	else if (level < tree->curr->level) { /* step back to a previous level */
		while(level < tree->curr->level) tree->curr = tree->curr->parent;
		goto sibling;
	}
	tree->curr = nd;
}

static void rtrim(gds_t *s)
{
	int n;
	for(n = gds_len(s)-1; (n >= 0) && isspace(s->array[n]); n--)
		s->array[n] = '\0';
}

static void load_hkp(hkp_tree_t *tree, FILE *f)
{
	char *s, line[1024];
	gds_t vline;
	int level;

	tree->curr = tree->root = calloc(sizeof(node_t), 1);
	gds_init(&vline);

	/* read physical lines, build virtual lines and save them in the tree*/
	while(fgets(line, sizeof(line), f) != NULL) {
		s = line;
		while(isspace(*s)) s++;

		/* first char is '.' means it's a new virtual line */
		if (*s == '.') {
			if (gds_len(&vline) > 0) {
				rtrim(&vline);
				save_vline(tree, vline.array, level);
				gds_truncate(&vline, 0);
			}
			level = 0;
			while(*s == '.') { s++; level++; };
		}

		if (gds_len(&vline) > 0)
			gds_append(&vline, ' ');
		gds_append_str(&vline, s);
	}

	/* the last virtual line before eof */
	if (gds_len(&vline) > 0) {
		rtrim(&vline);
		save_vline(tree, vline.array, level);
	}
	gds_uninit(&vline);

}

static int io_mentor_cell_pstks(hkp_ctx_t *ctx, const char *fn)
{
	FILE *fpstk;
	node_t *n;

	fpstk = pcb_fopen(&PCB->hidlib, fn, "r");
	if (fpstk == NULL)
		return -1;

	load_hkp(&ctx->padstacks, fpstk);

	/* build the indices */
	for(n = ctx->padstacks.root->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "UNITS") == 0) {
			ctx->pstk_unit = parse_units(n->argv[1]);
			if (ctx->pstk_unit == NULL) {
				pcb_message(PCB_MSG_ERROR, "Unkown unit '%s'\n", n->argv[1]);
				return -1;
			}
		}
		else if (strcmp(n->argv[0], "PAD") == 0) {}
		else if (strcmp(n->argv[0], "HOLE") == 0) {}
		else if (strcmp(n->argv[0], "PADSTACK") == 0) {}
	}

	TODO("parse padstacks");

	fclose(fpstk);
	return 0;
}

int io_mentor_cell_read_pcb(pcb_plug_io_t *pctx, pcb_board_t *pcb, const char *fn, conf_role_t settings_dest)
{
	hkp_ctx_t ctx;
	int res = -1;
	FILE *flay;
	char *end, fn2[PCB_PATH_MAX];

	memset(&ctx, 0, sizeof(ctx));

	flay = pcb_fopen(&PCB->hidlib, fn, "r");
	if (flay == NULL)
		goto err;

	/* create the file name for the padstacks */
	strncpy(fn2, fn, PCB_PATH_MAX);
	fn2[PCB_PATH_MAX-1] = '\0';
	end = strchr(fn2, PCB_DIR_SEPARATOR_C);
	if (end == NULL)
		end = fn2;
	else
		end++;
	strcpy(end, "Padstack.hkp");

	if (io_mentor_cell_pstks(&ctx, fn2) != 0) {
		fclose(flay);
		goto err;
	}

	ctx.pcb = pcb;
	ctx.unit = get_unit_struct("mm");

	load_hkp(&ctx.layout, flay);
	fclose(flay);

	/* we are loading the cells into a board, make a default layer stack for that */
	pcb_layergrp_inhibit_inc();
	pcb_layers_reset(pcb);
	pcb_layer_group_setup_default(pcb);
	pcb_layer_group_setup_silks(pcb);
	pcb_layer_auto_fixup(pcb);
	pcb_layergrp_inhibit_dec();

	/* parse the root */
	res = parse_layout_root(&ctx, &ctx.layout);

	pcb_layer_colors_from_conf(pcb, 1);

	res = 0; /* all ok */

	err:;
	if (res != 0) {
		if (ctx.layout.root != NULL) {
			printf("### layout tree:\n");
			dump(ctx.layout.root);
		}
		if (ctx.padstacks.root != NULL) {
			printf("### padstack tree:\n");
			dump(ctx.padstacks.root);
		}
	}
	destroy(ctx.padstacks.root);
	destroy(ctx.layout.root);

	return res;
}

int io_mentor_cell_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[1024], *s;

	while(!(feof(f))) {
		if (fgets(line, sizeof(line), f) != NULL) {
			s = line;
			while(isspace(*s)) s++; /* strip leading whitespace */
			if (strncmp(s, ".FILETYPE LAYOUT", 16) == 0) /* valid root */
				return 1;
			if ((*s == '\r') || (*s == '\n') || (*s == '\0')) /* ignore empty lines and comments */
				continue;
			/* non-comment, non-empty line - and we don't have our root -> it's not an s-expression */
			return 0;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;
}
