/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - std format read: low level string unpack
 *  pcb-rnd Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 Entrust Fund in 2024)
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


#include <libnanojson/nanojson.c>
#include <libnanojson/semantic.c>

#include <rnd_inclib/lib_easyeda/gendom.c>
#include <rnd_inclib/lib_easyeda/gendom_json.c>
#include <rnd_inclib/lib_easyeda/easyeda_low.c>

#include "easyeda_sphash.h"

#define SEP3 "#@$"

long easyeda_str2name(const char *str)
{
	long res = easy_sphash(str);
	if (res < 0)
		rnd_message(RND_MSG_ERROR, "Internal error: missing easyeda keyword '%s'\n", str);
	return res;
}

static int parse_pcb_shape_any(gdom_node_t **shape);

/*** helpers ***/
static void fixup_poly_coords(gdom_node_t *parent, long subtree_name)
{
	gdom_node_t *coords, *old_coords;
	static const str_tab_t ctab[] = {
		{easy_crd, GDOM_DOUBLE},
		{-1}
	};

	/* parse coords */
	old_coords = gdom_hash_get(parent, subtree_name);
	if (old_coords != NULL) {
		coords = gdom_alloc(subtree_name, GDOM_ARRAY);
		parse_str_by_tab(old_coords->value.str, coords, ctab, ' ');
		replace_node(old_coords, coords);
	}
}

/*** shapes ***/

/* CA~1000~1000~#000000~yes~#FFFFFF~10~1000~1000~line~0.5~mm~1~45~~0.5~4020~3400.5~0~yes */
static int parse_canvas(gdom_node_t **canvas_orig)
{
	gdom_node_t *canvas;
	static const str_tab_t fields[] = {
		{easy_viewbox_width, GDOM_DOUBLE},
		{easy_viewbox_height, GDOM_DOUBLE},
		{easy_bg_color, GDOM_STRING},
		{easy_grid_visible, GDOM_STRING},
		{easy_grid_color, GDOM_STRING},
		{easy_grid_size, GDOM_DOUBLE},
		{easy_canvas_width, GDOM_DOUBLE},
		{easy_canvas_height, GDOM_DOUBLE},
		{easy_grid_style, GDOM_STRING},
		{easy_snap_size, GDOM_DOUBLE},
		{easy_grid_unit, GDOM_STRING},
		{easy_routing_width, GDOM_DOUBLE},
		{easy_routing_angle, GDOM_STRING},
		{easy_copper_area_visible, GDOM_STRING},
		{easy_snap_size_alt, GDOM_DOUBLE},
		{easy_origin_x, GDOM_DOUBLE},
		{easy_origin_y, GDOM_DOUBLE},
		{-1}
	};

	canvas = gdom_alloc(easy_canvas, GDOM_HASH);
	parse_str_by_tab((*canvas_orig)->value.str+3, canvas, fields, '~');

	replace_node(*canvas_orig, canvas);

	return 0;
}

static int parse_shape_track(char *str, gdom_node_t **shape)
{
	gdom_node_t *track;
	static const str_tab_t fields[] = {
		{easy_stroke_width, GDOM_DOUBLE},
		{easy_layer, GDOM_LONG},
		{easy_net, GDOM_STRING},
		{easy_points, GDOM_STRING},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	track = gdom_alloc(easy_track, GDOM_HASH);
	parse_str_by_tab(str, track, fields, '~');

	fixup_poly_coords(track, easy_points);

	replace_node(*shape, track);

	return 0;
}

static int parse_shape_via(char *str, gdom_node_t **shape)
{
	gdom_node_t *via;
	static const str_tab_t fields[] = {
		{easy_x, GDOM_DOUBLE},
		{easy_y, GDOM_DOUBLE},
		{easy_dia, GDOM_DOUBLE},
		{easy_net, GDOM_STRING},
		{easy_hole_radius, GDOM_DOUBLE},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	via = gdom_alloc(easy_via, GDOM_HASH);
	parse_str_by_tab(str, via, fields, '~');

	replace_node(*shape, via);

	return 0;
}

static int parse_shape_pad(char *str, gdom_node_t **shape)
{
	gdom_node_t *pad;
	static const str_tab_t fields[] = {
		{easy_shape, GDOM_STRING},
		{easy_x, GDOM_DOUBLE},
		{easy_y, GDOM_DOUBLE},
		{easy_width, GDOM_DOUBLE},
		{easy_height, GDOM_DOUBLE},
		{easy_layer, GDOM_LONG},
		{easy_net, GDOM_STRING},
		{easy_number, GDOM_LONG},
		{easy_hole_radius, GDOM_DOUBLE},
		{easy_points, GDOM_STRING},
		{easy_rot, GDOM_DOUBLE},
		{easy_id, GDOM_STRING},
		{easy_slot_length, GDOM_DOUBLE},
		{easy_slot_points, GDOM_STRING},
		{easy_plated, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	pad = gdom_alloc(easy_pad, GDOM_HASH);
	parse_str_by_tab(str, pad, fields, '~');

	fixup_poly_coords(pad, easy_points);
	fixup_poly_coords(pad, easy_slot_points);

	replace_node(*shape, pad);

	return 0;
}

static int parse_shape_hole(char *str, gdom_node_t **shape)
{
	gdom_node_t *hole;
	static const str_tab_t fields[] = {
		{easy_x, GDOM_DOUBLE},
		{easy_y, GDOM_DOUBLE},
		{easy_dia, GDOM_DOUBLE},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	hole = gdom_alloc(easy_hole, GDOM_HASH);
	parse_str_by_tab(str, hole, fields, '~');

	replace_node(*shape, hole);

	return 0;
}

static int parse_shape_text(char *str, gdom_node_t **shape)
{
	gdom_node_t *text;
	static const str_tab_t fields[] = {
		{easy_type, GDOM_STRING},
		{easy_x, GDOM_DOUBLE},
		{easy_y, GDOM_DOUBLE},
		{easy_stroke_width, GDOM_DOUBLE},
		{easy_rot, GDOM_DOUBLE},
		{easy_mirror, GDOM_STRING},
		{easy_layer, GDOM_LONG},
		{easy_net, GDOM_STRING},
		{easy_height, GDOM_DOUBLE},
		{easy_string, GDOM_STRING},
		{easy_path, GDOM_STRING},
		{easy_display, GDOM_STRING},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	text = gdom_alloc(easy_text, GDOM_HASH);
	parse_str_by_tab(str, text, fields, '~');

	replace_node(*shape, text);

	return 0;
}


static int parse_shape_arc(char *str, gdom_node_t **shape)
{
	gdom_node_t *arc;
	static const str_tab_t fields[] = {
		{easy_stroke_width, GDOM_DOUBLE},
		{easy_layer, GDOM_LONG},
		{easy_net, GDOM_STRING},
		{easy_path, GDOM_STRING},
		{easy_helper_dots, GDOM_STRING},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	arc = gdom_alloc(easy_arc, GDOM_HASH);
	parse_str_by_tab(str, arc, fields, '~');

	replace_node(*shape, arc);

	return 0;
}

static int parse_shape_circle(char *str, gdom_node_t **shape)
{
	gdom_node_t *circle;
	static const str_tab_t fields[] = {
		{easy_x, GDOM_DOUBLE},
		{easy_y, GDOM_DOUBLE},
		{easy_r, GDOM_DOUBLE},
		{easy_stroke_width, GDOM_DOUBLE},
		{easy_layer, GDOM_LONG},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	circle = gdom_alloc(easy_circle, GDOM_HASH);
	parse_str_by_tab(str, circle, fields, '~');

	replace_node(*shape, circle);

	return 0;
}

static int parse_shape_copperarea(char *str, gdom_node_t **shape)
{
	gdom_node_t *copperarea;
	static const str_tab_t fields[] = {
		{easy_stroke_width, GDOM_DOUBLE},
		{easy_layer, GDOM_LONG},
		{easy_net, GDOM_STRING},
		{easy_path, GDOM_STRING},
		{easy_clearance, GDOM_DOUBLE},
		{easy_fill, GDOM_STRING},
		{easy_id, GDOM_STRING},
		{easy_thermal, GDOM_STRING},
		{easy_keep_island, GDOM_STRING},
		{easy_zone, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	copperarea = gdom_alloc(easy_copperarea, GDOM_HASH);
	parse_str_by_tab(str, copperarea, fields, '~');

	replace_node(*shape, copperarea);

	return 0;
}

static int parse_shape_solidregion(char *str, gdom_node_t **shape)
{
	gdom_node_t *solidregion;
	static const str_tab_t fields[] = {
		{easy_layer, GDOM_LONG},
		{easy_net, GDOM_STRING},
		{easy_path, GDOM_STRING},
		{easy_type, GDOM_STRING},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	solidregion = gdom_alloc(easy_solidregion, GDOM_HASH);
	parse_str_by_tab(str, solidregion, fields, '~');

	replace_node(*shape, solidregion);

	return 0;
}

static int parse_shape_rect(char *str, gdom_node_t **shape)
{
	gdom_node_t *rect;
	static const str_tab_t fields[] = {
		{easy_x, GDOM_DOUBLE},
		{easy_y, GDOM_DOUBLE},
		{easy_width, GDOM_DOUBLE},
		{easy_height, GDOM_DOUBLE},
		{easy_layer, GDOM_LONG},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	rect = gdom_alloc(easy_rect, GDOM_HASH);
	parse_str_by_tab(str, rect, fields, '~');

	replace_node(*shape, rect);

	return 0;
}

static int parse_shape_dimension(char *str, gdom_node_t **shape)
{
	gdom_node_t *dimension;
	static const str_tab_t fields[] = {
		{easy_layer, GDOM_LONG},
		{easy_path, GDOM_STRING},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	dimension = gdom_alloc(easy_dimension, GDOM_HASH);
	parse_str_by_tab(str, dimension, fields, '~');

	replace_node(*shape, dimension);

	return 0;
}

static int parse_shape_svgnode(char *str, gdom_node_t **shape)
{
	gdom_node_t *svgnode, *subtree;
	long len = strlen(str);
	char *tmp, *so;
	const char *si;

	svgnode = gdom_alloc(easy_svgnode, GDOM_HASH);

	/* unquote the string */
	tmp = malloc(len+1);
	for(si = str, so = tmp; *si != '\0'; si++,so++) {
		if (*si == '\\') si++;
		*so = *si;
	}
	*so = '\0';

	subtree = gdom_json_parse_str(tmp, easyeda_str2name);
	if (subtree == NULL) {
		rnd_trace("Unquoted: '%s'\n", tmp);
		rnd_trace("Subtree: %p\n", subtree);
	}
	else
		gdom_hash_put(svgnode, subtree);

	replace_node(*shape, svgnode);

	free(tmp);

	return 0;
}

/*** lib (subcircuit) ***/

static int parse_shape_lib_hdr(char *str, gdom_node_t *sym)
{
	static const str_tab_t fields[] = {
		{easy_x, GDOM_DOUBLE},
		{easy_y, GDOM_DOUBLE},
		{easy_attributes, GDOM_STRING},
		{easy_rot, GDOM_LONG},
		{easy_import, GDOM_LONG},
		{easy_id, GDOM_STRING},
		{easy_locked, GDOM_LONG},
		{-1}
	};

	return parse_str_by_tab(str, sym, fields, '~');
}

static int parse_shape_lib_shape(char *str, gdom_node_t *shapes)
{
	gdom_node_t *tmp = gdom_alloc(easy_shape, GDOM_STRING);
	tmp->value.str = rnd_strdup(str);
	if (gdom_array_append(shapes, tmp) != 0)
		return -1;

	return parse_pcb_shape_any(&tmp);
}

static int parse_shape_lib(char *str, gdom_node_t **lib)
{
	gdom_node_t *fp = NULL, *shapes = NULL;
	char *s, *next;
	int n, res = 0;

	for(n = 0, s = str; s != NULL; n++, s = next) {
		next = strstr(s, SEP3);
		if (next != NULL) {
			*next = '\0';
			next += 3;
		}
		if (n == 0) {
			fp = gdom_alloc(easy_subc, GDOM_HASH);
			shapes = gdom_alloc(easy_shape, GDOM_ARRAY);
			if (gdom_hash_put(fp, shapes) != 0)
				res = -1;
			parse_shape_lib_hdr(s, fp);
		}
		else
			res |= parse_shape_lib_shape(s, shapes);
	}

	if (fp == NULL)
		return res;

	replace_node(*lib, fp);

	return res;
}


/*** common ***/
static int parse_pcb_shape_any(gdom_node_t **shape)
{
	char *str;

	if ((*shape)->type != GDOM_STRING)
		return -1;

	str = (*shape)->value.str;
	if (str[0] == '\0')
		return -1;

	if (str[1] == '~') {
/*		if (str[0] == 'C') return parse_shape_circle(str+2, shape);*/
	}
	else {
		if (strncmp(str, "TRACK~", 6) == 0) return parse_shape_track(str+6, shape);
		if (strncmp(str, "VIA~", 4) == 0) return parse_shape_via(str+4, shape);
		if (strncmp(str, "HOLE~", 5) == 0) return parse_shape_hole(str+5, shape);
		if (strncmp(str, "PAD~", 4) == 0) return parse_shape_pad(str+4, shape);
		if (strncmp(str, "TEXT~", 5) == 0) return parse_shape_text(str+5, shape);
		if (strncmp(str, "ARC~", 4) == 0) return parse_shape_arc(str+4, shape);
		if (strncmp(str, "CIRCLE~", 7) == 0) return parse_shape_circle(str+7, shape);
		if (strncmp(str, "COPPERAREA~", 11) == 0) return parse_shape_copperarea(str+11, shape);
		if (strncmp(str, "SOLIDREGION~", 12) == 0) return parse_shape_solidregion(str+12, shape);
		if (strncmp(str, "RECT~", 5) == 0) return parse_shape_rect(str+5, shape);
		if (strncmp(str, "DIMENSION~", 10) == 0) return parse_shape_dimension(str+10, shape);
		if (strncmp(str, "LIB~", 4) == 0) return parse_shape_lib(str+4, shape);
		if (strncmp(str, "SVGNODE~", 8) == 0) return parse_shape_svgnode(str+8, shape);
	}

	return -1;
}

/* 7~TopSolderMaskLayer~#800080~true~false~true~0.3 */
static int parse_pcb_layer(gdom_node_t **shape)
{
	gdom_node_t *layer;
	static const str_tab_t fields[] = {
		{easy_id, GDOM_LONG},
		{easy_name, GDOM_STRING},
		{easy_color, GDOM_STRING},
		{easy_visible, GDOM_STRING},
		{easy_active, GDOM_STRING},
		{easy_config, GDOM_STRING},
		{-1}
	};

	layer = gdom_alloc(easy_layer, GDOM_HASH);
	parse_str_by_tab((*shape)->value.str, layer, fields, '~');

	fixup_poly_coords(layer, easy_points);

	replace_node(*shape, layer);

	return 0;
}


gdom_node_t *easystd_low_pcb_parse(FILE *f, int is_fp)
{
	gdom_node_t *root, *shapes, *canvas, *layers;

	/* low level json parse -> initial dom */
	root = gdom_json_parse(f, easyeda_str2name);
	if (root == NULL)
		return NULL;

	/* parse strings into dom subtrees */
	canvas = gdom_hash_get(root, easy_canvas);
	if ((canvas != NULL) && (canvas->type == GDOM_STRING))
		parse_canvas(&canvas);

	shapes = gdom_hash_get(root, easy_shape);
	if ((shapes != NULL) && (shapes->type == GDOM_ARRAY)) {
		long n;
		for(n = 0; n < shapes->value.array.used; n++)
			parse_pcb_shape_any(&shapes->value.array.child[n]);
	}

	layers = gdom_hash_get(root, easy_layers);
	if ((layers != NULL) && (layers->type == GDOM_ARRAY)) {
		long n;
		for(n = 0; n < layers->value.array.used; n++)
			parse_pcb_layer(&layers->value.array.child[n]);
	}

	return root;
}



static const char *name2str(long name)
{
	return easy_keyname(name);
}

void easyeda_dump_tree(FILE *f, gdom_node_t *tree)
{
	if (tree != NULL)
		gdom_dump(f, tree, 0, name2str);
	else
		fprintf(f, "<NULL tree>\n");
}

gdom_node_t *easystd_low_parse(FILE *f, int is_sym)
{
	gdom_node_t *tree = easystd_low_pcb_parse(f, is_sym);

	if (conf_io_easyeda.plugins.io_easyeda.debug.dump_dom)
		easyeda_dump_tree(stdout, tree);

	return tree;
}
