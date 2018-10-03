/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  dsn IO plugin - file format read, parser
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <gensexpr/gsxl.h>
#include <genht/htsp.h>

#include "board.h"
#include "data.h"
#include "plug_io.h"
#include "error.h"
#include "pcb_bool.h"
#include "safe_fs.h"
#include "compat_misc.h"
#include "layer_grp.h"
#include "conf_core.h"

#include "read.h"

typedef struct {
	gsxl_dom_t dom;
	pcb_board_t *pcb;
	const pcb_unit_t *unit;
	pcb_box_t bbox; /* board's bbox from the boundary subtrees, in the file's coordinate system */
	htsp_t name2layer;
	unsigned has_pcb_boundary:1;
} dsn_read_t;

static char *STR(gsxl_node_t *node)
{
	if (node == NULL)
		return NULL;
	return node->str;
}

static char *STRE(gsxl_node_t *node)
{
	if (node == NULL)
		return "";
	if (node->str == NULL)
		return "";
	return node->str;
}

/* check if node is named name and if so, save the node in nname for
   later reference; assumes node->str is not NULL */
#define if_save_uniq(node, name) \
	if (pcb_strcasecmp(node->str, #name) == 0) { \
		if (n ## name != NULL) { \
			pcb_message(PCB_MSG_ERROR, "Multiple " #name " nodes where only one is expected (at %ld:%ld)\n", (long)node->line, (long)node->col); \
			return -1; \
		} \
		n ## name = node; \
	}

static pcb_coord_t COORD(dsn_read_t *ctx, gsxl_node_t *n)
{
	char *end, *s = STRE(n);
	double v = strtod(s, &end);

	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "Invalid coord: '%s' (at %ld:%ld)\n", s, (long)n->line, (long)n->col);
		return 0;
	}
	v /= ctx->unit->scale_factor;
	if (ctx->unit->family == PCB_UNIT_METRIC)
		return PCB_MM_TO_COORD(v);
	return PCB_MIL_TO_COORD(v);
}

#define COORDX(ctx, n) COORD(ctx, n)
#define COORDY(ctx, n) (ctx->bbox.Y2 - COORD(ctx, n))

static const pcb_unit_t *push_unit(dsn_read_t *ctx, gsxl_node_t *nu)
{
	const pcb_unit_t *old = ctx->unit;
	char *su, *s;

	if ((nu == NULL) || (nu->children == NULL))
		return ctx->unit;

	for(su = s = STRE(gsxl_children(nu)); *s != '\0'; s++)
		*s = tolower(*s);

	ctx->unit = get_unit_struct(su);
	if (ctx->unit == NULL) {
		pcb_message(PCB_MSG_ERROR, "Invalid unit: '%s' (at %ld:%ld)\n", su, (long)nu->line, (long)nu->col);
		return NULL;
	}

	return old;
}

static void pop_unit(dsn_read_t *ctx, const pcb_unit_t *saved)
{
	ctx->unit = saved;
}

/*** tree parse ***/

static void parse_attribute(dsn_read_t *ctx, pcb_attribute_list_t *attr, gsxl_node_t *kv)
{
	for(;kv != NULL; kv = kv->next)
		pcb_attribute_put(attr, STRE(kv), STRE(kv->children));
}

static int dsn_parse_rect(dsn_read_t *ctx, pcb_box_t *dst, gsxl_node_t *src, int no_y_flip)
{
	pcb_coord_t x, y;

	if (src == NULL) {
		pcb_message(PCB_MSG_ERROR, "Missing coord in rect\n");
		return -1;
	}

	/* set all corners to first x;y */
	dst->X1 = dst->X2 = COORDX(ctx, src);
	if (src->next == NULL) goto err;
	src = src->next;
	if (no_y_flip)
		dst->Y1 = dst->Y2 = COORD(ctx, src);
	else
		dst->Y1 = dst->Y2 = COORDY(ctx, src);
	if (src->next == NULL) goto err;
	src = src->next;

	/* bump with second x;y */
	x = COORDX(ctx, src);
	if (src->next == NULL) goto err;
	src = src->next;
	if (no_y_flip)
		y = COORD(ctx, src);
	else
		y = COORDY(ctx, src);
	pcb_box_bump_point(dst, x, y);
	return 0;

	err:;
	pcb_message(PCB_MSG_ERROR, "Missing coord in rect (at %ld:%ld)\n", (long)src->line, (long)src->col);
	return -1;
}


static int dsn_parse_rule(dsn_read_t *ctx, gsxl_node_t *bnd)
{
#warning TODO
	return 0;
}

static void boundary_line(pcb_layer_t *oly, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t aper)
{
	if (aper <= 0)
		aper = 1;
	pcb_line_new(oly, x1, y1, x2, y2, aper, 0, pcb_no_flags());
}

static int dsn_parse_boundary_(dsn_read_t *ctx, gsxl_node_t *bnd, int do_bbox, pcb_layer_t *oly)
{
	gsxl_node_t *b, *n;
	for(bnd = bnd->children; bnd != NULL; bnd = bnd->next) {
		if (bnd->str == NULL)
			continue;
		if (pcb_strcasecmp(bnd->str, "path") == 0) {
			pcb_coord_t x, y, lx, ly, fx, fy, aper;
			int len;

			b = gsxl_children(bnd);
			if (!do_bbox && (pcb_strcasecmp(STRE(b), "pcb") == 0)) {
				pcb_message(PCB_MSG_ERROR, "PCB boundary shall be a rect, not a path;\naccepting the path, but other software may choke on this file\n");
				ctx->has_pcb_boundary = 1;
			}
			if ((b->next == NULL) || (b->next->next == NULL)) {
				pcb_message(PCB_MSG_ERROR, "not enough arguments for boundary poly (at %ld:%ld)\n", (long)b->line, (long)b->col);
				return -1;
			}

			aper = COORD(ctx, b->next);

			for(len = 0, n = b->next->next; n != NULL; len++) {
				x = COORDX(ctx, n);
				if (n->next == NULL) {
					pcb_message(PCB_MSG_ERROR, "Not enough coordinate values (missing y)\n");
					break;
				}
				n = n->next;
				if (do_bbox)
					y = COORD(ctx, n);
				else
					y = COORDY(ctx, n);
				n = n->next;
				if (!do_bbox) {
					if (len == 0) {
						fx = x;
						fy = y;
					}
					else
						boundary_line(oly, lx, ly, x, y, aper);
					lx = x;
					ly = y;
				}
				else
					pcb_box_bump_point(&ctx->bbox, x, y);
			}
			if (!do_bbox && (x != fx) && (y != fy)) /* close the boundary */
				boundary_line(oly, lx, ly, x, y, aper);
		}
		else if (pcb_strcasecmp(bnd->str, "rect") == 0) {
			pcb_box_t box;

			b = gsxl_children(bnd);
			if ((b->next == NULL) || (b->next->next == NULL)) {
				pcb_message(PCB_MSG_ERROR, "not enough arguments for boundary rect (at %ld:%ld)\n", (long)b->line, (long)b->col);
				return -1;
			}
			if (pcb_strcasecmp(STRE(b), "pcb") == 0)
				ctx->has_pcb_boundary = 1;
			if (dsn_parse_rect(ctx, &box, b->next, do_bbox) != 0)
				return -1;
			if (!do_bbox) {
				boundary_line(oly, box.X1, box.Y1, box.X1, box.Y2, 0);
				boundary_line(oly, box.X1, box.Y2, box.X2, box.Y2, 0);
				boundary_line(oly, box.X2, box.Y2, box.X1, box.Y2, 0);
				boundary_line(oly, box.X1, box.Y2, box.X1, box.Y1, 0);
			}
			else
				pcb_box_bump_box(&ctx->bbox, &box);
		}
		else if (pcb_strcasecmp(bnd->str, "rule") == 0) {
			if (!do_bbox && (dsn_parse_rule(ctx, bnd) != 0))
				return -1;
		}
	}
	return 0;
}

static int dsn_parse_boundary(dsn_read_t *ctx, gsxl_node_t *bnd)
{
	int res;
	pcb_layer_id_t olid;
	pcb_layer_t *oly;

	if (bnd == NULL)
		return 0;

	if (pcb_layer_list(ctx->pcb, PCB_LYT_BOUNDARY, &olid, 1) < 1) {
		pcb_message(PCB_MSG_ERROR, "Intenal error: no boundary layer found\n");
		return -1;
	}
	oly = pcb_get_layer(ctx->pcb->Data, olid);


	res = dsn_parse_boundary_(ctx, bnd, 1, oly);
	res |= dsn_parse_boundary_(ctx, bnd, 0, oly);
	if (res != 0)
		return -1;


	return 0;
}

static int parse_layer_type(dsn_read_t *ctx, pcb_layergrp_t *grp, const char *ty)
{
	if ((pcb_strcasecmp(ty, "signal") == 0) || (pcb_strcasecmp(ty, "jumper") == 0))
		return 0; /* nothig special to do */
	if ((pcb_strcasecmp(ty, "power") == 0) || (pcb_strcasecmp(ty, "mixed") == 0)) {
#warning TODO: place poly
		return 0;
	}

	pcb_message(PCB_MSG_WARNING, "Ignoring unknown layer type '%s' for %s\n", ty, grp->name);
	return 0;
}

#define CHECK_TOO_MANY_LAYERS(node, num) \
do { \
	if (num >= PCB_MAX_LAYERGRP) { \
		pcb_message(PCB_MSG_ERROR, "Too many layer groups in the layer stack (at %ld:%ld)\n", (long)node->line, (long)node->col); \
		return -1; \
	} \
} while(0)

static int dsn_parse_structure(dsn_read_t *ctx, gsxl_node_t *str)
{
	const pcb_dflgmap_t *m;
	gsxl_node_t *n, *i;
	pcb_layergrp_t *topcop = NULL, *botcop = NULL;

	if (str == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not parse board without a structure subtree\n");
		return -1;
	}

	pcb_layergrp_inhibit_inc();
	for(m = pcb_dflgmap; m <= pcb_dflgmap_last_top_noncopper; m++) {
		CHECK_TOO_MANY_LAYERS(str, ctx->pcb->LayerGroups.len);
		pcb_layergrp_set_dflgly(ctx->pcb, &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++], m, NULL, NULL);
	}

	for(n = str->children; n != NULL; n = n->next) {
		if (n->str == NULL)
			continue;
		else if (pcb_strcasecmp(n->str, "layer") == 0) {
			pcb_layer_t *ly;

			if (botcop != NULL) {
				CHECK_TOO_MANY_LAYERS(n, ctx->pcb->LayerGroups.len);
				pcb_layergrp_set_dflgly(ctx->pcb, &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++], &pcb_dflg_substrate, NULL, NULL);
			}
			CHECK_TOO_MANY_LAYERS(n, ctx->pcb->LayerGroups.len);
			botcop = &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++];
			if (topcop == NULL)
				topcop = botcop;
			pcb_layergrp_set_dflgly(ctx->pcb, botcop, &pcb_dflg_int_copper, STR(gsxl_children(n)), STR(gsxl_children(n)));

			ly = pcb_get_layer(ctx->pcb->Data, botcop->lid[0]);
			if (ly == NULL) {
				pcb_message(PCB_MSG_ERROR, "io_dsn internal error: no layer in group\n");
				return -1;
			}
			htsp_set(&ctx->name2layer, (char *)ly->name, ly);

			if (n->children != NULL) {
				for(i = n->children->next; i != NULL; i = i->next) {
					if (pcb_strcasecmp(i->str, "type") == 0) {
						if (parse_layer_type(ctx, botcop, STRE(i->children)) != 0)
							return -1;
					}
					else if (pcb_strcasecmp(i->str, "property") == 0) {
						parse_attribute(ctx, &botcop->Attributes, i->children);
					}
				}
			}
		}
	}

	if (topcop == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not parse board without a copper layers\n");
		return -1;
	}

	topcop->ltype = PCB_LYT_TOP | PCB_LYT_COPPER;
	botcop->ltype = PCB_LYT_BOTTOM | PCB_LYT_COPPER;

	CHECK_TOO_MANY_LAYERS(str, ctx->pcb->LayerGroups.len);
	pcb_layergrp_set_dflgly(ctx->pcb, &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++], &pcb_dflg_outline, NULL, NULL);

	for(m = pcb_dflgmap_first_bottom_noncopper; m->name != NULL; m++) {
		CHECK_TOO_MANY_LAYERS(str, ctx->pcb->LayerGroups.len);
		pcb_layergrp_set_dflgly(ctx->pcb, &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++], m, NULL, NULL);
	}

	pcb_layergrp_inhibit_dec();

	ctx->has_pcb_boundary = 0;
	for(n = str->children; n != NULL; n = n->next) {
		if (n->str == NULL)
			continue;
		else if (pcb_strcasecmp(n->str, "boundary") == 0) {
			if (dsn_parse_boundary(ctx, n) != 0)
				return -1;
		}
	}

	if ((ctx->bbox.X1 < 0) || (ctx->bbox.Y1 < 0))
		pcb_message(PCB_MSG_WARNING, "Negative coordinates on input - you may want to execute autocrop()\n");

	ctx->pcb->MaxWidth = ctx->bbox.X2 - ctx->bbox.X1;
	ctx->pcb->MaxHeight = ctx->bbox.Y2 - ctx->bbox.Y1;

#warning TODO: make up the boundary later on from bbox
	if (!ctx->has_pcb_boundary)
		pcb_message(PCB_MSG_ERROR, "Missing pcb boundary; every dsn design must have a pcb boundary.\ntrying to make up one using the bounding box.\n");

	return 0;
}

#define DSN_PARSE_NET(ly, net, fail) \
do { \
	gsxl_node_t *__net__ = (net); \
	const char *__nname__ = STRE(__net__); \
	ly = htsp_get(&ctx->name2layer, __nname__); \
	if (ly == NULL) { \
		pcb_message(PCB_MSG_ERROR, "Invalid/unknown net '%s' (at %ld:%ld)\n", __nname__, (long)__net__->line, (long)__net__->col); \
		{ fail; } \
	} \
} while(0)

static int dsn_parse_wire_poly(dsn_read_t *ctx, gsxl_node_t *wrr)
{
#warning TODO
return 0;
}

static int dsn_parse_wire_rect(dsn_read_t *ctx, gsxl_node_t *wrr)
{
	pcb_box_t box;
	gsxl_node_t *net = wrr->children;
	pcb_layer_t *ly;

	DSN_PARSE_NET(ly, net, return -1);

	if (dsn_parse_rect(ctx, &box, net->next, 0) != 0)
		return -1;

	pcb_poly_new_from_rectangle(ly, box.X1, box.Y1, box.X2, box.Y2, conf_core.design.clearance, pcb_flag_make(PCB_FLAG_CLEARPOLYPOLY));

	return 0;
}

static int dsn_parse_wire_path(dsn_read_t *ctx, gsxl_node_t *wrr)
{
	gsxl_node_t *n, *net = wrr->children;
	pcb_layer_t *ly;
	pcb_coord_t aper;
	pcb_coord_t x, y, px, py;
	int len = 0;

	DSN_PARSE_NET(ly, net, return -1);

	if ((net->next == NULL) || (net->next->next == NULL)) {
		pcb_message(PCB_MSG_ERROR, "Not enogh wire path attributes (at %ld:%ld)\n", (long)wrr->line, (long)wrr->col);
		return -1;
	}

	aper = COORD(ctx, net->next);

	for(n = net->next->next; n != NULL;) {
		x = COORDX(ctx, n);
		if (n->next == NULL) {
			pcb_message(PCB_MSG_ERROR, "Not enough coordinate values (missing y)\n");
			break;
		}
		n = n->next;
		y = COORDY(ctx, n);
		n = n->next;
		if (len > 0)
			pcb_line_new(ly, px, py, x, y, aper, conf_core.design.clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
		len++;
		px = x;
		py = y;
	}

	return 0;
}

static int dsn_parse_wire_qarc(dsn_read_t *ctx, gsxl_node_t *wrr)
{
#warning TODO
return 0;
}

static int dsn_parse_wire_circle(dsn_read_t *ctx, gsxl_node_t *wrr)
{
#warning TODO
return 0;
}


static int dsn_parse_wire(dsn_read_t *ctx, gsxl_node_t *wrr)
{
	/* pick up properties */
/* These are specified but not handled by pcb-rnd: /
	for(wrr = wrr->children; wrr != NULL; wrr = wrr->next) {
		if (wrr->str == NULL)
			continue;
		if (pcb_strcasecmp(wrr->str, "type")) { }
		else if (pcb_strcasecmp(wrr->str, "attr") == 0) { }
		else if (pcb_strcasecmp(wrr->str, "net") == 0) { }
		else if (pcb_strcasecmp(wrr->str, "turret") == 0) { }
		else if (pcb_strcasecmp(wrr->str, "shield") == 0) { }
		else if (pcb_strcasecmp(wrr->str, "connect") == 0) { }
		else if (pcb_strcasecmp(wrr->str, "supply") == 0) { }
	}
*/

	/* draw the actual objects */
	for(wrr = wrr->children; wrr != NULL; wrr = wrr->next) {
		if (wrr->str == NULL)
			continue;
		if (pcb_strcasecmp(wrr->str, "poly") == 0) {
			if (dsn_parse_wire_poly(ctx, wrr) != 0)
				return -1;
		}
		else if (pcb_strcasecmp(wrr->str, "path") == 0) {
			if (dsn_parse_wire_path(ctx, wrr) != 0)
				return -1;
		}
		else if (pcb_strcasecmp(wrr->str, "qarc") == 0) {
			if (dsn_parse_wire_qarc(ctx, wrr) != 0)
				return -1;
		}
		else if (pcb_strcasecmp(wrr->str, "rect") == 0) {
			if (dsn_parse_wire_rect(ctx, wrr) != 0)
				return -1;
		}
		else if (pcb_strcasecmp(wrr->str, "circle") == 0) {
			if (dsn_parse_wire_circle(ctx, wrr) != 0)
				return -1;
		}
	}
	return 0;
}

static int dsn_parse_via(dsn_read_t *ctx, gsxl_node_t *wrr)
{
#warning TODO
return 0;
}

static int dsn_parse_wiring(dsn_read_t *ctx, gsxl_node_t *wrr)
{
	for(wrr = wrr->children; wrr != NULL; wrr = wrr->next) {
		if (wrr->str == NULL)
			continue;
		if (pcb_strcasecmp(wrr->str, "wire") == 0) {
			if (dsn_parse_wire(ctx, wrr) != 0)
				return -1;
		}
		else if (pcb_strcasecmp(wrr->str, "via") == 0) {
			if (dsn_parse_via(ctx, wrr) != 0)
				return -1;
		}
		else if (pcb_strcasecmp(wrr->str, "bond") == 0) {
			pcb_message(PCB_MSG_WARNING, "unhandled bond shape (at %ld:%ld) - please send the dsn file as a bugreport\n", (long)wrr->line, (long)wrr->col);
		}
#warning TODO: what else
	}
	return 0;
}


static int dsn_parse_pcb(dsn_read_t *ctx, gsxl_node_t *root)
{
	gsxl_node_t *n, *nunit = NULL, *nstructure = NULL, *nplacement = NULL, *nlibrary = NULL, *nnetwork = NULL, *nwiring = NULL, *ncolors = NULL, *nresolution = NULL;

	/* default unit in case the file does not specify one */
	ctx->unit = get_unit_struct("inch");

	for(n = root->children->next; n != NULL; n = n->next) {
		if (n->str == NULL)
			continue;
		else if_save_uniq(n, unit)
		else if_save_uniq(n, resolution)
		else if_save_uniq(n, structure)
		else if_save_uniq(n, placement)
		else if_save_uniq(n, library)
		else if_save_uniq(n, network)
		else if_save_uniq(n, wiring)
		else if_save_uniq(n, colors)
	}

	if ((nresolution != NULL) && (nresolution->children != NULL)) {
		char *s, *su;

		for(su = s = STRE(gsxl_children(nresolution)); *s != '\0'; s++)
			*s = tolower(*s);

		ctx->unit = get_unit_struct(su);
		if (ctx->unit == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid resolution unit: '%s'\n", su);
			return -1;
		}
	}

	if (push_unit(ctx, nunit) == NULL)
		return -1;

	if (dsn_parse_structure(ctx, nstructure) != 0)
		return -1;

	if ((nwiring != NULL) && (nwiring->children != NULL))
		if (dsn_parse_wiring(ctx, nwiring) != 0)
			return -1;
	

	return 0;
}

/*** glue and low level ***/


int io_dsn_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[1024], *s;
	int phc = 0, in_pcb = 0, lineno = 0;

	if (typ != PCB_IOT_PCB)
		return 0;

	while(!(feof(f)) && (lineno < 512)) {
		if (fgets(line, sizeof(line), f) != NULL) {
			lineno++;
			for(s = line; *s != '\0'; s++)
				if (*s == '(')
					phc++;
			s = line;
			if ((phc > 0) && (strstr(s, "pcb") != 0))
				in_pcb = 1;
			if ((phc > 0) && (strstr(s, "PCB") != 0))
				in_pcb = 1;
			if ((phc > 2) && in_pcb && (strstr(s, "space_in_quoted_tokens") != 0))
				return 1;
			if ((phc > 2) && in_pcb && (strstr(s, "host_cad") != 0))
				return 1;
			if ((phc > 2) && in_pcb && (strstr(s, "host_version") != 0))
				return 1;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;
}

static int dsn_parse_file(dsn_read_t *rdctx, const char *fn)
{
	int c, blen = -1;
	gsx_parse_res_t res;
	FILE *f;
	char buff[12];
	long q_offs = -1, offs;


	f = pcb_fopen(fn, "r");
	if (f == NULL)
		return -1;

	/* find the offset of the quote char so it can be ignored during the s-expression parse */
	offs = 0;
	while(!(feof(f))) {
		c = fgetc(f);
		if (c == 's')
			blen = 0;
		if (blen >= 0) {
			buff[blen] = c;
			if (blen == 12) {
				if (memcmp(buff, "string_quote", 12) == 0) {
					for(c = fgetc(f),offs++; isspace(c); c = fgetc(f),offs++) ;
					q_offs = offs;
					printf("quote is %c at %ld\n", c, q_offs);
					break;
				}
				blen = -1;
			}
			blen++;
		}
		offs++;
	}
	rewind(f);


	gsxl_init(&rdctx->dom, gsxl_node_t);
	rdctx->dom.parse.line_comment_char = '#';
	offs = 0;
	do {
		c = fgetc(f);
		if (offs == q_offs) /* need to ignore the quote char else it's an unbalanced quote */
			c = '.';
		offs++;
	} while((res = gsxl_parse_char(&rdctx->dom, c)) == GSX_RES_NEXT);

	fclose(f);
	if (res != GSX_RES_EOE) {
		pcb_message(PCB_MSG_ERROR, "s-expression parse error at offset %ld\n", offs);
		return -1;
	}

	return 0;
}

int io_dsn_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	dsn_read_t rdctx;
	gsxl_node_t *rn;
	int ret;

	memset(&rdctx, 0, sizeof(rdctx));
	if (dsn_parse_file(&rdctx, Filename) != 0)
		goto error;

	gsxl_compact_tree(&rdctx.dom);
	rn = rdctx.dom.root;
	if ((rn == NULL) || (rn->str == NULL) || (pcb_strcasecmp(rn->str, "pcb") != 0)) {
		pcb_message(PCB_MSG_ERROR, "Root node should be pcb, got %s instead\n", rn->str);
		goto error;
	}
	if (gsxl_next(rn) != NULL) {
		pcb_message(PCB_MSG_ERROR, "Multiple root nodes?!\n");
		goto error;
	}

	rdctx.pcb = pcb;
	htsp_init(&rdctx.name2layer, strhash, strkeyeq);
	ret = dsn_parse_pcb(&rdctx, rn);
	htsp_uninit(&rdctx.name2layer);
	gsxl_uninit(&rdctx.dom);
	return ret;

	error:;
	gsxl_uninit(&rdctx.dom);
	return -1;
}

