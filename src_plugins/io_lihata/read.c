/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016..2021 Tibor 'Igor2' Palinkas
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

/* Load a lihata document in-memory and walk the tree and build pcb native
   structs. A full dom load is used instead of the event parser so that
   symlinks and tree merges can be supported later. */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <genht/htip.h>
#include <liblihata/tree.h>
#include <libminuid/libminuid.h>
#include "config.h"
#include "board.h"
#include "data.h"
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include "plug_io.h"
#include "flag_str.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/error.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/base64.h>
#include <librnd/hid/pixmap.h>
#include <libulzw/libulzw.h>
#include "layer.h"
#include <genvector/vtp0.h>
#include "common.h"
#include "polygon.h"
#include "conf_core.h"
#include "obj_subc.h"
#include "pcb_minuid.h"
#include "thermal.h"
#include "io_lihata.h"
#include <librnd/core/safe_fs.h>
#include "plug_footprint.h"
#include "vtpadstack.h"
#include "obj_pstk_inlines.h"
#include "netlist.h"

#include "../src_plugins/lib_compat_help/subc_help.h"
#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_compat_help/elem_rot.h"
#include "../src_plugins/lib_compat_help/route_style.h"

typedef struct {
	vtp0_t post_ids, post_thermal_old, post_thermal_heavy;
	int rdver;
	unsigned long warned, old_model_warned;
	rnd_conf_role_t cfg_dest;
	unsigned conf_root_replaced:1;
} lht_read_t;

static pcb_data_t DUMMY_BUFFER_SUBC;

/* Note: this works because of using str_flag compat_types */
#define PCB_OBJ_VIA PCB_OBJ_PSTK
#define PCB_OBJ_PIN PCB_OBJ_PSTK
#define PCB_OBJ_PAD PCB_OBJ_PSTK
#define PCB_OBJ_ELEMENT PCB_OBJ_SUBC

static pcb_data_t *parse_data(lht_read_t *rctx, pcb_board_t *pcb, pcb_data_t *dst, lht_node_t *nd, pcb_data_t *subc_parent);

static int iolht_error(lht_node_t *nd, char *fmt, ...)
{
	gds_t str;
	va_list ap;

	gds_init(&str);
	gds_append_str(&str, "io_lihata parse error");
	if (nd != NULL)
		rnd_append_printf(&str, " at %s:%d.%d: ", nd->file_name, nd->line, nd->col);
	else
		gds_append_str(&str, ": ");

	va_start(ap, fmt);
	rnd_safe_append_vprintf(&str, 0, fmt, ap);
	va_end(ap);

	rnd_message(RND_MSG_ERROR, "%s", str.array);

	gds_uninit(&str);
	return -1;
}

static void iolht_warn(lht_read_t *rctx, lht_node_t *nd, int wbit, char *fmt, ...)
{
	gds_t str;
	va_list ap;

	if (wbit >= 0) {
		unsigned long bv = 1ul << wbit;
		if (rctx->warned & bv)
			return;
		rctx->warned |= bv;
	}

	gds_init(&str);
	gds_append_str(&str, "io_lihata parse warning");
	if (nd != NULL)
		rnd_append_printf(&str, " at %s:%d.%d: ", nd->file_name, nd->line, nd->col);
	else
		gds_append_str(&str, ": ");

	va_start(ap, fmt);
	rnd_safe_append_vprintf(&str, 0, fmt, ap);
	va_end(ap);

	rnd_message(RND_MSG_WARNING, "%s", str.array);

	gds_uninit(&str);
}

/* Collect objects that has unknown ID on a list. Once all objects with
   known-IDs are allocated, the unknonw-ID objects are allocated a fresh
   ID. This makes sure they don't occupy IDs that would be used by known-ID
   objects during the load. */
#define post_id_req(obj) vtp0_append(&rctx->post_ids, &((obj)->ID))

static lht_node_t missing_ok;

static int parse_attributes(pcb_attribute_list_t *list, lht_node_t *nd)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (nd == &missing_ok)
		return 0;

	if (nd == NULL)
		return 0;

	if (nd->type != LHT_HASH)
		return -1;

	for(n = lht_dom_first(&it, nd); n != NULL; n = lht_dom_next(&it)) {
		if (n->type == LHT_TEXT)
			pcb_attribute_put(list, n->name, n->data.text.value);
	}

	return 0;
}

/* Load the (duplicated) string value of a text node into res. Return 0 on success */
static int parse_text(char **res, lht_node_t *nd)
{
	if (nd == &missing_ok)
		return 0;
	if (nd == NULL)
		return -1;
	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "expected a text node\n");
	*res = rnd_strdup(nd->data.text.value);
	return 0;
}

/* Load a minuid from a text node into res. Return 0 on success */
static int parse_minuid(minuid_bin_t res, lht_node_t *nd)
{
	if (nd == &missing_ok)
		return 0;
	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return iolht_error(nd, "expected a text node for minuid\n");
	if (strlen(nd->data.text.value) != sizeof(minuid_str_t)-1)
		return iolht_error(nd, "invalid minuid: '%s'\n", nd->data.text.value);
	minuid_str2bin(res, (char *)nd->data.text.value);
	return 0;
}

/* Load the rnd_coord_t value of a text node into res. Return 0 on success */
static int parse_coord(rnd_coord_t *res, lht_node_t *nd)
{
	double tmp;
	rnd_bool success;

	if (nd == &missing_ok)
		return 0;

	if (nd == NULL)
		return iolht_error(nd, "Missing coord value\n");

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid coord type: '%d'\n", nd->type);

	tmp = rnd_get_value_ex(nd->data.text.value, NULL, NULL, NULL, NULL, &success);
	if (!success)
		return iolht_error(nd, "Invalid coord value: '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}

/* Load the Angle value of a text node into res. Return 0 on success */
static int parse_angle(rnd_angle_t *res, lht_node_t *nd)
{
	double tmp;
	rnd_bool success;

	if (nd == &missing_ok)
		return 0;

	if (nd == NULL)
		return iolht_error(nd, "Missing angle\n");

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid angle type: '%d'\n", nd->type);

	tmp = rnd_get_value_ex(nd->data.text.value, NULL, NULL, NULL, NULL, &success);
	if (!success)
		return iolht_error(nd, "Invalid angle value: '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}

static int valid_int_tail(const char *end)
{
	while(isspace(*end)) end++;
	return *end == '\0';
}

/* Load the integer value of a text node into res. Return 0 on success */
static int parse_int(int *res, lht_node_t *nd)
{
	long int tmp;
	int base = 10;
	char *end;

	if (nd == &missing_ok)
		return 0;

	if (nd == NULL)
		return -1;

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid integer node type (int): '%d'\n", nd->type);

	if ((nd->data.text.value[0] == '0') && (nd->data.text.value[1] == 'x'))
		base = 16;
	tmp = strtol(nd->data.text.value, &end, base);
	if (!valid_int_tail(end))
		return iolht_error(nd, "Invalid integer value (not an integer number): '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}


/* Load the unsigned long value of a text node into res. Return 0 on success */
static int parse_ulong(unsigned long *res, lht_node_t *nd)
{
	unsigned long int tmp;
	int base = 10;
	char *end;

	if (nd == &missing_ok)
		return 0;

	if (nd == NULL)
		return iolht_error(nd, "Missing unsigned long field\n");

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid integer node type (ulong): '%d'\n", nd->type);

	if ((nd->data.text.value[0] == '0') && (nd->data.text.value[1] == 'x'))
		base = 16;
	tmp = strtoul(nd->data.text.value, &end, base);
	if (!valid_int_tail(end))
		return iolht_error(nd, "Invalid integer value (not an unsigned long integer): '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}

/* Load the duble value of a text node into res. Return 0 on success */
static int parse_double(double *res, lht_node_t *nd)
{
	double tmp;
	char *end;

	if (nd == &missing_ok)
		return 0;

	if (nd == NULL)
		return iolht_error(nd, "Missing floating point number\n");

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid floating point number type: '%d'\n", nd->type);

	tmp = strtod(nd->data.text.value, &end);

	if (*end != '\0')
		return iolht_error(nd, "Invalid floating point value: '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}

/* Load the id name of a text node (with a prefixed name) into res.
   Return 0 on success */
static int parse_id(long int *res, lht_node_t *nd, int prefix_len)
{
	long int tmp;
	char *end;

	if (nd == &missing_ok)
		return 0;

	if (nd == NULL)
		return iolht_error(nd, "Missing ID node\n");

	tmp = strtol(nd->name + prefix_len, &end, 10);
	if (*end != '\0')
		return iolht_error(nd, "Invalid id value (must be a positive integer): '%s'\n", nd->data.text.value);

	pcb_create_ID_bump(tmp+1);

	*res = tmp;
	return 0;
}

/* Load the integer value of a text node into res. Return 0 on success */
static int parse_idpath(pcb_board_t *pcb, pcb_idpath_t **res, lht_node_t *nd)
{
	if (nd == &missing_ok)
		return -1;

	if (nd == NULL)
		return -1;

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid idpath node type: '%d'\n", nd->type);

	*res = pcb_str2idpath(pcb, nd->data.text.value);
	return 0;
}

/* Load the boolean value of a text node into res.
   Return 0 on success */
static int parse_bool(rnd_bool *res, lht_node_t *nd)
{
	char val[8], *end;

	if (nd == &missing_ok)
		return 0;

	if (nd == NULL)
		return iolht_error(nd, "Missing bool\n");

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid bool type: '%d'\n", nd->type);

	strncpy(val, nd->data.text.value, sizeof(val)-1);
	val[sizeof(val)-1] = '\0';
	end = strpbrk(val, " \t\r\n");
	if (end != NULL)
		*end = '\0';


	if ((strcmp(val, "1") == 0) || (rnd_strcasecmp(val, "on") == 0) ||
	    (rnd_strcasecmp(val, "true") == 0) || (rnd_strcasecmp(val, "yes") == 0)) {
		*res = 1;
		return 0;
	}

	if ((strcmp(val, "0") == 0) || (rnd_strcasecmp(val, "off") == 0) ||
	    (rnd_strcasecmp(val, "false") == 0) || (rnd_strcasecmp(val, "no") == 0)) {
		*res = 0;
		return 0;
	}

	return iolht_error(nd, "Invalid bool value: '%s'\n", nd->data.text.value);
}

static int parse_coord_conf(lht_read_t *rctx, const char *path, lht_node_t *nd)
{
	rnd_coord_t tmp;

	if (nd == &missing_ok)
		return 0;
	if (nd == NULL)
		return 0;
	if (parse_coord(&tmp, nd) != 0)
		return -1;

	if (rctx->cfg_dest != RND_CFR_invalid) {
		if ((rctx->cfg_dest == RND_CFR_DESIGN) && !rctx->conf_root_replaced) {
			/* special case: old versions of the format did not have a config subtree;
			   we have to emulate one if we are setting CFR_DESIGN, else the
			   caller (plug_io) will drop the new conf */
			rctx->conf_root_replaced = 1;
			rnd_conf_main_root_replace_cnt[RND_CFR_DESIGN]++;
		}
		rnd_conf_set(rctx->cfg_dest, path, -1, nd->data.text.value, RND_POL_OVERWRITE);
	}
	return 0;
}

static lht_node_t *hash_get(lht_node_t *hash, const char *name, int optional)
{
	lht_node_t *nd = lht_dom_hash_get(hash, name);
	if (nd != NULL)
		return nd;
	if (optional)
		return &missing_ok;
	iolht_error(hash, "Missing hash field: '%s'\n", name);
	return NULL;
}

static int parse_meta(lht_read_t *rctx, pcb_board_t *pcb, lht_node_t *nd)
{
	lht_node_t *grp;
	int err = 0;

	if (nd->type != LHT_HASH)
		return iolht_error(nd, "board meta must be a hash\n");

	parse_text(&pcb->hidlib.name, lht_dom_hash_get(nd, "meta"));

	parse_text(&pcb->hidlib.name, lht_dom_hash_get(nd, "board_name"));

	grp = lht_dom_hash_get(nd, "grid");
	if ((grp != NULL) && (grp->type == LHT_HASH)) {
		err |= parse_coord(&pcb->hidlib.grid_ox, hash_get(grp, "offs_x", 1));
		err |= parse_coord(&pcb->hidlib.grid_oy, hash_get(grp, "offs_y", 1));
		err |= parse_coord(&pcb->hidlib.grid, hash_get(grp, "spacing", 1));
		if (err != 0)
			return -1;
	}

	grp = lht_dom_hash_get(nd, "size");
	if ((grp != NULL) && (grp->type == LHT_HASH)) {
		err |= parse_coord(&pcb->hidlib.dwg.X2, hash_get(grp, "x", 0));
		err |= parse_coord(&pcb->hidlib.dwg.Y2, hash_get(grp, "y", 0));
		err |= parse_coord_conf(rctx, "design/poly_isle_area", hash_get(grp, "isle_area_nm2", 1));
		err |= parse_double(&pcb->ThermScale, hash_get(grp, "thermal_scale", 1));
		if (pcb->ThermScale < 0.01) {
			lht_node_t *ts = hash_get(grp, "thermal_scale", 1);
			if (ts != NULL)
				iolht_error(ts, "Your thermal_scale is too small. This will probably cause problems in calculating thermals.\n");
		}
		if (err != 0)
			return -1;
	}

	grp = lht_dom_hash_get(nd, "drc");
	if ((grp != NULL) && (grp->type == LHT_HASH)) {
		if (rctx->rdver >= 5)
			iolht_warn(rctx, grp, 5, "Lihata board v5+ should not have drc metadata saved in board header (use the config)\n");
		err |= parse_coord_conf(rctx, "design/bloat", hash_get(grp, "bloat", 1));
		err |= parse_coord_conf(rctx, "design/shrink", hash_get(grp, "shrink", 1));
		err |= parse_coord_conf(rctx, "design/min_wid", hash_get(grp, "min_width", 1));
		err |= parse_coord_conf(rctx, "design/min_slk", hash_get(grp, "min_silk", 1));
		err |= parse_coord_conf(rctx, "design/min_drill", hash_get(grp, "min_drill", 1));
		err |= parse_coord_conf(rctx, "design/min_ring", hash_get(grp, "min_ring", 1));
		if (err != 0)
			return 1;
	}

	grp = lht_dom_hash_get(nd, "cursor");
	if ((grp != NULL) && (grp->type == LHT_HASH)) {
		if (rctx->rdver >= 5)
			iolht_warn(rctx, grp, 0, "Lihata board v5+ should not have cursor metadata saved\n");
	}

	return 0;
}

static int parse_thermal(unsigned char *dst, lht_node_t *src)
{
	if (src == NULL)
		return 0;

	if (src->type != LHT_LIST)
		return iolht_error(src, "thermals must be a list\n");

	*dst = 0;
	for(src = src->data.list.first; src != NULL; src = src->next)
		if (src->type == LHT_TEXT)
			*dst |= pcb_thermal_str2bits(src->data.text.value);

	return 0;
}

/* pt is a list of lihata node pointers to thermal nodes; each has user
   data set to the object. Look up layer info and build the thermal. This
   needs to be done in a separate pass at the end of parsing because
   vias may precede layers in the lihata input file. */
static int post_thermal_assign(lht_read_t *rctx, pcb_board_t *pcb, vtp0_t *old, vtp0_t *heavy)
{
	int i;
	lht_node_t *n;
	lht_dom_iterator_t it;

	/* pin/via before lihata v4: thermal is part of the object flag*/
	for(i = 0; i < vtp0_len(old); i++) {
		lht_node_t *thr = old->array[i];
		pcb_pstk_t *ps = thr->user_data;

		assert(ps->type == PCB_OBJ_PSTK);

		ps->thermals.used = 0;

		for(n = lht_dom_first(&it, thr); n != NULL; n = lht_dom_next(&it)) {
			if (n->type == LHT_TEXT) {
				int layer = pcb_layer_by_name(pcb->Data, n->name) + 1;
				if (layer > ps->thermals.used)
					ps->thermals.used = layer;
			}
		}

		if (ps->thermals.used > 0) {
			ps->thermals.shape = calloc(sizeof(ps->thermals.shape[0]), ps->thermals.used);
			for(n = lht_dom_first(&it, thr); n != NULL; n = lht_dom_next(&it)) {
				if (n->type == LHT_TEXT) {
					int layer = pcb_layer_by_name(pcb->Data, n->name);
					if (layer < 0)
						return iolht_error(n, "Invalid layer name in thermal: '%s'\n", n->name);
					ps->thermals.shape[layer] = io_lihata_resolve_thermal_style_old(n->data.text.value, rctx->rdver);
				}
			}
		}
		else
			ps->thermals.shape = NULL;
	}
	vtp0_uninit(old);

	/* from lihata v4 up: thermal is an object property (on heavy terminal layer objects) */
	for(i = 0; i < vtp0_len(heavy); i++) {
		lht_node_t *thr = heavy->array[i];
		pcb_any_obj_t *obj = thr->user_data;

		/* single character thermal: only on the layer the object is on */
		parse_thermal(&obj->thermal, thr);
	}
	vtp0_uninit(heavy);

	return 0;
}

static int parse_flags(lht_read_t *rctx, pcb_flag_t *f, lht_node_t *fn, int object_type, unsigned char *intconn, int can_have_thermal)
{
	io_lihata_flag_holder fh;

	memset(&fh, 0, sizeof(fh));

	if (fn != NULL) {
		int n;
		for (n = 0; n < pcb_object_flagbits_len; n++) {
			long my_types = pcb_object_flagbits[n].object_types | ((rctx->rdver <= 4) ? pcb_object_flagbits[n].compat_types : 0);
			if (my_types & object_type) {
				rnd_bool b = 0;
				if ((parse_bool(&b, hash_get(fn, pcb_object_flagbits[n].name, 1)) == 0) && b)
					PCB_FLAG_SET(pcb_object_flagbits[n].mask, &fh);
			}
		}

		if ((!can_have_thermal) && (lht_dom_hash_get(fn, "thermal") != NULL))
			iolht_error(fn, "Invalid flag thermal: object type can not have a thermal (ignored)\n");


		if (parse_int(&n, lht_dom_hash_get(fn, "shape")) == 0)
			fh.Flags.q = n;

		if ((intconn != NULL) && (rctx->rdver < 3))
			if (parse_int(&n, lht_dom_hash_get(fn, "intconn")) == 0)
				*intconn = n;
	}

	*f = fh.Flags;
	return 0;
}

static void parse_thermal_old(lht_read_t *rctx, pcb_any_obj_t *obj, lht_node_t *fn)
{
	lht_node_t *thr;

	if (fn == NULL)
		return;

	thr = lht_dom_hash_get(fn, "thermal");
	if (thr == NULL)
		return;

	thr->user_data = obj;
	vtp0_append(&rctx->post_thermal_old, thr);
}

/* heavy terminal thermal: save for later processing */
static int parse_thermal_heavy(lht_read_t *rctx, pcb_any_obj_t *obj, lht_node_t *src)
{
	if (src == NULL)
		return 0;

	if (src->type != LHT_LIST)
		return iolht_error(src, "heavy thermal must be a list\n");

	src->user_data = obj;
	vtp0_append(&rctx->post_thermal_heavy, src);

	return 0;
}

static int parse_line(lht_read_t *rctx, pcb_layer_t *ly, lht_node_t *obj, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_line_t *line;
	unsigned char intconn = 0;
	int err = 0;
	long int id;

	if (obj->type != LHT_HASH)
		return iolht_error(obj, "line.ID must be a hash\n");

	if (ly == NULL)
		return iolht_error(obj, "can't allocate line on invalid layer\n");

	if (parse_id(&id, obj, 5) != 0)
		return -1;

	line = pcb_line_alloc_id(ly, id);
	parse_flags(rctx, &line->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_LINE, &intconn, 0);
	pcb_attrib_compat_set_intconn(&line->Attributes, intconn);
	parse_attributes(&line->Attributes, lht_dom_hash_get(obj, "attributes"));

	if (rctx->rdver >= 4)
		parse_thermal_heavy(rctx, (pcb_any_obj_t *)line, lht_dom_hash_get(obj, "thermal"));

	err |= parse_coord(&line->Thickness, hash_get(obj, "thickness", 0));
	err |= parse_coord(&line->Clearance, hash_get(obj, "clearance", 0));
	err |= parse_coord(&line->Point1.X, hash_get(obj, "x1", 0));
	err |= parse_coord(&line->Point1.Y, hash_get(obj, "y1", 0));
	err |= parse_coord(&line->Point2.X, hash_get(obj, "x2", 0));
	err |= parse_coord(&line->Point2.Y, hash_get(obj, "y2", 0));

	line->Point1.X += dx;
	line->Point2.X += dx;
	line->Point1.Y += dy;
	line->Point2.Y += dy;

	post_id_req(&line->Point1);
	post_id_req(&line->Point2);

	if (ly != NULL)
		pcb_add_line_on_layer(ly, line);

	return err;
}

static int parse_rat(lht_read_t *rctx, pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *obj)
{
	pcb_rat_t rat, *new_rat;
	int tmp, err = 0;

	if (parse_id(&rat.ID, obj, 4) != 0)
		return -1;
	parse_flags(rctx, &rat.Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_LINE, NULL, 0);

	err |= parse_coord(&rat.Point1.X, hash_get(obj, "x1", 0));
	err |= parse_coord(&rat.Point1.Y, hash_get(obj, "y1", 0));
	err |= parse_coord(&rat.Point2.X, hash_get(obj, "x2", 0));
	err |= parse_coord(&rat.Point2.Y, hash_get(obj, "y2", 0));

	err |= parse_int(&tmp, hash_get(obj, "lgrp1", 0));
	rat.group1 = tmp;
	err |= parse_int(&tmp, hash_get(obj, "lgrp2", 0));
	rat.group2 = tmp;

	rat.anchor[0] = rat.anchor[1] = NULL;
	err |= parse_idpath(pcb, &rat.anchor[0], hash_get(obj, "anchor1", 1));
	err |= parse_idpath(pcb, &rat.anchor[1], hash_get(obj, "anchor2", 1));

	new_rat = pcb_rat_new(dt, rat.ID, rat.Point1.X, rat.Point1.Y, rat.Point2.X, rat.Point2.Y, rat.group1, rat.group2,
		conf_core.appearance.rat_thickness, rat.Flags, NULL, NULL);
	
	new_rat->anchor[0] = rat.anchor[0];
	new_rat->anchor[1] = rat.anchor[1];

	parse_attributes(&new_rat->Attributes, lht_dom_hash_get(obj, "attributes"));

	post_id_req(&new_rat->Point1);
	post_id_req(&new_rat->Point2);

	return err;
}

static int parse_arc(lht_read_t *rctx, pcb_layer_t *ly, lht_node_t *obj, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_arc_t *arc;
	unsigned char intconn = 0;
	int err = 0;
	long int id;

	if (obj->type != LHT_HASH)
		return iolht_error(obj, "arc.ID must be a hash\n");

	if (ly == NULL)
		return iolht_error(obj, "can't allocate arc on invaid layer\n");

	if (parse_id(&id, obj, 4) != 0)
		return -1;

	arc = pcb_arc_alloc_id(ly, id);
	parse_flags(rctx, &arc->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_ARC, &intconn, 0);
	pcb_attrib_compat_set_intconn(&arc->Attributes, intconn);
	parse_attributes(&arc->Attributes, lht_dom_hash_get(obj, "attributes"));

	if (rctx->rdver >= 4)
		parse_thermal_heavy(rctx, (pcb_any_obj_t *)arc, lht_dom_hash_get(obj, "thermal"));

	err |= parse_coord(&arc->Thickness,  hash_get(obj, "thickness", 0));
	err |= parse_coord(&arc->Clearance,  hash_get(obj, "clearance", 0));
	err |= parse_coord(&arc->X,          hash_get(obj, "x", 0));
	err |= parse_coord(&arc->Y,          hash_get(obj, "y", 0));
	err |= parse_coord(&arc->Width,      hash_get(obj, "width", 0));
	err |= parse_coord(&arc->Height,     hash_get(obj, "height", 0));
	err |= parse_angle(&arc->StartAngle, hash_get(obj, "astart", 0));
	err |= parse_angle(&arc->Delta,      hash_get(obj, "adelta", 0));

	if (arc->Width < 0) {
		arc->Width = 0;
		iolht_error(obj, "negative radius in arc - replaced it with 0\n");
	}

	if (arc->Height < 0) {
		arc->Height = 0;
		iolht_error(obj, "negative radius in arc - replaced it with 0\n");
	}

	arc->X += dx;
	arc->Y += dy;

	if (ly != NULL)
		pcb_add_arc_on_layer(ly, arc);

	return err;
}

typedef struct {
	gds_t buff;
	unsigned char *iptr, *ibuff;
	size_t ibs;
} ucomp_t;

static void ucomp_dst(void *ctx_, int d)
{
	ucomp_t *ctx = ctx_;
	gds_append(&ctx->buff, d);
}

static int ucomp_src(void *ctx_)
{
	ucomp_t *ctx = ctx_;
	if (ctx->ibs == 0)
		return EOF;
	ctx->ibs--;
	return *ctx->iptr++;
}

static htip_t id2pxm;
static lht_node_t *pxm_root;

static rnd_pixmap_t *parse_pxm_(lht_node_t *obj)
{
	int err = 0, res, has_transp = 0;
	size_t b64s;
	unsigned long psx, psy;
	ucomp_t uctx;
	rnd_pixmap_t *pxm;
	lht_node_t *pmn, *trn;
	rnd_color_t transp;

	err |= parse_ulong(&psx,             hash_get(obj, "sx", 0));
	err |= parse_ulong(&psy,             hash_get(obj, "sy", 0));
	trn = hash_get(obj, "transparent", 0);

	if ((trn != NULL) && (trn->type == LHT_TEXT)) {
		rnd_color_load_str(&transp, trn->data.text.value);
		has_transp = 1;
	}

	pmn = hash_get(obj, "pixmap", 0);
	if ((pmn == NULL) || (pmn->type != LHT_TEXT)) {
		iolht_error(obj, "Failed to find a valid gfx pixmap node\n");
		return NULL;
	}

	memset(&uctx, 0, sizeof(uctx));
	b64s = strlen(pmn->data.text.value);
	uctx.iptr = uctx.ibuff = malloc(b64s);
	uctx.ibs = rnd_base64_str2bin(uctx.ibuff, b64s, pmn->data.text.value, b64s);

	if (uctx.ibs == -1) {
		free(uctx.ibuff);
		iolht_error(obj, "Failed to base64 decode pixmap gfx\n");
		return NULL;
	}

	res = ulzw_decompress(&uctx, ucomp_dst, ucomp_src);
	free(uctx.ibuff);
	if (res != 0) {
		gds_uninit(&uctx.buff);
		iolht_error(obj, "Failed to decompress gfx pixmap data\n");
		return NULL;
	}

	if (psx * psy * 3 != uctx.buff.used) {
		gds_uninit(&uctx.buff);
		iolht_error(obj, "Wrong size of pixmap\n");
		return NULL;
	}

	pxm = rnd_pixmap_alloc(&PCB->hidlib, psx, psy);
	pxm->p = (unsigned char *)uctx.buff.array;
	if (has_transp) {
		pxm->has_transp = 1;
		pxm->tr = transp.r;
		pxm->tg = transp.g;
		pxm->tb = transp.b;
	}
	return pxm;
}

static rnd_pixmap_t *parse_pxm(long int ID)
{
	lht_node_t *nd;
	char buff[128];
	rnd_pixmap_t *pxm;

	if (pxm_root == NULL)
		return NULL;

	pxm = htip_get(&id2pxm, ID);
	if (pxm != NULL)
		return pxm;

	sprintf(buff, "ulzw.%ld", ID);
	nd = hash_get(pxm_root, buff, 0);
	if (nd == NULL)
		return NULL;

	pxm = parse_pxm_(nd);
	if (pxm == NULL)
		return NULL;

	htip_set(&id2pxm, ID, pxm);
	return pxm;
}


static int pxm_inited = 0;
static void pxm_init(lht_read_t *rctx, lht_node_t *pixmaps)
{
	pxm_root = NULL;
	if (rctx->rdver >= 7)
		pxm_root = pixmaps;
	htip_init(&id2pxm, longhash, longkeyeq);
	pxm_inited = 1;
}

static void pxm_uninit(void)
{
	htip_entry_t *e;
	if (!pxm_inited)
		return;
	for(e = htip_first(&id2pxm); e != NULL; e = htip_next(&id2pxm, e))
		rnd_pixmap_free(e->value);
	htip_uninit(&id2pxm);
	pxm_root = NULL;
	pxm_inited = 0;
}


static int parse_gfx(lht_read_t *rctx, pcb_board_t *pcb, pcb_layer_t *ly, lht_node_t *obj, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_gfx_t *gfx;
	unsigned char intconn = 0;
	int err = 0, itmp;
	long int id;
	unsigned long int refid;
	rnd_pixmap_t *pxm;


	if (obj->type != LHT_HASH)
		return iolht_error(obj, "gfx.ID must be a hash\n");

	if (ly == NULL)
		return iolht_error(obj, "can't allocate gfx on invalid layer\n");

	if (parse_id(&id, obj, 4) != 0)
		return -1;

	gfx = pcb_gfx_alloc_id(ly, id);
	parse_flags(rctx, &gfx->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_ARC, &intconn, 0);
	pcb_attrib_compat_set_intconn(&gfx->Attributes, intconn);
	parse_attributes(&gfx->Attributes, lht_dom_hash_get(obj, "attributes"));

	if (rctx->rdver >= 4)
		parse_thermal_heavy(rctx, (pcb_any_obj_t *)gfx, lht_dom_hash_get(obj, "thermal"));

	err |= parse_coord(&gfx->cx,         hash_get(obj, "cx", 0));
	err |= parse_coord(&gfx->cy,         hash_get(obj, "cy", 0));
	err |= parse_coord(&gfx->sx,         hash_get(obj, "sx", 0));
	err |= parse_coord(&gfx->sy,         hash_get(obj, "sy", 0));
	err |= parse_angle(&gfx->rot,        hash_get(obj, "rot", 0));
	err |= parse_int(&itmp,              hash_get(obj, "xmirror", 0)); gfx->xmirror = itmp;
	err |= parse_int(&itmp,              hash_get(obj, "ymirror", 0)); gfx->ymirror = itmp;
	err |= parse_ulong(&refid,           hash_get(obj, "pixmap_ref", 0));

	if (err != 0) {
		pcb_gfx_free(gfx);
		return err;
	}

	gfx->cx += dx;
	gfx->cy += dy;

	pxm = parse_pxm(refid);
	if (pxm == NULL) {
		pcb_gfx_free(gfx);
		return iolht_error(obj, "Failed to load referenced pixmap\n");
	}

	pcb_gfx_set_pixmap_dup(gfx, pxm, 0);
	gfx->pxm_id = refid;

	pcb_gfx_update(gfx);
	if (ly != NULL)
		pcb_add_gfx_on_layer(ly, gfx);

	return err;
}


static int parse_polygon(lht_read_t *rctx, pcb_layer_t *ly, lht_node_t *obj)
{
	pcb_poly_t *poly;
	lht_node_t *geo;
	rnd_cardinal_t n = 0, c;
	unsigned char intconn = 0;
	int err = 0;
	long int id;

	if (obj->type != LHT_HASH)
		return iolht_error(obj, "polygon.ID must be a hash\n");

	if (parse_id(&id, obj, 8) != 0)
		return -1;
	poly = pcb_poly_alloc_id(ly, id);
	parse_flags(rctx, &poly->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_POLY, &intconn, 0);
	pcb_attrib_compat_set_intconn(&poly->Attributes, intconn);
	parse_attributes(&poly->Attributes, lht_dom_hash_get(obj, "attributes"));

	if (rctx->rdver >= 3)
		err |= parse_coord(&poly->Clearance, hash_get(obj, "clearance", 1));

	if (rctx->rdver >= 7)
		err |= parse_coord(&poly->enforce_clearance, hash_get(obj, "enforce_clearance", 1));

	if (rctx->rdver >= 4)
		parse_thermal_heavy(rctx, (pcb_any_obj_t *)poly, lht_dom_hash_get(obj, "thermal"));

	geo = lht_dom_hash_get(obj, "geometry");
	if ((geo != NULL) && (geo->type == LHT_LIST)) {
		lht_node_t *cnt;
		lht_dom_iterator_t it;

		/* count points and holes */
		poly->PointN = 0;
		for(c = 0, cnt = lht_dom_first(&it, geo); cnt != NULL; c++, cnt = lht_dom_next(&it)) {
			if (cnt->type != LHT_TABLE)
				continue;
			poly->PointN += cnt->data.table.rows;
		}
		poly->PointMax = poly->PointN;
		poly->Points = malloc(sizeof(rnd_point_t) * poly->PointMax);
		poly->HoleIndexMax = poly->HoleIndexN = c-1;
		if (poly->HoleIndexN > 0)
			poly->HoleIndex = malloc(sizeof(rnd_cardinal_t) * poly->HoleIndexMax);
		else
			poly->HoleIndex = NULL;

		/* convert points and build hole index */
		for(c = 0, cnt = lht_dom_first(&it, geo); cnt != NULL; c++, cnt = lht_dom_next(&it)) {
			rnd_cardinal_t r;
			if (cnt->type != LHT_TABLE)
				continue;
			if (c > 0)
				poly->HoleIndex[c-1] = n;
			for(r = 0; r < cnt->data.table.rows; r++) {
				err |= parse_coord(&poly->Points[n].X, cnt->data.table.r[r][0]);
				err |= parse_coord(&poly->Points[n].Y, cnt->data.table.r[r][1]);
				/* a point also needs to be a box on paper, but the poly code sets X2 and Y2 to 0 in reality... */
				poly->Points[n].X2 = poly->Points[n].Y2 = 0;
				post_id_req(&poly->Points[n]);
				n++;
			}
		}
	}
	else {
		pcb_poly_free(poly);
		return iolht_error(obj, "invalid polygon: empty geometry\n");
	}

	if (poly->PointN < 3) {
		pcb_poly_free(poly);
		return iolht_error(obj, "invalid polygon: less than 3 contour vertices\n");
	}

	if (err != 0) {
		pcb_poly_free(poly);
		return -1;
	}

	pcb_add_poly_on_layer(ly, poly);
	pcb_poly_init_clip(ly->parent.data, ly, poly);

	return 0;
}

static int parse_pcb_text(lht_read_t *rctx, pcb_layer_t *ly, lht_node_t *obj)
{
	pcb_text_t *text;
	lht_node_t *role, *nthickness, *nclearance, *nrot, *ndir;
	int tmp, err = 0, dir;
	unsigned char intconn = 0;
	long int id;

	if (obj->type != LHT_HASH)
		return iolht_error(obj, "text.ID must be a hash\n");

	role = lht_dom_hash_get(obj, "role");

	if (ly == NULL)
		return iolht_error(obj, "failed to allocate text object (layer == NULL)\n");

	if (parse_id(&id, obj, 5) != 0)
		return -1;

	if (role != NULL)
		return iolht_error(obj, "invalid role: text on layer shall not have a role\n");

	text = pcb_text_alloc_id(ly, id);
	if (text == NULL)
		return iolht_error(obj, "failed to allocate text object\n");

	parse_flags(rctx, &text->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_TEXT, &intconn, 0);
	pcb_attrib_compat_set_intconn(&text->Attributes, intconn);
	parse_attributes(&text->Attributes, lht_dom_hash_get(obj, "attributes"));
	err |= parse_int(&text->Scale, hash_get(obj, "scale", 0));
	err |= parse_double(&text->scale_x, hash_get(obj, "scale_x", 1));
	err |= parse_double(&text->scale_y, hash_get(obj, "scale_y", 1));
	tmp = 0;
	err |= parse_int(&tmp, hash_get(obj, "fid", 0));
	text->fid = tmp;
	tmp = 0;
	err |= parse_coord(&text->X, hash_get(obj, "x", 0));
	err |= parse_coord(&text->Y, hash_get(obj, "y", 0));
	err |= parse_text(&text->TextString, hash_get(obj, "string", 0));

	nthickness = lht_dom_hash_get(obj, "thickness");
	nclearance = lht_dom_hash_get(obj, "clearance");
	nrot = lht_dom_hash_get(obj, "rot");
	ndir = lht_dom_hash_get(obj, "direction");


	if (nthickness != NULL) {
		if (rctx->rdver < 6)
			iolht_warn(rctx, nthickness, -1, "Text thickness should not be present in a file with version lower than v6\n");
		err |= parse_coord(&text->thickness, nthickness);
	}
	else
		text->thickness = 0;

	if (nclearance != NULL) {
		if (rctx->rdver < 8)
			iolht_warn(rctx, nthickness, -1, "Text clearance should not be present in a file with version lower than v8\n");
		err |= parse_coord(&text->clearance, nclearance);
	}
	else
		text->clearance = 0;

	if ((ndir != NULL) && (rctx->rdver >= 6))
		iolht_warn(rctx, nthickness, -1, "Text direction should not be present in a file with version higher than v5 - use text rot instead\n");

	if (nrot != 0) {
		if (rctx->rdver < 6)
			iolht_warn(rctx, nthickness, -1, "Text rot should not be present in a file with version lower than v6\n");
		err |= parse_double(&text->rot, nrot);
	}
	else {
		if (ndir != NULL) {
			err |= parse_int(&dir, ndir);
			dir %= 4;
			if (dir < 0)
				dir += 4;
			text->rot = 90.0 * dir;
		}
		else
			text->rot = 0;
	}

	if (ly != NULL)
		pcb_add_text_on_layer(ly, text, pcb_font(PCB, text->fid, 1));

	return err;
}

static int parse_layer_type(lht_read_t *rctx, pcb_layer_type_t *dst, const char **dst_purpose, lht_node_t *nd, const char *loc)
{
	lht_node_t *flg;
	lht_dom_iterator_t itt;

	if (nd == NULL)
		return -1;

	*dst_purpose = NULL;

	for(flg = lht_dom_first(&itt, nd); flg != NULL; flg = lht_dom_next(&itt)) {
		pcb_layer_type_t val;
		if (rctx->rdver < 6) {
			if (strcmp(flg->name, "outline") == 0) {
				*dst |= PCB_LYT_BOUNDARY;
				*dst_purpose = "uroute";
				continue;
			}
		}
		
		val = pcb_layer_type_str2bit(flg->name);
		if (val == 0)
			iolht_error(flg, "Invalid type name: '%s' in %s (ignoring the type flag)\n", flg->name, loc);
		*dst |= val;
		if (rctx->rdver < 6) {
			if (val & (PCB_LYT_MECH | PCB_LYT_DOC | PCB_LYT_BOUNDARY))
				iolht_warn(rctx, flg, -1, "Potentially invalid type name: '%s' in %s - lihata board before v6 did not support it\n(accepting it for now, but expect broken layer stack)\n", flg->name, loc);
		}
	}

	return 0;
}

static pcb_layer_combining_t parse_comb(pcb_board_t *pcb, lht_node_t *ncmb)
{
	lht_node_t *n;
	pcb_layer_combining_t comb = 0;
	lht_dom_iterator_t it;

	for(n = lht_dom_first(&it, ncmb); n != NULL; n = lht_dom_next(&it)) {
		pcb_layer_combining_t cval;
		if (n->type != LHT_TEXT) {
			iolht_error(n, "Ignoring non-text combining flag\n");
			continue;
		}
		cval = pcb_layer_comb_str2bit(n->name);
		if (cval == 0)
			iolht_error(n, "Ignoring unknown combining flag: '%s'\n", n->name);
		comb |= cval;
	}

	return comb;
}

static int parse_data_layer(lht_read_t *rctx, pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *grp, int layer_id, int bound, pcb_data_t *subc_parent)
{
	lht_node_t *n, *lst, *ncmb, *nvis, *npurp;
	lht_dom_iterator_t it;
	pcb_layer_t *ly = &dt->Layer[layer_id];

	if (layer_id >= PCB_MAX_LAYER)
		return iolht_error(grp, "Board has more layers than supported by this compilation of pcb-rnd (%d)\nIf this is a valid board, please increase PCB_MAX_LAYER and recompile.\n", PCB_MAX_LAYER);
	if (layer_id >= dt->LayerN)
		dt->LayerN = layer_id+1;

	ly->parent.data = dt;
	ly->parent_type = PCB_PARENT_DATA;
	ly->type = PCB_OBJ_LAYER;

	/* Layer attributes are not loaded if config is not to be loaded; this
	   happens when the board is loaded withotu side effects (e.g. into buffer)
	   so that layer keys do not need to be updated. In such situation losing
	   other layer attributes is not a big deal either, for now. */
	if (rctx->cfg_dest != RND_CFR_invalid)
		parse_attributes(&ly->Attributes, lht_dom_hash_get(grp, "attributes"));

	ncmb = lht_dom_hash_get(grp, "combining");
	if (ncmb != NULL) {
		if (rctx->rdver < 2)
			iolht_warn(rctx, ncmb, 1, "Version 1 lihata board should not have combining subtree for layers\n");
		ly->comb = parse_comb(pcb, ncmb);
	}

	npurp = lht_dom_hash_get(grp, "purpose");
	if ((rctx->rdver < 6) && (npurp != NULL))
		iolht_warn(rctx, npurp, -1, "Lihata board below v6 should not have layer purpose (the file may not load correctly in older versions of pcb-rnd)\n");

	if (!bound && (npurp != NULL))
		iolht_warn(rctx, npurp, -1, "Only bound layers should have purpose - ignoring this field\n");

	if (bound) {
		const char *prp;
		ly->is_bound = 1;
		ly->name = rnd_strdup(grp->name);
		parse_int(&dt->Layer[layer_id].meta.bound.stack_offs, lht_dom_hash_get(grp, "stack_offs"));
		parse_layer_type(rctx, &dt->Layer[layer_id].meta.bound.type, &prp, lht_dom_hash_get(grp, "type"), "bound layer");
		if (npurp != NULL) { /* use the explicit purpose if it is set */
			if (npurp->type == LHT_TEXT)
				dt->Layer[layer_id].meta.bound.purpose = rnd_strdup(npurp->data.text.value);
			else
				iolht_warn(rctx, npurp, -1, "Layers purpose shall be text - ignoring this field\n");
		}
		else if (prp != NULL) /* or the implicit one from parse_layer_type(rctx, ), for old versions */
			dt->Layer[layer_id].meta.bound.purpose = rnd_strdup(prp);

		if (pcb != NULL) {
			dt->Layer[layer_id].meta.bound.real = pcb_layer_resolve_binding(pcb, &dt->Layer[layer_id]);
			if (dt->Layer[layer_id].meta.bound.real != NULL)
				pcb_layer_link_trees(&dt->Layer[layer_id], dt->Layer[layer_id].meta.bound.real);
			else if (!(dt->Layer[layer_id].meta.bound.type & PCB_LYT_VIRTUAL))
				iolht_warn(rctx, ncmb, 2, "Can't bind subcircuit layer %s: can't find anything similar on the current board\n", dt->Layer[layer_id].name);
			if (subc_parent != NULL)
				dt->padstack_tree = subc_parent->padstack_tree;
		}
	}
	else {
		/* real */
		lht_node_t *nclr;
		ly->name = rnd_strdup(grp->name);
		nclr = hash_get(grp, "color", 1);
		if ((nclr != NULL) && (nclr->type != LHT_INVALID_TYPE)) {
			if (rctx->rdver < 5)
				iolht_warn(rctx, nclr, 1, "layer color was not supprted before lihata board v5 (reading from v%d)\n", rctx->rdver);
			if (nclr->type == LHT_TEXT) {
				if (rnd_color_load_str(&ly->meta.real.color, nclr->data.text.value) != 0)
					return iolht_error(nclr, "Invalid color: '%s'\n", nclr->data.text.value);
			}
			else
				iolht_warn(rctx, nclr, 1, "Ignoring color: text node required\n");
		}

		nvis = hash_get(grp, "visible", 1);
		if ((nvis != &missing_ok) && rctx->rdver >= 6)
			iolht_warn(rctx, nvis, -1, "saving layer visibility was supported only before lihata board v6 (reading from v%d)\n", rctx->rdver);
		parse_bool(&ly->meta.real.vis, nvis);
		if (pcb != NULL) {
			int grp_id;
			parse_int(&grp_id, hash_get(grp, "group", 0));
			dt->Layer[layer_id].meta.real.grp = grp_id;
	/*		rnd_trace("parse_data_layer name: %d,%d '%s' grp=%d\n", layer_id, dt->LayerN-1, ly->name, grp_id);*/
		}
	}

	lst = lht_dom_hash_get(grp, "objects");
	if (lst != NULL) {
		if (lst->type != LHT_LIST)
			return iolht_error(lst, "objects must be in a list\n");

		for(n = lht_dom_first(&it, lst); n != NULL; n = lht_dom_next(&it)) {
			if (strncmp(n->name, "line.", 5) == 0)
				parse_line(rctx, ly, n, 0, 0);
			if (strncmp(n->name, "arc.", 4) == 0)
				parse_arc(rctx, ly, n, 0, 0);
			if (strncmp(n->name, "gfx.", 4) == 0)
				parse_gfx(rctx, pcb, ly, n, 0, 0);
			if (strncmp(n->name, "polygon.", 8) == 0)
				parse_polygon(rctx, ly, n);
			if (strncmp(n->name, "text.", 5) == 0)
				parse_pcb_text(rctx, ly, n);
		}
	}

	return 0;
}

static int parse_data_layers(lht_read_t *rctx, pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *grp, int bound, pcb_data_t *subc_parent)
{
	int id;
	lht_node_t *n;
	lht_dom_iterator_t it;

	for(id = 0, n = lht_dom_first(&it, grp); n != NULL; id++, n = lht_dom_next(&it))
		if (n->type == LHT_HASH)
			if (parse_data_layer(rctx, pcb, dt, n, id, bound, subc_parent) < 0)
				return -1;

	return 0;
}

static int parse_pstk(lht_read_t *rctx, pcb_data_t *dt, lht_node_t *obj)
{
	pcb_pstk_t *ps;
	lht_node_t *thl, *t;
	unsigned char intconn = 0;
	unsigned long int pid;
	int tmp, err = 0;
	long int id;

	if (obj->type != LHT_HASH)
		return iolht_error(obj, "pstk.ID must be a hash\n");

	parse_ulong(&pid, lht_dom_hash_get(obj, "proto"));
	if (pcb_pstk_get_proto_(dt, pid) == NULL)
		return iolht_error(obj, "Padstack references to non-existent prototype\n");

	if (parse_id(&id, obj, 13) != 0)
		return -1;

	ps = pcb_pstk_alloc_id(dt, id);

	parse_flags(rctx, &ps->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_PSTK, &intconn, 0);
	pcb_attrib_compat_set_intconn(&ps->Attributes, intconn);
	parse_attributes(&ps->Attributes, lht_dom_hash_get(obj, "attributes"));

	err |= parse_coord(&ps->x,    hash_get(obj, "x", 0));
	err |= parse_coord(&ps->y,    hash_get(obj, "y", 0));
	err |= parse_double(&ps->rot, hash_get(obj, "rot", 0));
	tmp = 0;
	err |= parse_int(&tmp,        hash_get(obj, "xmirror", 1));
	ps->xmirror = tmp;
	tmp = 0;
	err |= parse_int(&tmp,        hash_get(obj, "smirror", 1));
	ps->smirror = tmp;
	ps->Clearance = 0;
	parse_coord(&ps->Clearance,   hash_get(obj, "clearance", 1));
	ps->proto = pid;
	if (err != 0)
		return -1;

	thl = lht_dom_hash_get(obj, "thermal");
	if ((thl != NULL) && (thl->type == LHT_LIST)) {
		int max, n;
		max = 0;
		for(t = thl->data.list.first, n = 0; t != NULL; t = t->next,n++)
		{
			if (t->type == LHT_LIST) {
				char *end;
				int ly = strtol(t->name, &end, 10);
				if ((*end == '\0') && (ly > max))
					max = ly;
			}
		}
		if (n > max)
			max = n;

		ps->thermals.used = max+1;
		ps->thermals.shape = calloc(sizeof(ps->thermals.shape[0]), ps->thermals.used);
		for(t = thl->data.list.first, n = 0; t != NULL; t = t->next, n++) {
			int i;
			if (t->type == LHT_TEXT) {
				parse_int(&i, t);
				ps->thermals.shape[n] = i;
			}
			else if (t->type == LHT_LIST) {
				unsigned char dst;
				char *end;
				int ly = strtol(t->name, &end, 10);
				if ((*end == '\0') && (ly < ps->thermals.used)) {
					parse_thermal(&dst, t);
					ps->thermals.shape[ly] = dst;
				}
			}
		}
	}

	pcb_pstk_add(dt, ps);

	return 0;
}

static void warn_old_model(lht_read_t *rctx, lht_node_t *obj, char *type, int warnid)
{
	unsigned long warnbit = 1ul << warnid;
	if ((rctx->rdver < 5) || (rctx->old_model_warned & warnbit))
		return;

	rctx->old_model_warned |= warnbit;
	iolht_warn(rctx, obj, -1, "Lihata from v5 does not support the old data model (elements, pins, pads and vias);\nyour file contains %s that will be converted to the new model\n", type);
}

static int parse_via(lht_read_t *rctx, pcb_data_t *dt, lht_node_t *obj, rnd_coord_t dx, rnd_coord_t dy, int subc_on_bottom)
{
	pcb_pstk_t *ps;
	unsigned char intconn = 0;
	rnd_coord_t Thickness, Clearance, Mask = 0, DrillingHole, X, Y;
	char *Name = NULL, *Number = NULL;
	pcb_flag_t flg;
	lht_node_t *fln;
	int err = 0;
	long int id;

	if (dt == NULL)
		return -1;

	warn_old_model(rctx, obj, "via", 1);

	parse_flags(rctx, &flg, fln=lht_dom_hash_get(obj, "flags"), PCB_OBJ_VIA, &intconn, 1);
	err |= parse_coord(&Thickness,    hash_get(obj, "thickness", 0));
	err |= parse_coord(&Clearance,    hash_get(obj, "clearance", 0));
	err |= parse_coord(&Mask,         hash_get(obj, "mask", 1));
	err |= parse_coord(&DrillingHole, hash_get(obj, "hole", 0));
	err |= parse_coord(&X,            hash_get(obj, "x", 0));
	err |= parse_coord(&Y,            hash_get(obj, "y", 0));
	err |= parse_text(&Name,          hash_get(obj, "name", 1));
	err |= parse_text(&Number,        hash_get(obj, "number", 1));
	if (err != 0)
		return -1;

	if (parse_id(&id, obj, 4) != 0)
		return -1;

	ps = pcb_old_via_new(dt, id, X+dx, Y+dy, Thickness, Clearance, Mask, DrillingHole, Name, flg);
	if (ps == NULL) {
		iolht_error(obj, "Failed to convert old via to padstack (this via is LOST)\n");
		return 0;
	}

	pcb_attrib_compat_set_intconn(&ps->Attributes, intconn);
	parse_attributes(&ps->Attributes, lht_dom_hash_get(obj, "attributes"));

	parse_thermal_old(rctx, (pcb_any_obj_t *)ps, fln);

	if (Number != NULL)
		pcb_attribute_put(&ps->Attributes, "term", Number);
	if (Name != NULL)
		pcb_attribute_put(&ps->Attributes, "name", Name);

	if (subc_on_bottom)
		pcb_pstk_mirror(ps, PCB_PSTK_DONT_MIRROR_COORDS, 1, 0, 0);

	return err;
}

static int parse_pad(lht_read_t *rctx, pcb_subc_t *subc, lht_node_t *obj, rnd_coord_t dx, rnd_coord_t dy, int subc_on_bottom)
{
	pcb_pstk_t *p;
	unsigned char intconn = 0;
	pcb_flag_t flg;
	rnd_coord_t X1, Y1, X2, Y2, Thickness, Clearance, Mask = 0;
	char *Name = NULL, *Number = NULL;
	int err = 0;
	long int id;

	warn_old_model(rctx, obj, "pad", 2);

	parse_flags(rctx, &flg, lht_dom_hash_get(obj, "flags"), PCB_OBJ_PAD, &intconn, 0);

	err |= parse_coord(&Thickness, hash_get(obj, "thickness", 0));
	err |= parse_coord(&Clearance, hash_get(obj, "clearance", 0));
	err |= parse_coord(&Mask,      hash_get(obj, "mask", 1));
	err |= parse_coord(&X1,        hash_get(obj, "x1", 0));
	err |= parse_coord(&Y1,        hash_get(obj, "y1", 0));
	err |= parse_coord(&X2,        hash_get(obj, "x2", 0));
	err |= parse_coord(&Y2,        hash_get(obj, "y2", 0));
	err |= parse_text(&Name,       hash_get(obj, "name", 1));
	err |= parse_text(&Number,     hash_get(obj, "number", 1));

	if (err != 0)
		return -1;

	if (parse_id(&id, obj, 4) != 0)
		return -1;

	p = pcb_pstk_new_compat_pad(subc->data, id, X1+dx, Y1+dy, X2+dx, Y2+dy, Thickness, Clearance, Mask, flg.f & PCB_FLAG_SQUARE, flg.f & PCB_FLAG_NOPASTE, (!!(flg.f & PCB_FLAG_ONSOLDER)));
	if (Number != NULL)
		pcb_attribute_put(&p->Attributes, "term", Number);
	if (Name != NULL)
		pcb_attribute_put(&p->Attributes, "name", Name);

	if (subc_on_bottom)
		pcb_pstk_mirror(p, PCB_PSTK_DONT_MIRROR_COORDS, 1, 0, 0);

	pcb_attrib_compat_set_intconn(&p->Attributes, intconn);
	parse_attributes(&p->Attributes, lht_dom_hash_get(obj, "attributes"));

	return 0;
}


static int parse_element(lht_read_t *rctx, pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *obj)
{
	pcb_subc_t *subc = pcb_subc_alloc();
	pcb_layer_t *silk = NULL;
	lht_node_t *lst, *n;
	lht_dom_iterator_t it;
	int onsld, tdir = 0, tscale = 100;
	rnd_coord_t ox = 0, oy = 0, tx, ty;
	pcb_text_t *txt;
	int err = 0;

	warn_old_model(rctx, obj, "element", 3);

	if (parse_id(&subc->ID, obj, 8) != 0)
		return -1;

	pcb_subc_reg(dt, subc);
	pcb_obj_id_reg(dt, subc);

	parse_flags(rctx, &subc->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_ELEMENT, NULL, 0);
	parse_attributes(&subc->Attributes, lht_dom_hash_get(obj, "attributes"));
	err |= parse_coord(&ox, hash_get(obj, "x", 0));
	err |= parse_coord(&oy, hash_get(obj, "y", 0));
	tx = ox;
	ty = oy;

	onsld = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, subc);
	subc->Flags.f &= ~PCB_FLAG_ONSOLDER;

	/* bind the via rtree so that vias added in this subc show up on the board */
	if (pcb != NULL)
		pcb_subc_bind_globals(pcb, subc);

	/* the only layer objects are put on from and old element is the primary silk layer */
	{
		pcb_layer_type_t silk_side = onsld ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
		const char *name = onsld ? "bottom-silk" : "top-silk";
		silk = pcb_subc_get_layer(subc, PCB_LYT_SILK | silk_side, /*PCB_LYC_AUTO*/0, rnd_true, name, rnd_false);
	}

	lst = lht_dom_hash_get(obj, "objects");
	if (lst == NULL)
		return iolht_error(obj, "Invalid element: no objects\n");
	if (lst->type == LHT_LIST) {
		for(n = lht_dom_first(&it, lst); n != NULL; n = lht_dom_next(&it)) {
			if (strncmp(n->name, "line.", 5) == 0)
				parse_line(rctx, silk, n, ox, oy);
			if (strncmp(n->name, "arc.", 4) == 0)
				parse_arc(rctx, silk, n, ox, oy);
			if (strncmp(n->name, "text.", 5) == 0) {
				lht_node_t *role   = lht_dom_hash_get(n, "role");
				lht_node_t *string = lht_dom_hash_get(n, "string");
				if ((role != NULL) && (role->type == LHT_TEXT) && (string != NULL) && (string->type == LHT_TEXT)) {
					const char *key = role->data.text.value;
					const char *val = string->data.text.value;
					if (strcmp(key, "desc") == 0)   pcb_attribute_put(&subc->Attributes, "footprint", val);
					if (strcmp(key, "value") == 0)  pcb_attribute_put(&subc->Attributes, "value", val);
					if (strcmp(key, "name") == 0)   pcb_attribute_put(&subc->Attributes, "refdes", val);
					err |= parse_coord(&tx,     hash_get(n, "x", 0));
					err |= parse_coord(&ty,     hash_get(n, "y", 0));
					err |= parse_coord(&tdir,   hash_get(n, "direction", 1));
					err |= parse_coord(&tscale, hash_get(n, "scale", 1));
				}
			}
			if (strncmp(n->name, "pin.", 4) == 0)
				parse_via(rctx, subc->data, n, ox, oy, onsld);
			if (strncmp(n->name, "pad.", 4) == 0)
				parse_pad(rctx, subc, n, ox, oy, onsld);
		}
	}
	else
		return iolht_error(obj, "invalid element: objects is not a list\n");

	txt = pcb_subc_add_refdes_text(subc, tx, ty, tdir, tscale, onsld);

	pcb_subc_xy_rot_pnp(subc, ox, oy, onsld);

	pcb_subc_bbox(subc);

	if (dt->subc_tree == NULL)
		rnd_rtree_init(dt->subc_tree = malloc(sizeof(rnd_rtree_t)));
	rnd_rtree_insert(dt->subc_tree, subc, (rnd_rtree_box_t *)subc);

	pcb_subc_rebind(pcb, subc);

	return err;
}

static int parse_subc(lht_read_t *rctx, pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *obj, pcb_subc_t **subc_out)
{
	pcb_subc_t *sc = pcb_subc_alloc();
	unsigned char intconn = 0;
	int n;

	if (obj->type != LHT_HASH)
		return iolht_error(obj, "subc.ID must be a hash\n");

	parse_id(&sc->ID, obj, 5);
	parse_flags(rctx, &sc->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_ELEMENT, &intconn, 0);
	pcb_attrib_compat_set_intconn(&sc->Attributes, intconn);
	parse_attributes(&sc->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_minuid(sc->uid, lht_dom_hash_get(obj, "uid"));

	if (!dt->padstack_tree)
		rnd_rtree_init(dt->padstack_tree = malloc(sizeof(rnd_rtree_t)));
	sc->data->padstack_tree = dt->padstack_tree;

	pcb_subc_reg(dt, sc);
	pcb_obj_id_reg(dt, sc);

	if (parse_data(rctx, pcb, sc->data, lht_dom_hash_get(obj, "data"), dt) == 0)
		return iolht_error(obj, "Invalid subc: no data\n");

	for(n = 0; n < sc->data->LayerN; n++)
		sc->data->Layer[n].is_bound = 1;

	pcb_data_bbox(&sc->BoundingBox, sc->data, rnd_true);
	pcb_data_bbox_naked(&sc->bbox_naked, sc->data, rnd_true);

	if (!dt->subc_tree)
		rnd_rtree_init(dt->subc_tree = malloc(sizeof(rnd_rtree_t)));
	rnd_rtree_insert(dt->subc_tree, sc, (rnd_rtree_box_t *)sc);

	if (subc_out != NULL)
		*subc_out = sc;

	return 0;
}


static int parse_data_objects(lht_read_t *rctx, pcb_board_t *pcb_for_font, pcb_data_t *dt, lht_node_t *grp)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (grp->type != LHT_LIST)
		return iolht_error(grp, "groups must be a list\n");

	for(n = lht_dom_first(&it, grp); n != NULL; n = lht_dom_next(&it)) {
		if (strncmp(n->name, "padstack_ref.", 13) == 0)
			parse_pstk(rctx, dt, n);
		if (strncmp(n->name, "via.", 4) == 0)
			parse_via(rctx, dt, n, 0, 0, 0);
		if (strncmp(n->name, "rat.", 4) == 0)
			parse_rat(rctx, pcb_for_font, dt, n);
		else if (strncmp(n->name, "element.", 8) == 0)
			parse_element(rctx, pcb_for_font, dt, n);
		else if (strncmp(n->name, "subc.", 5) == 0)
			if (parse_subc(rctx, pcb_for_font, dt, n, NULL) != 0)
				return iolht_error(n, "failed to parse subcircuit\n");
	}

	return 0;
}

static void layer_fixup(pcb_board_t *pcb)
{
	int n;
	rnd_layergrp_id_t top_silk, bottom_silk;
	pcb_layergrp_t *g;

	pcb_layergrp_inhibit_inc();

	pcb_layer_group_setup_default(pcb);

	/* old silk assumption: last two layers are silk, bottom and top */
	bottom_silk = pcb->Data->Layer[pcb->Data->LayerN-2].meta.real.grp;
	top_silk = pcb->Data->Layer[pcb->Data->LayerN-1].meta.real.grp;
	pcb->Data->Layer[pcb->Data->LayerN-2].meta.real.grp = -1;
	pcb->Data->Layer[pcb->Data->LayerN-1].meta.real.grp = -1;

/*	rnd_trace("NAME: '%s' '%s'\n", pcb->Data->Layer[pcb->Data->LayerN-1].Name,pcb->Data->Layer[pcb->Data->LayerN-2].Name);*/

	for(n = 0; n < pcb->Data->LayerN - 2; n++) {
		pcb_layer_t *l = &pcb->Data->Layer[n];
		rnd_layergrp_id_t grp = l->meta.real.grp;
		/*rnd_trace("********* l=%d %s g=%ld (top=%ld bottom=%ld)\n", n, l->name, grp, top_silk, bottom_silk);*/
		l->meta.real.grp = -1;

		if (grp == bottom_silk)
			g = pcb_get_grp(&pcb->LayerGroups, PCB_LYT_BOTTOM, PCB_LYT_COPPER);
		else if (grp == top_silk)
			g = pcb_get_grp(&pcb->LayerGroups, PCB_LYT_TOP, PCB_LYT_COPPER);
		else
			g = pcb_get_grp_new_intern(pcb, grp);
/*			rnd_trace(" add %ld\n", g - pcb->LayerGroups.meta.real.grp);*/
		if (g != NULL) {
			pcb_layer_add_in_group_(pcb, g, g - pcb->LayerGroups.grp, n);
			if (strcmp(l->name, "outline") == 0)
				pcb_layergrp_fix_turn_to_outline(g);
		}
		else
			rnd_message(RND_MSG_ERROR, "failed to create layer %s\n", l->name);
	}

	pcb_layergrp_fix_old_outline(pcb);

	/* link in the 2 hardwired silks and mark them auto */
	g = pcb_get_grp(&pcb->LayerGroups, PCB_LYT_BOTTOM, PCB_LYT_SILK);
	pcb_layer_add_in_group_(pcb, g, g - pcb->LayerGroups.grp, pcb->Data->LayerN-2);
	pcb->Data->Layer[pcb->Data->LayerN-2].comb |= PCB_LYC_AUTO;
	
	g = pcb_get_grp(&pcb->LayerGroups, PCB_LYT_TOP, PCB_LYT_SILK);
	pcb_layer_add_in_group_(pcb, g, g - pcb->LayerGroups.grp, pcb->Data->LayerN-1);
	pcb->Data->Layer[pcb->Data->LayerN-1].comb |= PCB_LYC_AUTO;

	pcb_layergrp_inhibit_dec();
}


/* outline layers did not have auto flkag back then but we need that now for padstack slots */
static void outline_fixup(pcb_board_t *pcb)
{
	rnd_layer_id_t n;
	pcb_layergrp_inhibit_inc();

	for(n = 0; n < pcb->Data->LayerN; n++) {
		pcb_layer_t *l = &pcb->Data->Layer[n];
		pcb_layergrp_t *g = &pcb->LayerGroups.grp[l->meta.real.grp];

		if (g->ltype & PCB_LYT_BOUNDARY)
			l->comb |= PCB_LYC_AUTO;
	}

	pcb_layergrp_inhibit_dec();
}

static int validate_layer_stack_lyr(lht_read_t *rctx, pcb_board_t *pcb, lht_node_t *loc)
{
	rnd_layer_id_t tmp[2], lid;
	rnd_layergrp_id_t gid;
		int n, found;

	/* check layer->group cross-links */
	for(lid = 0; lid < pcb->Data->LayerN; lid++) {
		pcb_layergrp_t *grp;
		gid = pcb->Data->Layer[lid].meta.real.grp;
		grp = pcb_get_layergrp(pcb, gid);
		if (grp == NULL)
			return iolht_error(loc, "Broken layer stackup: missing valid group for layer %ld\n", lid);

		for(found = n = 0; n < grp->len; n++) {
			if (grp->lid[n] == lid) {
				found = 1;
				break;
			}
		}

		if (!found)
			return iolht_error(loc, "Broken layer stackup: group->layer backlink for layer %ld\n", lid);
	}

	/* check group->layer cross-links */
	for(gid = 0; gid < pcb->LayerGroups.len; gid++) {
		pcb_layergrp_t *grp = &pcb->LayerGroups.grp[gid];
		for(n = 0; n < grp->len; n++) {
			pcb_layer_t *ly;
			lid = grp->lid[n];
			ly = pcb_get_layer(pcb->Data, lid);
			if (ly == NULL)
				return iolht_error(loc, "Broken layer stackup: group %ld links to non-existent layer %ld\n", gid, lid);
			if (ly->meta.real.grp != gid)
				return iolht_error(loc, "Broken layer stackup: group %ld links to layer %ld which then links back to group %ld instead\n", gid, lid, ly->meta.real.grp);
		}
	}

	if (rctx->rdver == 1) { /* v1 used to require top and bottom silk */
		if (pcb_layer_list(pcb, PCB_LYT_TOP | PCB_LYT_SILK, tmp, 2) < 1)
			iolht_warn(rctx, loc, -1, "Strange layer stackup for v1: top silk layer missing\n");
		if (pcb_layer_list(pcb, PCB_LYT_BOTTOM | PCB_LYT_SILK, tmp, 2) < 1)
			iolht_warn(rctx, loc, -1, "Strange layer stackup for v1: bottom silk layer missing\n");
	}

	if (rctx->rdver < 6) {
		if (pcb_layer_list(pcb, PCB_LYT_BOUNDARY, tmp, 2) > 1)
			return iolht_error(loc, "Unsupported layer stackup: multiple outline layers was not possible before lihata board v6\n");
	}
	return 0;
}

static int validate_layer_stack_grp(lht_read_t *rctx, pcb_board_t *pcb, lht_node_t *loc)
{
	rnd_layergrp_id_t tmp[2];

	if (rctx->rdver == 1) { /*v1 required top and bottom silk */
		if (pcb_layergrp_list(pcb, PCB_LYT_TOP | PCB_LYT_SILK, tmp, 2) < 1)
			iolht_warn(rctx, loc, -1, "Strange layer stackup in v1: top silk layer group missing\n");
		if (pcb_layergrp_list(pcb, PCB_LYT_BOTTOM | PCB_LYT_SILK, tmp, 2) < 1)
			iolht_warn(rctx, loc, -1, "Strange layer stackup in v1: bottom silk layer group missing\n");
	}
	if ((pcb_layergrp_list(pcb, PCB_LYT_BOUNDARY, tmp, 2) > 1) && (rctx->rdver < 6))
		return iolht_error(loc, "Unsupported layer stackup: multiple outline layer groups was not possible before lihata board v6\n");
	return 0;
}

static int parse_layer_stack(lht_read_t *rctx, pcb_board_t *pcb, lht_node_t *nd)
{
	lht_node_t *grps, *grp, *name, *layers, *lyr, *nattr, *npurp;
	lht_dom_iterator_t it, itt;
	long int n;

	for(n = 0; n < PCB_MAX_LAYERGRP; n++)
		pcb_layergrp_free(pcb, n);
	pcb->LayerGroups.len = 0;

	grps = lht_dom_hash_get(nd, "groups");
	for(grp = lht_dom_first(&it, grps); grp != NULL; grp = lht_dom_next(&it)) {
		rnd_layergrp_id_t gid;
		pcb_layergrp_t *g;
		char *end;
		const char *prp;

		if (grp->type != LHT_HASH) {
			iolht_error(grp, "Invalid group in layer stack: '%s' (not a hash; ignoring the group)\n", grp->name);
			continue;
		}

		gid = strtol(grp->name, &end, 10);
		if ((*end != '\0') || (gid < 0)) {
			iolht_error(grp, "Invalid group id in layer stack: '%s' (not an int or negative; ignoring the group)\n", grp->name);
			continue;
		}
		if (gid >= PCB_MAX_LAYERGRP) {
			iolht_error(grp, "Invalid group id in layer stack: '%s' (too many layers; ignoring the group)\n", grp->name);
			continue;
		}

		pcb_layergrp_free(pcb, n); /* just in case of double initialization of the same layer id */

		g = &pcb->LayerGroups.grp[gid];
		if (pcb->LayerGroups.len <= gid)
			pcb->LayerGroups.len = gid+1;
		pcb_layergrp_setup(g, pcb);
		g->purpose = NULL;
		g->purpi = -1;

		/* set name and type*/
		name = lht_dom_hash_get(grp, "name");
		if ((name == NULL) || (name->type != LHT_TEXT) || (name->data.text.value == NULL)) {
			g->name = malloc(32);
			sprintf(g->name, "grp_%ld", gid);
		}
		else
			g->name = rnd_strdup(name->data.text.value);
		parse_layer_type(rctx, &g->ltype, &prp, lht_dom_hash_get(grp, "type"), g->name);

		if (rctx->rdver < 6) {
			if ((g->ltype & PCB_LYT_DOC) || (g->ltype & PCB_LYT_MECH))
				iolht_warn(rctx, grp, -1, "Layer groups could not have type DOC or MECH before lihata v6 - still loading these types,\nbut they will be ignored by older versions of pcb-rnd.\n");
		}

		npurp = lht_dom_hash_get(grp, "purpose");
		if (npurp != NULL) { /* use the explicit purpose field if found */
			if (rctx->rdver < 6)
				iolht_warn(rctx, grp, -1, "Layer groups could not have a purpose field before lihata v6 - still loading the purpose,\nbut it will be ignored by older versions of pcb-rnd.\n");
			if (npurp->type == LHT_TEXT)
				pcb_layergrp_set_purpose__(g, rnd_strdup(npurp->data.text.value), 0);
			else
				iolht_warn(rctx, npurp, -1, "Group purpose shall be text - ignoring this field\n");
		}
		else if (prp != NULL) /* or the implicit one returned by parse_layer_type(rctx, ) */
			pcb_layergrp_set_purpose__(g, rnd_strdup(prp), 0);

		/* load attributes */
		nattr = lht_dom_hash_get(grp, "attributes");
		if (nattr != NULL) {
			if (rctx->rdver < 5)
				iolht_warn(rctx, nattr, 3, "Layer groups could not have attributes before lihata v5 - still loading these attributes,\nbut they will be ignored by older versions of pcb-rnd.\n");
			if (parse_attributes(&g->Attributes, nattr) < 0)
				return iolht_error(nattr, "failed to load attributes\n");
		}

		/* load layers */
		layers = lht_dom_hash_get(grp, "layers");
		if (layers != NULL) {
			for(lyr = lht_dom_first(&itt, layers); lyr != NULL; lyr = lht_dom_next(&itt)) {
				rnd_layer_id_t lid;
				if (lyr->type != LHT_TEXT) {
					iolht_error(lyr, "Invalid layer node type in group '%s' (ignoring the layer)\n", g->name);
					continue;
				}
				lid = strtol(lyr->data.text.value, &end, 10);
				if ((*end != '\0') || (lid < 0)) {
					iolht_error(lyr, "Invalid layer id '%s' in group '%s' (not an int or negative; ignoring the layer)\n", lyr->data.text.value, g->name);
					continue;
				}
				if (g->len >= PCB_MAX_LAYER) {
					iolht_error(lyr, "Too many layers  in group '%s' (ignoring the layer)\n", g->name);
					continue;
				}
				g->lid[g->len] = lid;
				g->len++;
			}
		}
	}
	return validate_layer_stack_grp(rctx, pcb, nd);
}

static int parse_data_pstk_shape_poly(lht_read_t *rctx, pcb_board_t *pcb, pcb_pstk_shape_t *dst, lht_node_t *nshape, pcb_data_t *subc_parent)
{
	lht_node_t *n;
	rnd_cardinal_t i;

	dst->shape = PCB_PSSH_POLY;
	dst->data.poly.x = NULL; /* if we return before the allocation... */
	dst->data.poly.len = 0;
	for(n = nshape->data.list.first; n != NULL; n = n->next)
		dst->data.poly.len++;

	if ((dst->data.poly.len % 2) != 0)
		return iolht_error(n, "odd number of padstack shape polygon points\n");

	dst->data.poly.len /= 2;

	pcb_pstk_shape_alloc_poly(&dst->data.poly, dst->data.poly.len);
	for(n = nshape->data.list.first, i = 0; n != NULL; i++) {
		if (parse_coord(&dst->data.poly.x[i], n) != 0) return -1;
		n = n->next;
		if (parse_coord(&dst->data.poly.y[i], n) != 0) return -1;
		n = n->next;
	}
	return 0;
}

static int parse_data_pstk_shape_line(lht_read_t *rctx, pcb_board_t *pcb, pcb_pstk_shape_t *dst, lht_node_t *nshape, pcb_data_t *subc_parent)
{
	int sq;
	int err = 0;

	dst->shape = PCB_PSSH_LINE;

	err |= parse_coord(&dst->data.line.x1,        hash_get(nshape, "x1", 0));
	err |= parse_coord(&dst->data.line.y1,        hash_get(nshape, "y1", 0));
	err |= parse_coord(&dst->data.line.x2,        hash_get(nshape, "x2", 0));
	err |= parse_coord(&dst->data.line.y2,        hash_get(nshape, "y2", 0));
	err |= parse_coord(&dst->data.line.thickness, hash_get(nshape, "thickness", 0));
	err |= parse_int(&sq,                         hash_get(nshape, "square", 0));
	dst->data.line.square = sq;
	return err;
}

static int parse_data_pstk_shape_circ(lht_read_t *rctx, pcb_board_t *pcb, pcb_pstk_shape_t *dst, lht_node_t *nshape, pcb_data_t *subc_parent)
{
	int err = 0;
	dst->shape = PCB_PSSH_CIRC;

	err |= parse_coord(&dst->data.circ.x,   hash_get(nshape, "x", 0));
	err |= parse_coord(&dst->data.circ.y,   hash_get(nshape, "y", 0));
	err |= parse_coord(&dst->data.circ.dia, hash_get(nshape, "dia", 0));
	return err;
}

static int parse_data_pstk_shape_hshadow(lht_read_t *rctx, pcb_board_t *pcb, pcb_pstk_shape_t *dst, lht_node_t *nshape, pcb_data_t *subc_parent)
{
	dst->shape = PCB_PSSH_HSHADOW;
	if (rctx->rdver < 6)
		iolht_warn(rctx, nshape, 7, "lihata board before v6 did not support padstack shape hshadow\n");
	return 0;
}

static int parse_data_pstk_shape_v4(lht_read_t *rctx, pcb_board_t *pcb, pcb_pstk_shape_t *dst, lht_node_t *nshape, pcb_data_t *subc_parent)
{
	lht_node_t *ncmb, *nlyt, *ns;
	int res = -1;
	const char *prp;

	nlyt = lht_dom_hash_get(nshape, "layer_mask");
	if ((nlyt != NULL) && (nlyt->type == LHT_HASH))
		res = parse_layer_type(rctx, &dst->layer_mask, &prp, nlyt, "padstack shape");

TODO("layer: shape v6 and support for prp")

	if (res != 0)
		return iolht_error(nlyt != NULL ? nlyt : nshape, "Failed to parse pad stack shape (layer mask)\n");

	if (dst->layer_mask == 0)
		iolht_warn(rctx, nlyt, -1, "Failed to parse pad stack shape (empty layer mask)\nThe padstack may have shapes that will behave strangely - please fix it manually\n");

	ncmb = lht_dom_hash_get(nshape, "combining");
	if ((ncmb != NULL) && (ncmb->type == LHT_HASH))
		dst->comb = parse_comb(pcb, ncmb);

	if (parse_coord(&dst->clearance, lht_dom_hash_get(nshape, "clearance")) != 0) return -1;

	ns = lht_dom_hash_get(nshape, "ps_poly");
	if ((ns != NULL) && (ns->type == LHT_LIST)) return parse_data_pstk_shape_poly(rctx, pcb, dst, ns, subc_parent);

	ns = lht_dom_hash_get(nshape, "ps_line");
	if ((ns != NULL) && (ns->type == LHT_HASH)) return parse_data_pstk_shape_line(rctx, pcb, dst, ns, subc_parent);

	ns = lht_dom_hash_get(nshape, "ps_circ");
	if ((ns != NULL) && (ns->type == LHT_HASH)) return parse_data_pstk_shape_circ(rctx, pcb, dst, ns, subc_parent);

	ns = lht_dom_hash_get(nshape, "ps_hshadow");
	if ((ns != NULL) && (ns->type == LHT_TEXT)) return parse_data_pstk_shape_hshadow(rctx, pcb, dst, ns, subc_parent);

	return iolht_error(nshape, "Failed to parse pad stack: missing shape\n");
}


static int parse_data_pstk_proto(lht_read_t *rctx, pcb_board_t *pcb, pcb_pstk_proto_t *dst, lht_node_t *nproto, pcb_data_t *subc_parent, int prver)
{
	int itmp, i;
	lht_node_t *nshape, *n;
	pcb_pstk_tshape_t *ts;

	switch(prver) {
		case 4:
			if (rctx->rdver >= 6)
				iolht_warn(rctx, nproto, 6, "lihata board from v6 should use padstack prototype v6\n");
			break;
		case 6:
			if (rctx->rdver < 6)
				iolht_warn(rctx, nproto, 6, "lihata board nefore v6 did not have padstack prototype v6\n");
			break;
		default:
			return iolht_error(nproto, "invalid padstack prototype version\n");
	}

	n = lht_dom_hash_get(nproto, "name");
	if (n != NULL) {
		dst->name = rnd_strdup(n->data.text.value);
		if (rctx->rdver < 5)
			iolht_warn(rctx, n, 6, "lihata board before v5 did not support padstack prototype names\n");
	}
	else
		dst->name = NULL;

	/* read the hole */
	if (parse_coord(&dst->hdia, lht_dom_hash_get(nproto, "hdia")) != 0) return -1;
	if (parse_int(&dst->htop, lht_dom_hash_get(nproto, "htop")) != 0) return -1;
	if (parse_int(&dst->hbottom, lht_dom_hash_get(nproto, "hbottom")) != 0) return -1;
	if (parse_int(&itmp, lht_dom_hash_get(nproto, "hplated")) != 0) return -1;
	dst->hplated = itmp;
	dst->in_use = 1;
	if (subc_parent != NULL)
		dst->parent = subc_parent;
	else if (pcb != NULL)
		dst->parent = pcb->Data;
	else
		dst->parent = NULL;

	/* read shapes */
	nshape = lht_dom_hash_get(nproto, "shape");
	if ((nshape == NULL) || (nshape->type != LHT_LIST))
		return iolht_error(nshape, "shape must be an existing list\n");

	ts = pcb_vtpadstack_tshape_get(&dst->tr, 0, 1);

	for(n = nshape->data.list.first, ts->len = 0; n != NULL; n = n->next) ts->len++;
	ts->shape = calloc(sizeof(pcb_pstk_shape_t), ts->len);

	for(n = nshape->data.list.first, i = 0; n != NULL; n = n->next, i++)
		if ((n->type == LHT_HASH) && (strcmp(n->name, "ps_shape_v4") == 0))
			if (parse_data_pstk_shape_v4(rctx, pcb, ts->shape+i, n, subc_parent) != 0)
				goto error;

	pcb_pstk_proto_update(dst);

	return 0;
	error:;
	free(ts->shape);
	ts->shape = NULL;
	ts->len = 0;
	return iolht_error(n, "failed to parse padstack due to bad shape\n");
}

static int parse_data_pstk_protos(lht_read_t *rctx, pcb_board_t *pcb, pcb_data_t *dst, lht_node_t *pp, pcb_data_t *subc_parent)
{
	rnd_cardinal_t pid, len;
	lht_node_t *pr;
	int res = 0;

	for(len = 0, pr = pp->data.list.first; pr != NULL; pr = pr->next) len++;

	if (len == 0)
		return 0; /* there are no prototypes to load */

	pcb_vtpadstack_proto_enlarge(&dst->ps_protos, len-1);
	for(pid = 0, pr = pp->data.list.first; ((pr != NULL) && (res == 0)); pr = pr->next, pid++) {
		if ((pr->type == LHT_TEXT) && (strcmp(pr->name, "unused") == 0))
			continue;
		else if ((pr->type == LHT_HASH) && (strncmp(pr->name, "ps_proto_v", 10) == 0)) {
			char *sid = pr->name+11, *end;
			long int pid_in_file;
			int prver = strtol(pr->name+10, &end, 10);

			if ((*end != '\0') && (*end != '.'))
				return iolht_error(pr, "Invalid padstack proto version '%s' (not an integer)\n", pr->name+10);

			if (*sid == '.') {
				sid++;
				pid_in_file = strtol(sid, &end, 0);
				if (*end != '\0')
					return iolht_error(pr, "Invalid padstack proto ID '%s' (not an integer)\n", sid);
				else if (pid_in_file < pid)
					return iolht_error(pr, "Invalid padstack proto ID '%s' (can't rewind)\n", sid);
				pid = pid_in_file;
			}
			else if (*sid != '\0')
				return iolht_error(pr, "Invalid padstack proto ID '%s' (syntax)\n", sid);
			if (pid >= dst->ps_protos.used)
				pcb_vtpadstack_proto_enlarge(&dst->ps_protos, pid);
			res = parse_data_pstk_proto(rctx, pcb, dst->ps_protos.array + pid, pr, dst, prver);
			if (res != 0)
				return iolht_error(pr, "Invalid padstack proto definition\n");
		}
		else
			return iolht_error(pr, "Invalid padstack proto definition\n", pp->name);
	}

	return res;
}

static pcb_data_t *parse_data(lht_read_t *rctx, pcb_board_t *pcb, pcb_data_t *dst, lht_node_t *nd, pcb_data_t *subc_parent)
{
	pcb_data_t *dt;
	lht_node_t *grp;
	int bound_layers = (subc_parent != NULL);

	if (subc_parent == &DUMMY_BUFFER_SUBC)
		subc_parent = NULL;

	if ((nd == NULL) || (nd->type != LHT_HASH))
		return NULL;

	if (dst == NULL)
		dt = pcb_buffer_new(pcb);
	else
		dt = dst;

	if (!bound_layers)
		pcb->Data = dt;

	grp = lht_dom_hash_get(nd, "layers");
	if ((grp != NULL) && (grp->type == LHT_LIST))
		parse_data_layers(rctx, pcb, dt, grp, bound_layers, subc_parent);

	if (rctx->rdver == 1)
		layer_fixup(pcb);

	if ((rctx->rdver < 6) && (pcb != NULL))
		outline_fixup(pcb);

	if (rctx->rdver >= 4) {
		grp = lht_dom_hash_get(nd, "padstack_prototypes");
		if ((grp != NULL) && (grp->type == LHT_LIST))
			parse_data_pstk_protos(rctx, pcb, dt, grp, subc_parent);
	}

	grp = lht_dom_hash_get(nd, "objects");
	if (grp != NULL)
		if (parse_data_objects(rctx, pcb, dt, grp) != 0)
			return NULL;

	return dt;
}


#define PARSE_COORD(dst, src)       parse_coord(dst, src)
#define PARSE_DOUBLE(dst, src)      parse_double(dst, src)
#define HASH_GET(hash, name)        hash_get(hash, name, 0)
#define RND_LHT_ERROR               iolht_error

#include <librnd/font/font_lht.h>

#undef PARSE_COORD
#undef PARSE_DOUBLE
#undef PARSE_HASH_GET
#undef RND_LHT_ERROR

static int parse_fontkit(pcb_fontkit_t *fk, lht_node_t *nd)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (nd->type != LHT_HASH)
		return iolht_error(nd, "fontkit must be a hash\n");

	pcb_fontkit_reset (fk);

	for(n = lht_dom_first(&it, nd); n != NULL; n = lht_dom_next(&it)) {
		rnd_font_t *f;

		if (strcmp(n->name, "geda_pcb") != 0) {
			char *end;
			int id = strtol(n->name, &end, 10);
			if (*end != '\0')
				continue; /* ingore fonts with invalid name for now - maybe it'd be safer to read the ID field */
			f = pcb_new_font(fk, id, NULL);
			if (f == NULL)
				return iolht_error(nd, "Failed to allocate font id %d (name '%s').\n", id, n->name);
		}
		else {
			rnd_font_free(&fk->dflt);
			fk->dflt.id = 0; /* restore default font's ID */
			f = &fk->dflt;
		}

		if (rnd_font_lht_parse_font(f, n) != 0)
			return -1;
	}

	return 0;
}


static void post_ids_assign(vtp0_t *ids)
{
	int n;
	for(n = 0; n < vtp0_len(ids); n++) {
		long int *id = ids->array[n];
		*id = pcb_create_ID_get();
	}
	vtp0_uninit(ids);
}

static int parse_styles(lht_read_t *rctx, pcb_data_t *dt, vtroutestyle_t *styles, lht_node_t *nd)
{
	lht_node_t *stn;
	lht_dom_iterator_t it;
	int err = 0;

	if (nd->type != LHT_LIST)
		return iolht_error(nd, "route styles must be a list\n");

	for(stn = lht_dom_first(&it, nd); stn != NULL; stn = lht_dom_next(&it)) {
		pcb_route_style_t *s = vtroutestyle_alloc_append(styles, 1);
		int name_len = strlen(stn->name);

		s->fid = -1;

		if (stn->type != LHT_HASH)
			return iolht_error(stn, "route style entry must be a hash\n");
		
		/* safe copy the name */
		if (name_len > sizeof(s->name)-1) {
			iolht_warn(rctx, stn, -1, "Route style name too long: '%s' (should be less than %d characters); name will be truncated\n", stn->name, sizeof(s->name)-1);
			memcpy(s->name, stn->name, sizeof(s->name)-2);
			s->name[sizeof(s->name)-1] = '\0';
		}
		else
			memcpy(s->name, stn->name, name_len+1);

		s->via_proto = 0;
		s->via_proto_set = 0;
		err |= parse_coord(&s->Thick,     hash_get(stn, "thickness", 0));
		err |= parse_coord(&s->Clearance, hash_get(stn, "clearance", 0));

		parse_attributes(&s->attr, lht_dom_hash_get(stn, "attributes"));

		/* read text thickness */
		{
			lht_node_t *tt = lht_dom_hash_get(stn, "text_thick");

			s->textt = 0;
			if (tt != NULL) {
				err |= parse_coord(&s->textt,  tt);
				if (rctx->rdver < 6)
					iolht_warn(rctx, stn, -1, "text_thick in route style before v6 was not supported\n(accepting it for now, but older versions of pcb-rnd won't)\n");
			}
		}

		/* read text scale */
		{
			lht_node_t *ts = lht_dom_hash_get(stn, "text_scale");

			s->texts = 0;
			if (ts != NULL) {
				err |= parse_int(&s->texts, ts);
				if (rctx->rdver < 6)
					iolht_warn(rctx, stn, -1, "text_scale in route style before v6 was not supported\n(accepting it for now, but older versions of pcb-rnd won't)\n");
			}
		}

		if (rctx->rdver >= 5) {
			lht_node_t *vp = lht_dom_hash_get(stn, "via_proto");
			if (vp != NULL) {
				unsigned long pid;
				if (parse_ulong(&pid, vp) != 0)
					return iolht_error(stn, "Invalid route style prototype ID\n");
				s->via_proto = pid;
				s->via_proto_set = 1;
				if (pcb_pstk_get_proto_(dt, pid) == NULL)
					iolht_warn(rctx, vp, -1, "Route style %s references to non-existent prototype %ld\n", s->name, s->via_proto);

				if ((hash_get(stn, "diameter", 1) != &missing_ok) || (hash_get(stn, "hole", 1) != &missing_ok))
					iolht_warn(rctx, stn, -1, "v8+ route style with via padstack proto shall not have diameters in %s: diametres are ignored, proto is used\n", s->name);
			}
		}

		if (!s->via_proto_set) {
			rnd_coord_t drill_dia, pad_dia, mask = 0;
			int err = 0;
			lht_node_t *dn, *hn;

			dn = hash_get(stn, "diameter", 1);
			hn = hash_get(stn, "hole", 1);

			if ((dn != &missing_ok) || (hn != &missing_ok)) {
				if (rctx->rdver >= 8)
					iolht_warn(rctx, stn, -1, "v8+ route style should not have via diameters in %s: it should use via proto\n(Maybe it is a manually written file? Converting diameters to prototypes...)\n", s->name);

				/* import old, diameter based via geometry from old files */
				err |= parse_coord(&pad_dia,   dn);
				err |= parse_coord(&drill_dia, hn);

				if ((err == 0) && ((drill_dia <= 0) || (pad_dia <= 0) || (pad_dia <= drill_dia))) {
					iolht_error(stn, "Invalid route style prototype via diameters: copper or hole dia too small\n(Via ignored for routing style '%s')\n", s->name);
					s->via_proto = -1;
				}
				else if ((err == 0) && (drill_dia > 0) && (pad_dia > 0) && (pad_dia > drill_dia)) {
					if (pcb_compat_route_style_via_load(dt, s, drill_dia, pad_dia, mask) != 0)
						iolht_warn(rctx, stn, -1, "Invalid route style via diameters in %s: can not create padstack proto\n", s->name);
				}
				else
					return iolht_error(stn, "Invalid route style prototype via diameters: syntax error\n");
			}
		}

		if (rctx->rdver >= 8) {
			lht_node_t *fidn = lht_dom_hash_get(stn, "fid");
			if (fidn != NULL) {
				unsigned long fid;
				if (parse_ulong(&fid, fidn) != 0)
					return iolht_error(stn, "Invalid route style font ID\n");
				s->fid = fid;
			}
		}
	}
	return 0;
}

static int parse_netlist_input(lht_read_t *rctx, pcb_board_t *pcb, pcb_netlist_t *nl, lht_node_t *netlist)
{
	lht_node_t *nnet;
	if (netlist->type != LHT_LIST)
		return iolht_error(netlist, "netlist (parent) must be a list\n");

	for(nnet = netlist->data.list.first; nnet != NULL; nnet = nnet->next) {
		lht_node_t *nconn, *nstyle, *nt, *nattr;
		pcb_net_t *net;
		const char *style = NULL;

		if (nnet->type != LHT_HASH)
			return iolht_error(nnet, "netlist must be a hash\n");
		nconn  = lht_dom_hash_get(nnet, "conn");
		nstyle = lht_dom_hash_get(nnet, "style");
		nattr = lht_dom_hash_get(nnet, "attributes");

		if ((nconn != NULL) && (nconn->type != LHT_LIST))
			return iolht_error(nconn, "conn must be a list\n");

		if (nstyle != NULL) {
			if (nstyle->type != LHT_TEXT)
				return iolht_error(nstyle, "style must be text\n");
			style = nstyle->data.text.value;
		}

		net = pcb_net_get(pcb, nl, nnet->name, PCB_NETA_ALLOC);
		if (net == NULL)
			return iolht_error(nnet, "failed to add network\n");
		if (nconn != NULL) {
			for(nt = nconn->data.list.first; nt != NULL; nt = nt->next) {
				if ((nt->type != LHT_TEXT) || (*nt->data.text.value == '\0'))
					return iolht_error(nt, "terminal id must be a non-empty text\n");
				if (pcb_net_term_get_by_pinname(net, nt->data.text.value, PCB_NETA_ALLOC) == NULL)
					return iolht_error(nt, "failed to add terminal id to network\n");
			}
		}

		if (nattr != NULL) {
			if (rctx->rdver < 5)
				iolht_warn(rctx, nattr, 4, "Netlist could not have attributes before lihata v5 - still loading these attributes,\nbut they will be ignored by older versions of pcb-rnd.\n");
			if (parse_attributes(&net->Attributes, nattr) < 0)
				return iolht_error(nattr, "failed to load attributes\n");
		}
		if (style != NULL)
			pcb_attribute_put(&net->Attributes, "style", style);
	}
	return 0;
}

#define REQUIRE_NNET \
do { \
		if ((nnet == NULL) || (nnet->type != LHT_TEXT) || (*nnet->data.text.value == '\0')) \
			return iolht_error(nnet, "netlist patch net name must be a non-empty text\n"); \
} while(0)

static int parse_netlist_patch(lht_read_t *rctx, pcb_board_t *pcb, lht_node_t *patches)
{
	lht_node_t *np;

	if (patches->type != LHT_LIST)
		return iolht_error(patches, "netlist patches must be a list\n");

	for(np = patches->data.list.first; np != NULL; np = np->next) {
		lht_node_t *nnet, *nkey, *nval;
		if (np->type != LHT_HASH)
			return iolht_error(np, "each netlist patch must be a hash\n");
		nnet = lht_dom_hash_get(np, "net");

		if (strcmp(np->name, "del_conn") == 0) {
			REQUIRE_NNET;
			nval = lht_dom_hash_get(np, "term");
			if ((nval == NULL) || (nval->type != LHT_TEXT) || (*nval->data.text.value == '\0'))
				return iolht_error(nval, "netlist patch terminal ID must be a non-empty string (del_conn)\n");
			pcb_ratspatch_append(pcb, RATP_DEL_CONN, nval->data.text.value, nnet->data.text.value, NULL, 0);
		}
		else if (strcmp(np->name, "add_conn") == 0) {
			REQUIRE_NNET;
			nval = lht_dom_hash_get(np, "term");
			if ((nval == NULL) || (nval->type != LHT_TEXT) || (*nval->data.text.value == '\0'))
				return iolht_error(nval, "netlist patch terminal ID must be a non-empty string (add_conn)\n");
			pcb_ratspatch_append(pcb, RATP_ADD_CONN, nval->data.text.value, nnet->data.text.value, NULL, 0);
		}
		else if (strcmp(np->name, "change_comp_attrib") == 0) {
			if (rctx->rdver < 9)
				iolht_warn(rctx, np, -1, "Lihata board below v9 should not have change_comp_attrib in the netlist patch\n");
			nnet = lht_dom_hash_get(np, "comp");
			if ((nnet == NULL) || (nnet->type != LHT_TEXT) || (*nnet->data.text.value == '\0')) \
				return iolht_error(nnet, "netlist patch comp name must be a non-empty text for change_comp_attrib\n"); \
			goto change_comp_attrib;
		}
		else if (strcmp(np->name, "change_attrib") == 0)  {
			REQUIRE_NNET;
			change_comp_attrib:;
			nkey = lht_dom_hash_get(np, "key");
			if ((nkey == NULL) || (nkey->type != LHT_TEXT) || (*nkey->data.text.value == '\0'))
				return iolht_error(nkey, "netlist patch attrib key must be a non-empty string (change_attrib)\n");
			nval = lht_dom_hash_get(np, "val");
			if ((nval == NULL) || (nval->type != LHT_TEXT))
				return iolht_error(nval, "netlist patch attrib value must be a non-empty string (change_attrib)\n");
			pcb_ratspatch_append(pcb, RATP_CHANGE_COMP_ATTRIB, nnet->data.text.value, nkey->data.text.value, nval->data.text.value, 0);
		}
		else if ((strcmp(np->name, "change_net_attrib") == 0)) {
			if (rctx->rdver < 9)
				iolht_warn(rctx, np, -1, "Lihata board below v9 should not have change_net_attrib in the netlist patch\n");
			REQUIRE_NNET;

			nkey = lht_dom_hash_get(np, "key");
			if ((nkey == NULL) || (nkey->type != LHT_TEXT) || (*nkey->data.text.value == '\0'))
				return iolht_error(nkey, "netlist patch attrib key must be a non-empty string (change_attrib)\n");
			nval = lht_dom_hash_get(np, "val");
			if ((nval == NULL) || (nval->type != LHT_TEXT))
				return iolht_error(nval, "netlist patch attrib value must be a non-empty string (change_attrib)\n");
			pcb_ratspatch_append(pcb, RATP_CHANGE_NET_ATTRIB, nnet->data.text.value, nkey->data.text.value, nval->data.text.value, 0);
		}
		else if ((strcmp(np->name, "comp_add") == 0) || (strcmp(np->name, "comp_del") == 0)) {
			int add = np->name[5] == 'a';

			if (rctx->rdver < 9)
				iolht_warn(rctx, np, -1, "Lihata board below v9 should not have comp_add in the netlist patch\n");

			nnet = lht_dom_hash_get(np, "comp");
			if ((nnet == NULL) || (nnet->type != LHT_TEXT) || (*nnet->data.text.value == '\0'))
				return iolht_error(nnet, "netlist patch comp must be a non-empty text in %s\n", np->name);

			pcb_ratspatch_append(pcb, add ? RATP_COMP_ADD : RATP_COMP_DEL, nnet->data.text.value, NULL, NULL, 0);
		}

	}
	return 0;
}

static int parse_netlists(lht_read_t *rctx, pcb_board_t *pcb, lht_node_t *netlists)
{
	lht_node_t *sub;

	if (netlists->type != LHT_HASH)
		return iolht_error(netlists, "netlists must be a hash\n");

	sub = lht_dom_hash_get(netlists, "input");
	if ((sub != NULL) && (parse_netlist_input(rctx, pcb, pcb->netlist+PCB_NETLIST_INPUT, sub) != 0))
		return iolht_error(sub, "failed to parse the input netlist\n");

	sub = lht_dom_hash_get(netlists, "netlist_patch");
	if ((sub != NULL) && (parse_netlist_patch(rctx, pcb, sub) != 0))
		return iolht_error(sub, "failed to parse the netlist patch\n");

	return 0;
}

static void parse_conf(lht_read_t *rctx, pcb_board_t *pcb, lht_node_t *sub)
{
	if (rctx->cfg_dest == RND_CFR_invalid)
		return;
	if (rnd_conf_insert_tree_as(rctx->cfg_dest, sub) != 0)
		rnd_message(RND_MSG_ERROR, "Failed to insert the config subtree '%s' found in %s\n", sub->name, pcb->hidlib.loadname);
	else
		rnd_conf_update(NULL, -1);
}


static int parse_board(lht_read_t *rctx, pcb_board_t *pcb, lht_node_t *nd)
{
	lht_node_t *sub;
	pcb_plug_io_t *loader;

	rctx->warned = 0;
	rctx->old_model_warned = 0;
	rctx->rdver = atoi(nd->name+15);
	switch(rctx->rdver) {
		case 1: loader = &plug_io_lihata_v1; break;
		case 2: loader = &plug_io_lihata_v2; break;
		case 3: loader = &plug_io_lihata_v3; break;
		case 4: loader = &plug_io_lihata_v4; break;
		case 5: loader = &plug_io_lihata_v5; break;
		case 6: loader = &plug_io_lihata_v6; break;
		case 7: loader = &plug_io_lihata_v7; break;
		case 8: loader = &plug_io_lihata_v8; break;
		case 9: loader = &plug_io_lihata_v9; break;
		default:
			return iolht_error(nd, "Lihata board version %d not supported;\n"
				"must be 1, 2, 3, 4, 5, 6, 7, 8 or 9.\n", rctx->rdver);
	}

	vtp0_init(&rctx->post_ids);
	vtp0_init(&rctx->post_thermal_old);
	vtp0_init(&rctx->post_thermal_heavy);
	pxm_init(rctx, lht_dom_hash_get(nd, "pixmaps"));

	pcb_rat_all_anchor_guess(pcb->Data);

	memset(&pcb->LayerGroups, 0, sizeof(pcb->LayerGroups));

	if (parse_attributes(&pcb->Attributes, lht_dom_hash_get(nd, "attributes")) != 0)
		goto error;

	sub = lht_dom_hash_get(nd, "meta");
	if ((sub != NULL) && (parse_meta(rctx, pcb, sub) != 0))
		goto error;

	sub = lht_dom_hash_get(nd, "font");
	if ((sub != NULL) && (parse_fontkit(&PCB->fontkit, sub) != 0))
		goto error;
	PCB->fontkit.valid = 1;

	if (rctx->rdver >= 2) {
		sub = lht_dom_hash_get(nd, "layer_stack");
		if (sub != NULL) {
			if (parse_layer_stack(rctx, pcb, sub) != 0)
				goto error;
		}
		else if (validate_layer_stack_grp(rctx, pcb, nd) != 0)
			goto error;
	}

	pcb_data_clip_inhibit_inc(pcb->Data);

	sub = lht_dom_hash_get(nd, "data");
	if (sub == NULL) {
		pcb_data_clip_inhibit_dec(pcb->Data, rnd_true);
		return iolht_error(nd, "Lihata board without data\n");
	}
	if (parse_data(rctx, pcb, pcb->Data, sub, NULL) == NULL) {
		pcb_data_clip_inhibit_dec(pcb->Data, rnd_true);
		goto error;
	}

	if (validate_layer_stack_lyr(rctx, pcb, sub) != 0)
		goto error;

	sub = lht_dom_hash_get(nd, "styles");
	if ((sub != NULL) && (parse_styles(rctx, pcb->Data, &pcb->RouteStyle, sub) != 0)) {
		pcb_data_clip_inhibit_dec(pcb->Data, rnd_true);
		goto error;
	}

	sub = lht_dom_hash_get(nd, "netlists");
	if ((sub != NULL) && (parse_netlists(rctx, pcb, sub) != 0)) {
		pcb_data_clip_inhibit_dec(pcb->Data, rnd_true);
		goto error;
	}

	post_ids_assign(&rctx->post_ids);
	if (post_thermal_assign(rctx, pcb, &rctx->post_thermal_old, &rctx->post_thermal_heavy) != 0) {
		pcb_data_clip_inhibit_dec(pcb->Data, rnd_true);
		goto error;
	}

	/* Run poly clipping at the end so we have all IDs and we can
	   announce the clipping (it's slow, we may need a progress bar) */
	{
		rnd_rtree_it_t it;
		rnd_box_t *b;
		int l;
		for(l = 0; l < pcb->Data->LayerN; l++) {
			pcb_layer_t *layer = pcb->Data->Layer + l;
			if (layer->polygon_tree != NULL) {
				for(b = rnd_rtree_all_first(&it, layer->polygon_tree); b != NULL; b = rnd_rtree_all_next(&it)) {
					pcb_poly_t *p = (pcb_poly_t *)b;
					pcb_poly_init_clip(pcb->Data, layer, p);
				}
			}
		}
	}

	sub = lht_dom_hash_get(nd, "pcb-rnd-conf-v1");
	if (sub != NULL)
		parse_conf(rctx, pcb, sub);

	if (rctx->rdver < 5) {
		/* have to parse meta again to get config values overwritten */
		sub = lht_dom_hash_get(nd, "meta");
		if (sub != NULL)
			parse_meta(rctx, pcb, sub);
	}

	pcb_data_clip_inhibit_dec(pcb->Data, rnd_true);

	pcb->Data->loader = loader; /* set this manually so the version is remembered */
	pxm_uninit();
	return 0;

	error:
	pxm_uninit();
	return -1;
}

int io_lihata_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, rnd_conf_role_t settings_dest)
{
	int res;
	char *errmsg = NULL, *realfn;
	lht_doc_t *doc = NULL;
	lht_read_t rctx = {0};

	rctx.cfg_dest = settings_dest;

	realfn = rnd_fopen_check(NULL, Filename, "r");
	if (realfn != NULL)
		doc = lht_dom_load(realfn, &errmsg);
	free(realfn);

	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "Error loading '%s': %s\n", Filename, errmsg);
		free(errmsg);
		return -1;
	}

	if ((doc->root->type == LHT_HASH) && (strncmp(doc->root->name, "pcb-rnd-board-v", 15) == 0)) {
		res = parse_board(&rctx, Ptr, doc->root);
	}
	else if ((doc->root->type == LHT_LIST) && (strncmp(doc->root->name, "pcb-rnd-subcircuit-v", 20) == 0)) {
		pcb_subc_t *sc;

		rctx.warned = 0;
		rctx.old_model_warned = 0;
		rctx.rdver = atoi(doc->root->name+20);
		Ptr->is_footprint = 1;
		res = parse_subc(&rctx, NULL, Ptr->Data, doc->root->data.list.first, &sc);

		if (res == 0) {
			pcb_layergrp_upgrade_to_pstk(Ptr);
			pcb_layer_create_all_for_recipe(Ptr, sc->data->Layer, sc->data->LayerN);
			pcb_subc_rebind(Ptr, sc);
			pcb_data_clip_polys(sc->data);
		}
	}
	else if ((doc->root->type == LHT_LIST) && (strncmp(doc->root->name, "pcb-rnd-font-v", 14) == 0)) {
		res = 0;
		if (rnd_actionva(&Ptr->hidlib, "loadfontfrom", Filename, "1", NULL) != 0) {
			rnd_message(RND_MSG_ERROR, "io_lihata: failed to load font file '%s' for editing\n", Filename);
			res = -1;
		}
		else {
			rnd_conf_set(RND_CFR_DESIGN, "design/text_font_id", 0, "1", RND_POL_OVERWRITE);
			if (rnd_actionva(&Ptr->hidlib, "fontedit", "inplace", NULL) != 0) {
				rnd_message(RND_MSG_ERROR, "io_lihata: failed to start the font editor\n");
				res = -1;
			}
		}

	}
	else {
		iolht_error(doc->root, "Error loading '%s': neither a board nor a subcircuit\n", Filename);
		res = -1;
	}

	lht_dom_uninit(doc);
	free(errmsg);
	return res;
}

int io_lihata_parse_buffer(pcb_plug_io_t *ctx, pcb_buffer_t *buff, const char *filename)
{
	int res;
	char *errmsg = NULL, *realfn;
	lht_doc_t *doc = NULL;
	lht_read_t rctx = {0};

	rctx.cfg_dest = RND_CFR_invalid;

	realfn = rnd_fopen_check(NULL, filename, "r");
	if (realfn != NULL)
		doc = lht_dom_load(realfn, &errmsg);
	free(realfn);

	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "Error loading '%s': %s\n", filename, errmsg);
		free(errmsg);
		return -1;
	}

	if ((doc->root->type == LHT_HASH) && (strncmp(doc->root->name, "pcb-rnd-buffer-v", 16) == 0)) {
		lht_node_t *datand = lht_dom_hash_get(doc->root, "data");
		rctx.rdver = atoi(doc->root->name+16);
		if (datand == NULL) {
			iolht_error(doc->root, "Error loading '%s': buffer has no data\n", filename);
			res = -1;
		}
		else
			res = (parse_data(&rctx, NULL, buff->Data, datand, &DUMMY_BUFFER_SUBC) == NULL);

		if (res == 0) {
			lht_node_t *ndx = lht_dom_hash_get(doc->root, "x"), *ndy = lht_dom_hash_get(doc->root, "y");
			parse_coord(&buff->X, ndx);
			parse_coord(&buff->Y, ndy);
		}
	}
	else {
		iolht_error(doc->root, "Error loading '%s': not a pcb-rnd paste buffer\n", filename);
		res = -1;
	}

	lht_dom_uninit(doc);
	free(errmsg);
	return res;
}

int io_lihata_parse_padstack(pcb_plug_io_t *ctx, pcb_pstk_proto_t *proto, const char *filename)
{
	int res;
	char *errmsg = NULL, *realfn;
	lht_doc_t *doc = NULL;
	lht_read_t rctx = {0};

	rctx.cfg_dest = RND_CFR_invalid;

	realfn = rnd_fopen_check(NULL, filename, "r");
	if (realfn != NULL)
		doc = lht_dom_load(realfn, &errmsg);
	free(realfn);

	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "Error loading '%s': %s\n", filename, errmsg);
		free(errmsg);
		return -1;
	}

	if ((doc->root->type == LHT_HASH) && (strncmp(doc->root->name, "pcb-rnd-padstack-v", 18) == 0)) {
		lht_node_t *datand = lht_dom_hash_get(doc->root, "ps_proto_v6.0");
		rctx.rdver = atoi(doc->root->name+18);
		if (datand == NULL) {
			iolht_error(doc->root, "Error loading '%s': padstack has no ps_proto_v6\n", filename);
			res = -1;
		}
		else
			res = parse_data_pstk_proto(&rctx, NULL, proto, datand, NULL, 6);
	}
	else {
		iolht_error(doc->root, "Error loading '%s': not a pcb-rnd paste buffer\n", filename);
		res = -1;
	}

	lht_dom_uninit(doc);
	free(errmsg);
	return res;
}


typedef enum {
	TPS_UNDECIDED,
	TPS_GOOD,
	TPS_BAD
} test_parse_t;

/* expect root to be a ha:pcb-rnd-board-v* */
void test_parse_ev(lht_parse_t *ctx, lht_event_t ev, lht_node_type_t nt, const char *name, const char *value)
{
	test_parse_t *state = ctx->user_data;
	if (ev == LHT_OPEN) {
		if ((nt == LHT_HASH) && (strncmp(name, "pcb-rnd-board-v", 15) == 0))
			*state = TPS_GOOD;
		else if ((nt == LHT_HASH) && (strncmp(name, "pcb-rnd-buffer-v", 16) == 0))
			*state = TPS_GOOD;
		else if ((nt == LHT_LIST) && (strncmp(name, "pcb-rnd-subcircuit-v", 20) == 0))
			*state = TPS_GOOD;
		else if ((nt == LHT_LIST) && (strncmp(name, "pcb-rnd-font-v", 14) == 0))
			*state = TPS_GOOD;
		else if ((nt == LHT_HASH) && (strncmp(name, "pcb-rnd-padstack-v", 18) == 0))
			*state = TPS_GOOD;
		else
			*state = TPS_BAD;
	}
}


/* run an event parser for the first 32k of the file; accept the file if it
   has a valid looking root; refuse if:
    - no root in the first 32k (or till eof)
    - not a valid lihata doc (parser error)
    - lihata, but the wrong root
*/
int io_lihata_test_parse(pcb_plug_io_t *plug_ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	lht_parse_t ctx;
	int count;
	test_parse_t state = TPS_UNDECIDED;

	lht_parser_init(&ctx);
	ctx.event = test_parse_ev;
	ctx.user_data = &state;

	for(count = 0; count < 32768; count++) {
		int c = fgetc(f);
		if (lht_parser_char(&ctx, c) != LHTE_SUCCESS) {
			/* parse error or end */
			state = TPS_BAD;
			break;
		}
		if (state != TPS_UNDECIDED)
			break;
	}
	lht_parser_uninit(&ctx);
	return (state == TPS_GOOD);
}

int io_lihata_parse_font(pcb_plug_io_t *ctx, rnd_font_t *Ptr, const char *Filename)
{
	int res;
	char *errmsg = NULL, *realfn;
	lht_doc_t *doc = NULL;

	realfn = rnd_fopen_check(&PCB->hidlib, Filename, "r");
	if (realfn != NULL)
		doc = lht_dom_load(realfn, &errmsg);
	free(realfn);

	if (doc == NULL) {
		if (!pcb_io_err_inhibit)
			rnd_message(RND_MSG_ERROR, "Error loading '%s': %s\n", Filename, errmsg);
		free(errmsg);
		return -1;
	}

	if ((doc->root->type != LHT_LIST) || (strcmp(doc->root->name, "pcb-rnd-font-v1"))) {
		if (!pcb_io_err_inhibit)
			rnd_message(RND_MSG_ERROR, "Not a font lihata.\n");
		res = -1;
	}
	else {
		TODO("font: remove this whole function and call rnd_font_load(Ptr, Filanem, pcb_io_err_inhibit) instead");
		res = rnd_font_lht_parse_font(Ptr, doc->root->data.list.first);
	}

	free(errmsg);
	lht_dom_uninit(doc);
	return res;
}


int io_lihata_parse_subc(pcb_plug_io_t *ctx, pcb_data_t *Ptr, const char *name, const char *subfpname)
{
	int res;
	char *errmsg = NULL;
	lht_doc_t *doc = NULL;
	lht_node_t *n;
	pcb_fp_fopen_ctx_t st;
	FILE *f;
	pcb_subc_t *sc;
	lht_read_t rctx = {0};

	rctx.cfg_dest = RND_CFR_invalid;

	f = pcb_fp_fopen(&conf_core.rc.library_search_paths, name, &st, NULL);

	if (f != NULL)
		doc = lht_dom_load_stream(f, st.filename, &errmsg);
	pcb_fp_fclose(f, &st);

	if (doc == NULL) {
		if (!pcb_io_err_inhibit)
			rnd_message(RND_MSG_ERROR, "Error loading '%s': %s\n", name, errmsg);
		free(errmsg);
		return -1;
	}

	if ((doc->root->type != LHT_LIST) || (strncmp(doc->root->name, "pcb-rnd-subcircuit-v", 20))) {
		if (!pcb_io_err_inhibit)
			rnd_message(RND_MSG_ERROR, "Not a subcircuit lihata.\n");
		free(errmsg);
		lht_dom_uninit(doc);
		return -1;
	}

	rctx.warned = 0;
	rctx.old_model_warned = 0;
	rctx.rdver = atoi(doc->root->name+20);
	if (rctx.rdver < 3) {
		if (!pcb_io_err_inhibit)
			rnd_message(RND_MSG_ERROR, "io_lihata: invalid subc file version: %s (expected 3 or higher)\n", doc->root->name+20);
		free(errmsg);
		lht_dom_uninit(doc);
		return -1;
	}

	for(n = doc->root->data.list.first; n != NULL; n = n->next) {
		if (strcmp(n->name, "pixmaps") == 0) {
			if (n->type == LHT_HASH) {
				pxm_init(&rctx, n);
				break;
			}
			else
				rnd_message(RND_MSG_ERROR, "io_lihata: invalid pixmaps subtree: needs to be a hash\n");
		}
	}

	res = parse_subc(&rctx, NULL, Ptr, doc->root->data.list.first, &sc);
	pxm_uninit();
	if (res == 0)
		pcb_data_clip_polys(sc->data);

	lht_dom_uninit(doc);
	free(errmsg);
	return res;
}

/* Decide about the type of a footprint file:
   - if root is li:pcb-rnd-subcircuit, it is a static footprint file
   - else it is a parametric element (footprint generator) if it contains
     "@@" "purpose"
   - if a line of a file element starts with ## and doesn't contain @, it's a tag
   - if need_tags is set, tags are saved
*/
pcb_plug_fp_map_t *io_lihata_map_footprint(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags)
{
	int c, comment_len;
	int first_subc = 1;
	long pos = 0;
	enum {
		ST_WS,
		ST_COMMENT,
		ST_TAG
	} state = ST_WS;
	gds_t tag;

	gds_init(&tag);
	while ((c = fgetc(f)) != EOF) {
		pos++;
		if (pos > 1024) break; /* header must be in the first kilobyte */
		switch (state) {
		case ST_WS:
			if (isspace(c))
				break;
			if (c == '#') {
				comment_len = 0;
				state = ST_COMMENT;
				break;
			}
			else if ((first_subc) && (c == 'l')) {
				char s[23];
				/* li:pcb-rnd-subcircuit */
				fgets(s, 21, f);
				s[20] = '\0';
				if (strcmp(s, "i:pcb-rnd-subcircuit") == 0) {
					head->type = PCB_FP_FILE;
					goto out;
				}
			}
			first_subc = 0;
			/* fall-thru for detecting @ */
		case ST_COMMENT:
			comment_len++;
			if ((c == '!') && (pos == 2)) {
				/* must be a shebang -> parametric footprint */
				head->type = PCB_FP_PARAMETRIC;
				goto out;
			}

			if ((c == '#') && (comment_len == 1)) {
				state = ST_TAG;
				break;
			}
			if ((c == '\r') || (c == '\n'))
				state = ST_WS;
			if (c == '@') {
				char s[10];
			maybe_purpose:;
				/* "@@" "purpose" */
				fgets(s, 9, f);
				s[8] = '\0';
				if (strcmp(s, "@purpose") == 0) {
					head->type = PCB_FP_PARAMETRIC;
					goto out;
				}
			}
			break;
		case ST_TAG:
			if ((c == '\r') || (c == '\n')) {	/* end of a tag */
				if (need_tags && (tag.used != 0))
					vts0_append(&head->tags, (char *)pcb_fp_tag(tag.array, 1));

				tag.used = 0;
				state = ST_WS;
				break;
			}
			if (c == '@')
				goto maybe_purpose;
			gds_append(&tag, c);
		}
	}

out:;
	gds_uninit(&tag);
	return head;
}
