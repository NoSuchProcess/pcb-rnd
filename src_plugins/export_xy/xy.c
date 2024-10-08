#include "config.h"
#include "conf_core.h"
#include <librnd/core/rnd_conf.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <genvector/vts0.h>

#include <librnd/core/math_helper.h>
#include "build_run.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/rotate.h>
#include "obj_pstk_inlines.h"
#include "obj_subc_op.h"
#include "layer.h"
#include "netlist.h"
#include <librnd/core/safe_fs.h>
#include "operation.h"
#include "xy_conf.h"

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>
#include <librnd/hid/hid_attrib.h>
#include "hid_cam.h"
#include <librnd/hid/hid_init.h>

#include "../src_plugins/lib_compat_help/elem_rot.h"
#include "../src_plugins/export_xy/conf_internal.c"

#define CONF_FN "export_xy.conf"

conf_xy_t conf_xy;

const char *xy_cookie = "XY HID";

/* Maximum length of a template name (in the config file, in the enum) */
#define MAX_TEMP_NAME_LEN 128


/* This one can not be const because format enumeration is loaded run-time */
static rnd_export_opt_t xy_options[] = {
/* %start-doc options "8 XY Creation"
@ftable @code
@item --xyfile <string>
Name of the XY output file.
@end ftable
%end-doc
*/
	{"xyfile", "Name of the XY output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_xyfile 0

/* %start-doc options "8 XY Creation"
@ftable @code
@item --xy-unit <unit>
Unit of XY dimensions. Defaults to mil.
@end ftable
%end-doc
*/
	{"xy-unit", "XY units",
	 RND_HATT_UNIT, 0, 0, {-1, 0, 0}, NULL},
#define HA_unit 1
	{"format", "file format (template)",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, NULL},
#define HA_format 2
	{"vendor", "vendor specific ID used for xy::VENDOR::rotate and xy::VENDOR::translate",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, NULL},
#define HA_vendor 3

	{"normalize-angles", "make sure angles are between 0 and 360",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, NULL},
#define HA_normalize_angles 4

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_cam 5

};

#define NUM_OPTIONS (sizeof(xy_options)/sizeof(xy_options[0]))

static rnd_hid_attr_val_t xy_values[NUM_OPTIONS];

static char *pcb_bom_clean_str(const char *in)
{
	char *out;
	int i;

	if ((out = malloc((strlen(in) + 1) * sizeof(char))) == NULL) {
		fprintf(stderr, "Error:  pcb_bom_clean_str() malloc() failed\n");
		exit(1);
	}

	/* copy over in to out with some character conversions.
	   Go all the way to then end to get the terminating \0 */
	for (i = 0; i <= strlen(in); i++) {
		switch (in[i]) {
		case '"':
			out[i] = '\'';
			break;
		default:
			out[i] = in[i];
		}
	}

	return out;
}

static const char *xy_filename;
static const rnd_unit_t *xy_unit;
vts0_t fmt_names; /* array of const char * long name of each format, pointing into the conf database */
vts0_t fmt_ids;   /* array of strdup'd short name (ID) of each format */

static void free_fmts(void)
{
	int n;
	for(n = 0; n < fmt_ids.used; n++) {
		free(fmt_ids.array[n]);
		fmt_ids.array[n] = NULL;
	}
}

static const rnd_export_opt_t *xy_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	static int last_unit_value = -1;
	rnd_conf_listitem_t *li;
	const char *val = xy_values[HA_xyfile].str;
	int idx;

	/* load all formats from the config */
	fmt_names.used = 0;
	fmt_ids.used = 0;

	free_fmts();
	rnd_conf_loop_list(&conf_xy.plugins.export_xy.templates, li, idx) {
		char id[MAX_TEMP_NAME_LEN];
		const char *sep = strchr(li->name, '.');
		int len;

		if (sep == NULL) {
			rnd_message(RND_MSG_ERROR, "export_xy: ignoring invalid template name (missing period): '%s'\n", li->name);
			continue;
		}
		if (strcmp(sep+1, "name") != 0)
			continue;
		len = sep - li->name;
		if (len > sizeof(id)-1) {
			rnd_message(RND_MSG_ERROR, "export_xy: ignoring invalid template name (too long): '%s'\n", li->name);
			continue;
		}
		memcpy(id, li->name, len);
		id[len] = '\0';
		vts0_append(&fmt_names, (char *)li->payload);
		vts0_append(&fmt_ids, rnd_strdup(id));
	}

	if (fmt_names.used == 0) {
		rnd_message(RND_MSG_ERROR, "export_xy: can not set up export options: no template available\n");
		return NULL;
	}

	xy_options[HA_format].enumerations = (const char **)fmt_names.array;

	/* set default unit and filename */
	if (xy_values[HA_unit].lng == last_unit_value) {
		if (rnd_conf.editor.grid_unit)
			xy_values[HA_unit].lng = rnd_conf.editor.grid_unit->index;
		else
			xy_values[HA_unit].lng = rnd_get_unit_struct("mil")->index;
		last_unit_value = xy_values[HA_unit].lng;
	}

	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &xy_values[HA_xyfile], ".xy");

	if (n)
		*n = NUM_OPTIONS;
	return xy_options;
}

/* ---------------------------------------------------------------------------
 * prints a centroid file in a format which includes data needed by a
 * pick and place machine.  Further formatting for a particular factory setup
 * can easily be generated with awk or perl.
 * returns != zero on error
 */

typedef struct {
	char utcTime[64];
	char *name, *descr, *value;
	const char *pad_netname;
	rnd_coord_t x, y, bottom_x, bottom_y;
	rnd_coord_t tx, ty;   /* translate x and y (xy::translate attr) */
	double theta;
	pcb_subc_t *subc;
	rnd_coord_t pad_cx, pad_cy, bottom_pad_cx, bottom_pad_cy;
	rnd_coord_t pad_w, pad_h;
	rnd_coord_t prpad_cx, prpad_cy, bottom_prpad_cx, bottom_prpad_cy;
	rnd_coord_t prpad_w, prpad_h;
	rnd_cardinal_t count;
	rnd_coord_t ox, oy, bottom_ox, bottom_oy;
	int origin_score;
	rnd_bool front;
	gds_t tmp;
} subst_ctx_t;

/* Find the pick and place 0;0 mark, if there is any */
static void find_origin_(const char *format_name, const char *prefix, rnd_coord_t *ox, rnd_coord_t *oy)
{
	char tmp[128], tmp2[128];
	pcb_data_it_t it;
	pcb_any_obj_t *obj;
	int origin_score = 0;

	rnd_snprintf(tmp, sizeof(tmp), "%spnp-origin-%s", prefix, format_name);
	rnd_snprintf(tmp2, sizeof(tmp), "%spnp-origin", prefix);

	for(obj = pcb_data_first(&it, PCB->Data, PCB_OBJ_CLASS_REAL); obj != NULL; obj = pcb_data_next(&it)) {
		int score;

		if (pcb_attribute_get(&obj->Attributes, tmp) != NULL)
			score = 2; /* first look for the format-specific attribute */
		else if (pcb_attribute_get(&obj->Attributes, tmp2) != NULL)
			score = 1; /* then for the generic pnp-specific attribute */
		else
			continue;

		if (score > origin_score) {
			origin_score = score;
			pcb_obj_center(obj, ox, oy);
		}
	}
}

/* Find the pick and place 0;0 mark, if there is any */
static void find_origin(subst_ctx_t *ctx, const char *format_name)
{
	/* default: bottom left of the drawing area */
	ctx->ox = 0;
	ctx->oy = PCB->hidlib.dwg.Y2;
	ctx->bottom_ox = PCB->hidlib.dwg.X2;
	ctx->bottom_oy = PCB->hidlib.dwg.Y2;

	find_origin_(format_name, "", &ctx->ox, &ctx->oy);
	find_origin_(format_name, "bottom-", &ctx->bottom_ox, &ctx->bottom_oy);
}


/* Calculate the bounding box of the subc as the bounding box of all terminals
   ("pads", bot heavy and light) */
static void calc_pad_bbox_(subst_ctx_t *ctx, rnd_coord_t *pw, rnd_coord_t *ph, rnd_coord_t *pcx, rnd_coord_t *pcy)
{
	rnd_box_t box;
	box.X1 = box.Y1 = RND_MAX_COORD;
	box.X2 = box.Y2 = -RND_MAX_COORD;

	if (ctx->subc != NULL) {
		pcb_any_obj_t *o;
		pcb_data_it_t it;

		for(o = pcb_data_first(&it, ctx->subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
			if (o->term != NULL) {
				if ((o->type != PCB_OBJ_PSTK) && (o->type != PCB_OBJ_SUBC)) { /* layer objects */
					pcb_layer_t *ly = o->parent.layer;
					assert(o->parent_type == PCB_PARENT_LAYER);
					if (!(pcb_layer_flags_(ly) & PCB_LYT_COPPER))
						continue;
				}
				rnd_box_bump_box(&box, &o->bbox_naked);
			}
		}
	}

	*pw = box.X2 - box.X1;
	*ph = box.Y2 - box.Y1;
	*pcx = (box.X2 + box.X1)/2;
	*pcy = (box.Y2 + box.Y1)/2;
}

static void count_pins_pads(subst_ctx_t *ctx, int *pins, int *pads)
{
	*pins = *pads = 0;

	if (ctx->subc != NULL) {
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		for(o = pcb_data_first(&it, ctx->subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
			pcb_layer_type_t lyt;

			if (o->term == NULL)
				continue;

			/* light terminals */
			if (o->type == PCB_OBJ_PSTK) {
				pcb_pstk_t *ps = (pcb_pstk_t *)o;
				pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);

				if (proto->hdia > 0) { /* through-hole */
					(*pins)++;
					continue;
				}

				/* smd */
				if (pcb_pstk_shape(ps, PCB_LYT_TOP | PCB_LYT_COPPER, 0))
					(*pads)++;
				if (pcb_pstk_shape(ps, PCB_LYT_BOTTOM | PCB_LYT_COPPER, 0))
					(*pads)++;
				continue;
			}
		
			/* heavy terminal */
			if (o->type == PCB_OBJ_SUBC) {
TODO("subc: subc-in-subc")
				assert(!"no subc-in-subc support yet");
				continue;
			}
			assert(o->parent_type == PCB_PARENT_LAYER);
			lyt = pcb_layer_flags_(o->parent.layer);
			if ((lyt & PCB_LYT_COPPER) && (lyt & (PCB_LYT_TOP || PCB_LYT_BOTTOM)))
				(*pads)++;
		}
	}
}

/* Calculate the bounding box of the subc as the bounding box of all terminals */
static void calc_pad_bbox(subst_ctx_t *ctx, int prerot, rnd_coord_t *pw, rnd_coord_t *ph, rnd_coord_t *pcx, rnd_coord_t *pcy)
{
	if (prerot) {
		pcb_subc_t *save, *sc_rot0;
		pcb_data_t *tmp = pcb_buffers[PCB_MAX_BUFFER-1].Data;
		double ang;
		pcb_opctx_t op;
		rnd_coord_t scox, scoy;

		/* Because of subc rotation terms are rotated too and non-round pads will
		   have outer corners that will bump the bbox more. What the fab wants
		   is "how much the original, 0 degree part needs to be rotated". This
		   is achieved by rotating the subc back to 0 deg position, calculate
		   the pad center and then rotate the result back to the rotated subc.
		   So pad center and width/height derived from pre-rotation state */

		/* optimization: for n*90 it's enough to swap width/heigth; pcx;pcy
		   happens to be correct as terminal bboxes are not rotated */
		if ((ctx->theta == 0) || (ctx->theta == 180) || (ctx->theta == -180)) {
			calc_pad_bbox_(ctx, pw, ph, pcx, pcy);
			return;
		}
		if ((ctx->theta == 90) || (ctx->theta == 270) || (ctx->theta == -90) || (ctx->theta == -270)) {
			calc_pad_bbox_(ctx, ph, pw, pcx, pcy);
			return;
		}

		/* generic case */

		/* acquire unrotated/untransformed origin of the input subc */
		pcb_subc_get_origin(ctx->subc, &scox, &scoy);

		/* create sc_rot0 in buffer: subc rotated back to 0 deg; ctx->theta is -subc_rot */
		sc_rot0 = pcb_subc_dup_at(NULL, tmp, ctx->subc, 0, 0, 0, rnd_false);
		ang = ctx->theta / RND_RAD_TO_DEG;
		pcb_subc_rotate(sc_rot0, scox, scoy, cos(ang), sin(ang), ctx->theta);

		/* compute pad bbox on the rotated-back subc in buffer */
		save = ctx->subc;
		ctx->subc = sc_rot0;
		calc_pad_bbox_(ctx, pw, ph, pcx, pcy);
		ctx->subc = save;

		/* rotate the result so it goes back to the rotation of the live subc */
		rnd_rotate(pcx, pcy, scox, scoy, cos(-ang), sin(-ang));

		op.remove.pcb = PCB;
		op.remove.destroy_target = tmp;
		pcb_subcop_destroy(&op, sc_rot0);
		return;
	}

	calc_pad_bbox_(ctx, pw, ph, pcx, pcy);
}

static void append_clean(gds_t *dst, const char *text)
{
	const char *s;
	for(s = text; *s != '\0'; s++)
		if (isalnum(*s) || (*s == '.') || (*s == '-') || (*s == '+'))
			gds_append(dst, *s);
		else
			gds_append(dst, '_');
}

static int is_val_true(const char *val)
{
	if (val == NULL) return 0;
	if (strcmp(val, "yes") == 0) return 1;
	if (strcmp(val, "on") == 0) return 1;
	if (strcmp(val, "true") == 0) return 1;
	if (strcmp(val, "1") == 0) return 1;
	return 0;
}

static double xy_normalize_angle(double a)
{
	if (xy_values[HA_normalize_angles].lng && ((a < 0.0) || (a > 360.0))) {
		a = fmod(a, 360.0);
		if (a < 0)
			a += 360;
	}

	if (a == -0.0)
		a = +0.0;

	return a;
}

static int subst_cb(void *ctx_, gds_t *s, const char **input)
{
	int pin_cnt = 0;
	int pad_cnt = 0;
	subst_ctx_t *ctx = ctx_;
	if (strncmp(*input, "UTC%", 4) == 0) {
		*input += 4;
		gds_append_str(s, ctx->utcTime);
		return 0;
	}
	if (strncmp(*input, "author%", 7) == 0) {
		*input += 7;
		gds_append_str(s, pcb_author());
		return 0;
	}
	if (strncmp(*input, "title%", 6) == 0) {
		*input += 6;
		gds_append_str(s, RND_UNKNOWN(PCB->hidlib.name));
		return 0;
	}
	if (strncmp(*input, "suffix%", 7) == 0) {
		*input += 7;
		gds_append_str(s, xy_unit->suffix);
		return 0;
	}
	if (strncmp(*input, "boardw%", 7) == 0) {
		*input += 7;
		rnd_append_printf(s, "%m+%mN", xy_unit->allow, PCB->hidlib.dwg.X2);
		return 0;
	}
	if (strncmp(*input, "boardh%", 7) == 0) {
		*input += 7;
		rnd_append_printf(s, "%m+%mN", xy_unit->allow, PCB->hidlib.dwg.Y2);
		return 0;
	}
	if (strncmp(*input, "subc.", 5) == 0) {
		*input += 5;

		/* elem attribute print:
		    subc.a.attribute            - print the attribute if exists, "n/a" if not
		    subc.a.attribute|unk        - print the attribute if exists, unk if not
		    subc.a.attribute?yes        - print yes if attribute is true, "n/a" if not
		    subc.a.attribute?yes:nope   - print yes if attribute is true, nope if not
		*/
		if (strncmp(*input, "a.", 2) == 0) {
			char aname[256], unk_buf[256], *nope;
			const char *val, *end, *unk = "n/a";
			long len;
			
			*input += 2;
			end = strpbrk(*input, "?|%");
			len = end - *input;
			if (len >= sizeof(aname) - 1) {
				rnd_message(RND_MSG_ERROR, "xy tempalte error: attribute name '%s' too long\n", *input);
				return 1;
			}
			memcpy(aname, *input, len);
			aname[len] = '\0';
			if (*end == '|') { /* "or unknown" */
				*input = end+1;
				end = strchr(*input, '%');
				len = end - *input;
				if (len >= sizeof(unk_buf) - 1) {
					rnd_message(RND_MSG_ERROR, "xy tempalte error: elem atribute '|unknown' field '%s' too long\n", *input);
					return 1;
				}
				memcpy(unk_buf, *input, len);
				unk_buf[len] = '\0';
				unk = unk_buf;
				*input = end;
			}
			else if (*end == '?') { /* trenary */
				*input = end+1;
				end = strchr(*input, '%');
				len = end - *input;
				if (len >= sizeof(unk_buf) - 1) {
					rnd_message(RND_MSG_ERROR, "xy tempalte error: elem atribute trenary field '%s' too long\n", *input);
					return 1;
				}

				memcpy(unk_buf, *input, len);
				unk_buf[len] = '\0';
				*input = end+1;

				nope = strchr(unk_buf, ':');
				if (nope != NULL) {
					*nope = '\0';
					nope++;
				}
				else /* only '?' is given, no ':' */
					nope = "n/a";

				val = pcb_attribute_get(&ctx->subc->Attributes, aname);
				if (is_val_true(val))
					gds_append_str(s, unk_buf);
				else
					gds_append_str(s, nope);

				return 0;
			}
			else
				*input = end;
			(*input)++;

			val = pcb_attribute_get(&ctx->subc->Attributes, aname);
			if (val == NULL)
				val = unk;
			gds_append_str(s, val);
			return 0;
		}
		if (strncmp(*input, "refdes%", 7) == 0) {
			*input += 7;
			gds_append_str(s, ctx->name);
			return 0;
		}
		if (strncmp(*input, "refdes_%", 8) == 0) {
			*input += 8;
			append_clean(s, ctx->name);
			return 0;
		}
		if (strncmp(*input, "footprint%", 10) == 0) {
			*input += 10;
			gds_append_str(s, ctx->descr);
			return 0;
		}
		if (strncmp(*input, "footprint_%", 11) == 0) {
			*input += 11;
			append_clean(s, ctx->descr);
			return 0;
		}
		if (strncmp(*input, "value%", 6) == 0) {
			*input += 6;
			gds_append_str(s, ctx->value);
			return 0;
		}
		if (strncmp(*input, "value_%", 7) == 0) {
			*input += 7;
			append_clean(s, ctx->value);
			return 0;
		}
		if (strncmp(*input, "x%", 2) == 0) {
			*input += 2;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->x);
			return 0;
		}
		if (strncmp(*input, "side-x%", 7) == 0) {
			*input += 7;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->front ? ctx->x : ctx->bottom_x);
			return 0;
		}
		if (strncmp(*input, "y%", 2) == 0) {
			*input += 2;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->y);
			return 0;
		}
		if (strncmp(*input, "side-y%", 7) == 0) {
			*input += 7;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->front ? ctx->y : ctx->bottom_y);
			return 0;
		}
		if (strncmp(*input, "padcx%", 6) == 0) {
			*input += 6;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->pad_cx);
			return 0;
		}
		if (strncmp(*input, "side-padcx%", 11) == 0) {
			*input += 11;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->front ? ctx->pad_cx : ctx->bottom_pad_cx);
			return 0;
		}
		if (strncmp(*input, "padcy%", 6) == 0) {
			*input += 6;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->pad_cy);
			return 0;
		}
		if (strncmp(*input, "side-padcy%", 11) == 0) {
			*input += 11;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->front ? ctx->pad_cy : ctx->bottom_pad_cy);
			return 0;
		}
		if (strncmp(*input, "padcx_prerot%", 13) == 0) {
			*input += 13;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->prpad_cx);
			return 0;
		}
		if (strncmp(*input, "padcy_prerot%", 13) == 0) {
			*input += 13;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->prpad_cy);
			return 0;
		}
		if (strncmp(*input, "rot%", 4) == 0) {
			*input += 4;
			rnd_append_printf(s, "%g", xy_normalize_angle(ctx->theta));
			return 0;
		}
		if (strncmp(*input, "negrot%", 7) == 0) {
			*input += 7;
			rnd_append_printf(s, "%g", xy_normalize_angle(-ctx->theta));
			return 0;
		}
		if (strncmp(*input, "siderot%", 8) == 0) { /* old, broken, kept for compatibility */
			*input += 8;
			rnd_append_printf(s, "%g", xy_normalize_angle(ctx->theta));
			return 0;
		}
		if (strncmp(*input, "side-rot%", 9) == 0) {
			*input += 9;
			rnd_append_printf(s, "%g", xy_normalize_angle(ctx->front ? ctx->theta : -ctx->theta));
			return 0;
		}
		if (strncmp(*input, "side-rot180%", 12) == 0) {
			*input += 12;
			rnd_append_printf(s, "%g", xy_normalize_angle(ctx->front ? ctx->theta : -ctx->theta + 180));
			return 0;
		}
		if (strncmp(*input, "side-negrot%", 12) == 0) {
			*input += 12;
			rnd_append_printf(s, "%g", xy_normalize_angle(ctx->front ? -ctx->theta : ctx->theta));
			return 0;
		}
		if (strncmp(*input, "side-negrot180%", 15) == 0) {
			*input += 15;
			rnd_append_printf(s, "%g", xy_normalize_angle((ctx->front ? -ctx->theta : ctx->theta + 180)));
			return 0;
		}
		if (strncmp(*input, "270-rot%", 8) == 0) {
			*input += 8;
			rnd_append_printf(s, "%g", xy_normalize_angle(270-ctx->theta));
			return 0;
		}
		if (strncmp(*input, "side270-rot%", 12) == 0) {
			*input += 12;
			rnd_append_printf(s, "%g", xy_normalize_angle(270-ctx->theta));
			return 0;
		}
		if (strncmp(*input, "90rot%", 6) == 0) {
			*input += 6;
			if (ctx->theta == 0) {
				rnd_append_printf(s, "0");
			} else if (ctx->theta == 90) {
				rnd_append_printf(s, "1");
			} else if (ctx->theta == 180) {
				rnd_append_printf(s, "2");
			} else {
				rnd_append_printf(s, "3");
			}
			return 0;
		}
		if (strncmp(*input, "ncapbbox%", 9) == 0) {
			*input += 9;
			/* need to calculate element bounding box */
			return 0;
		}
		if (strncmp(*input, "side%", 5) == 0) {
			*input += 5;
			gds_append_str(s, ctx->front ? "top" : "bottom");
			return 0;
		}
		if (strncmp(*input, "count%", 6) == 0) {
			*input += 6;
			rnd_append_printf(s, "%d", ctx->count);
			return 0;
		}
		if (strncmp(*input, "num-side%", 9) == 0) {
			*input += 9;
			gds_append_str(s, ctx->front ? "1" : "2");
			return 0;
		}
		if (strncmp(*input, "CHAR-side%", 10) == 0) {
			*input += 10;
			gds_append_str(s, ctx->front ? "T" : "B");
			return 0;
		}
		if (strncmp(*input, "pad_width%", 10) == 0) {
			*input += 10;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->pad_w);
			return 0;
		}
		if (strncmp(*input, "pad_height%", 11) == 0) {
			*input += 11;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->pad_h);
			return 0;
		}
		if (strncmp(*input, "pad_width_prerot%", 17) == 0) {
			*input += 17;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->prpad_w);
			return 0;
		}
		if (strncmp(*input, "pad_height_prerot%", 18) == 0) {
			*input += 18;
			rnd_append_printf(s, "%m+%mN", xy_unit->allow, ctx->prpad_h);
			return 0;
		}
		if (strncmp(*input, "smdvsthru%", 10) == 0) {
			*input += 10;
			count_pins_pads(ctx, &pin_cnt, &pad_cnt);
			if (pin_cnt > 0) {
				rnd_append_printf(s, "PTH");
			} else if (pad_cnt > 0) {
				rnd_append_printf(s, "SMD");
			} else {
				rnd_append_printf(s, "0");
			}
			return 0;
		}
		if (strncmp(*input, "smdvsthrunum%", 13) == 0) {
			*input += 13;
			count_pins_pads(ctx, &pin_cnt, &pad_cnt);
			if (pin_cnt > 0) {
				rnd_append_printf(s, "2");
			} else if (pad_cnt > 0) {
				rnd_append_printf(s, "1");
			} else {
				rnd_append_printf(s, "0");
			}
			return 0;
		}
		if (strncmp(*input, "pincount%", 9) == 0) {
			*input += 9;
			count_pins_pads(ctx, &pin_cnt, &pad_cnt);
			if (pin_cnt > 0) {
				rnd_append_printf(s, "%d", pin_cnt);
			} else if (pad_cnt > 0) {
				rnd_append_printf(s, "%d", pad_cnt);
			} else {
				rnd_append_printf(s, "0");
			}
			return 0;
		}
	}
	if (strncmp(*input, "term.", 5) == 0) {
		*input += 5;
		if (strncmp(*input, "netname%", 8) == 0) {
			*input += 8;
			if (*ctx->pad_netname != '\0')
				rnd_append_printf(s, "%s", ctx->pad_netname);
			else
				rnd_append_printf(s, "NC");
			return 0;
		}
	}
	return -1;
}

static void fprintf_templ(FILE *f, subst_ctx_t *ctx, const char *templ)
{
	if (templ != NULL) {
		char *tmp = rnd_strdup_subst(templ, subst_cb, ctx, RND_SUBST_PERCENT);
		fprintf(f, "%s", tmp);
		free(tmp);
	}
}

static char *sprintf_templ(subst_ctx_t *ctx, const char *templ)
{
	return rnd_strdup_subst(templ, subst_cb, ctx, RND_SUBST_PERCENT);
}


static void xy_translate(subst_ctx_t *ctx, rnd_coord_t *dstx, rnd_coord_t *dsty, rnd_coord_t *bottom_dstx, rnd_coord_t *bottom_dsty, int atrans)
{
	rnd_coord_t x = *dstx, y = *dsty, tx = 0, ty = 0;

	if (atrans) { /* apply attribute translation */
		tx = ctx->tx;
		ty = ctx->ty;
	}

	/* apply attribute translation (affected by the final rotation of the part
	   because the p&p machine will rotate around this point while the subc was
	   rotated around it's pcb-rnd-origin) */
	if (atrans && ((ctx->tx != 0) || (ctx->ty != 0))) {
		if (ctx->theta != 0) {
			double tdeg = ctx->front ? -ctx->theta : ctx->theta;
			double trad = tdeg / RND_RAD_TO_DEG;
			rnd_rotate(&tx, &ty, 0, 0, cos(trad), sin(trad));
		}
		if (!ctx->front)
			ty = -ty;
	}
	x += tx;
	y += ty;

	/* translate the xy coords using origin */
	*dstx = x - ctx->ox;
	*dsty = ctx->oy - y;
	*bottom_dstx = ctx->bottom_ox - x;
	*bottom_dsty = ctx->bottom_oy - y;
}

static const char *xy_xform_get_attr(subst_ctx_t *ctx, pcb_subc_t *subc, const char *key)
{
	ctx->tmp.used = 0;
	gds_append_str(&ctx->tmp, "xy::");

	if ((xy_values[HA_vendor].str != NULL) && (*xy_values[HA_vendor].str != '\0')) {
		gds_append_str(&ctx->tmp, xy_values[HA_vendor].str);
		gds_append_str(&ctx->tmp, "::");
	}

	gds_append_str(&ctx->tmp, key);
	return pcb_attribute_get(&subc->Attributes, ctx->tmp.array);
}

static void xy_xform_by_subc_attrs(subst_ctx_t *ctx, pcb_subc_t *subc)
{
	const char *sval, *sx, *sy;
	char *end;

	ctx->tx = ctx->ty = 0;

	sval = xy_xform_get_attr(ctx, subc, "rotate");
	if (sval != NULL) {
		double r = strtod(sval, &end);
		while(isspace(*end)) end++;
		if (!ctx->front) /* rotation is mirrored on bottom */
			r = -r;
		if (*end != '\0')
			rnd_message(RND_MSG_ERROR, "xy: invalid subc rotate (%s) on subc '%s'\n", sval, ((subc->refdes == NULL) ? "" : subc->refdes));
		else
			ctx->theta += r;
	}

	sval = xy_xform_get_attr(ctx, subc, "translate");
	if (sval != NULL) {
		double x, y;
		rnd_bool succ;

		sx = sval;
		sy = strpbrk(sx, " \t,;");
		if (sy == NULL) {
			rnd_message(RND_MSG_ERROR, "xy: invalid subc translate (%s) on subc '%s': need both x and y\n", sval, ((subc->refdes == NULL) ? "" : subc->refdes));
			return;
		}
		sy++;

		x = rnd_get_value_ex(sx, NULL, NULL, NULL, "mm", &succ);
		if (!succ) {
			rnd_message(RND_MSG_ERROR, "xy: invalid subc translate (%s) on subc '%s': error in X coord\n", sval, ((subc->refdes == NULL) ? "" : subc->refdes));
			return;
		}
		y = rnd_get_value_ex(sy, NULL, NULL, NULL, "mm", &succ);
		if (!succ) {
			rnd_message(RND_MSG_ERROR, "xy: invalid subc translate (%s) on subc '%s': error in Y coord\n", sval, ((subc->refdes == NULL) ? "" : subc->refdes));
			return;
		}
		ctx->tx += x;
		ctx->ty += y;
	}
}

typedef struct {
	const char *hdr, *subc, *term, *foot, *skip_if_nonempty;
} template_t;


static int PrintXY(const template_t *templ, const char *format_name)
{
	FILE *fp;
	subst_ctx_t ctx;

	fp = rnd_fopen_askovr(&PCB->hidlib, xy_filename, "w", NULL);
	if (!fp) {
		rnd_message(RND_MSG_ERROR, "Cannot open file %s for writing\n", xy_filename);
		return 1;
	}

	ctx.count = 0;
	gds_init(&ctx.tmp);

	rnd_print_utc(ctx.utcTime, sizeof(ctx.utcTime), 0);

	find_origin(&ctx, format_name);

	fprintf_templ(fp, &ctx, templ->hdr);

	/* For each subcircuit we calculate the centroid of the footprint. */
	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		int bott, skip = 0;

		if (subc->extobj != NULL) continue;

		ctx.count++;

		ctx.pad_w = ctx.pad_h = 0;
		ctx.theta = 0.0;

		ctx.name = pcb_bom_clean_str((char *) RND_UNKNOWN(pcb_attribute_get(&subc->Attributes, "refdes")));
		ctx.descr = pcb_bom_clean_str((char *) RND_UNKNOWN(pcb_subc_name(subc, "export_xy::footprint")));
		ctx.value = pcb_bom_clean_str((char *) RND_UNKNOWN(pcb_attribute_get(&subc->Attributes, "value")));

		/* prefer the pnp-origin but if that doesn't exist, pick the subc origin */
		if (!pcb_subc_find_aux_point(subc, "pnp-origin", &ctx.x, &ctx.y))
			if (pcb_subc_get_origin(subc, &ctx.x, &ctx.y) != 0)
				rnd_message(RND_MSG_ERROR, "xy: can't get subc origin for %s\n", ctx.name);

		if (pcb_subc_get_rotation(subc, &ctx.theta) != 0) rnd_message(RND_MSG_ERROR, "xy: can't get subc rotation for %s\n", ctx.name);
		if (pcb_subc_get_side(subc, &bott) != 0) rnd_message(RND_MSG_ERROR, "xy: can't get subc side for %s\n", ctx.name);

		ctx.subc = subc;
		ctx.front = !bott;

		xy_xform_by_subc_attrs(&ctx, subc);

		ctx.theta = -ctx.theta;
		if (ctx.theta == -0)
			ctx.theta = 0;

		xy_translate(&ctx, &ctx.x, &ctx.y, &ctx.bottom_x, &ctx.bottom_y, 1);

		calc_pad_bbox(&ctx, 0, &ctx.pad_w, &ctx.pad_h, &ctx.pad_cx, &ctx.pad_cy);
		calc_pad_bbox(&ctx, 1, &ctx.prpad_w, &ctx.prpad_h, &ctx.prpad_cx, &ctx.prpad_cy);
		xy_translate(&ctx, &ctx.pad_cx, &ctx.pad_cy, &ctx.bottom_pad_cx, &ctx.bottom_pad_cy, 0);
		xy_translate(&ctx, &ctx.prpad_cx, &ctx.prpad_cy, &ctx.bottom_prpad_cx, &ctx.bottom_prpad_cy, 0);

		if ((templ->skip_if_nonempty != NULL) && (*templ->skip_if_nonempty != '\0')) {
			char *s = sprintf_templ(&ctx, templ->skip_if_nonempty);
			if ((s != NULL) && (*s != '\0'))
				skip = 1;
			free(s);
		}

		if (!skip)
			fprintf_templ(fp, &ctx, templ->subc);

		for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
			if (o->term != NULL) {
				ctx.pad_netname = NULL;
				{
					pcb_net_term_t *t = pcb_net_find_by_obj(&PCB->netlist[PCB_NETLIST_EDITED], o);
					if (t != NULL)
						ctx.pad_netname = t->parent.net->name;
				}
				if (ctx.pad_netname == NULL)
					ctx.pad_netname = "";
				fprintf_templ(fp, &ctx, templ->term);
			}
		}

		ctx.pad_netname = NULL;

		free(ctx.name);
		free(ctx.descr);
		free(ctx.value);
	}
	PCB_END_LOOP;

	fprintf_templ(fp, &ctx, templ->foot);

	fclose(fp);
	gds_uninit(&ctx.tmp);

	return 0;
}

static void gather_templates(void)
{
	rnd_conf_listitem_t *i;
	int n;

	rnd_conf_loop_list(&conf_xy.plugins.export_xy.templates, i, n) {
		char buff[256], *id, *sect;
		int nl = strlen(i->name);
		if (nl > sizeof(buff)-1) {
			rnd_message(RND_MSG_ERROR, "export_xy: ignoring template '%s': name too long\n", i->name);
			continue;
		}
		memcpy(buff, i->name, nl+1);
		id = buff;
		sect = strchr(id, '.');
		if (sect == NULL) {
			rnd_message(RND_MSG_ERROR, "export_xy: ignoring template '%s': does not have a .section suffix\n", i->name);
			continue;
		}
		*sect = '\0';
		sect++;
	}
}

static const char *get_templ(const char *tid, const char *type)
{
	char path[MAX_TEMP_NAME_LEN + 16];
	rnd_conf_listitem_t *li;
	int idx;

	sprintf(path, "%s.%s", tid, type); /* safe: tid's length is checked before it was put in the vector, type is hardwired in code and is never longer than a few chars */
	rnd_conf_loop_list(&conf_xy.plugins.export_xy.templates, li, idx)
		if (strcmp(li->name, path) == 0)
			return li->payload;
	return NULL;
}

static void xy_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	template_t templ;
	char **tid;
	pcb_cam_t cam;

	memset(&templ, 0, sizeof(templ));


	gather_templates();

	if (!options) {
		xy_get_export_options(hid, 0, design, appspec);
		options = xy_values;
	}

	xy_filename = options[HA_xyfile].str;
	if (!xy_filename)
		xy_filename = "pcb-rnd-out.xy";

	pcb_cam_begin_nolayer(PCB, &cam, NULL, options[HA_cam].str, &xy_filename);

	if ((options[HA_unit].lng < 0) || (options[HA_unit].lng >= rnd_get_n_units()))
		xy_unit = rnd_get_unit_struct("mil");
	else
		xy_unit = rnd_unit_get_idx(options[HA_unit].lng);

	tid = vts0_get(&fmt_ids, options[HA_format].lng, 0);
	if ((tid == NULL) || (*tid == NULL)) {
		rnd_message(RND_MSG_ERROR, "export_xy: invalid template selected\n");
		return;
	}
	templ.hdr  = get_templ(*tid, "hdr");
	templ.subc = get_templ(*tid, "subc");
	templ.term = get_templ(*tid, "term");
	templ.skip_if_nonempty = get_templ(*tid, "skip_if_nonempty");

	PrintXY(&templ, options[HA_format].str);
	pcb_cam_end(&cam);
}

static int xy_usage(rnd_hid_t *hid, const char *topic)
{
	int n;
	fprintf(stderr, "\nXY exporter command line arguments:\n\n");
	xy_get_export_options(hid, &n, NULL, NULL);
	rnd_hid_usage(xy_options, sizeof(xy_options) / sizeof(xy_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x xy [xy_options] foo.pcb\n\n");
	return 0;
}

static int xy_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, xy_options, sizeof(xy_options) / sizeof(xy_options[0]), xy_cookie, 0);

	/* when called from the export() command this field may be uninitialized yet */
	if (xy_options[HA_format].enumerations == NULL)
		xy_get_export_options(hid, NULL, NULL, NULL);

	return rnd_hid_parse_command_line(argc, argv);
}

rnd_hid_t xy_hid;

int pplg_check_ver_export_xy(int ver_needed) { return 0; }

void pplg_uninit_export_xy(void)
{
	rnd_export_remove_opts_by_cookie(xy_cookie);
	rnd_conf_unreg_file(CONF_FN, export_xy_conf_internal);
	rnd_conf_unreg_fields("plugins/export_xy/");
	free_fmts();
	vts0_uninit(&fmt_names);
	vts0_uninit(&fmt_ids);
	rnd_hid_remove_hid(&xy_hid);
}


int pplg_init_export_xy(void)
{
	RND_API_CHK_VER;

	rnd_conf_reg_file(CONF_FN, export_xy_conf_internal);

	memset(&xy_hid, 0, sizeof(rnd_hid_t));

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_xy, field,isarray,type_name,cpath,cname,desc,flags);
#include "xy_conf_fields.h"

	rnd_hid_nogui_init(&xy_hid);

	xy_hid.struct_size = sizeof(rnd_hid_t);
	xy_hid.name = "XY";
	xy_hid.description = "Exports a XY (centroid)";
	xy_hid.exporter = 1;

	xy_hid.get_export_options = xy_get_export_options;
	xy_hid.do_export = xy_do_export;
	xy_hid.parse_arguments = xy_parse_arguments;
	xy_hid.argument_array = xy_values;

	xy_hid.usage = xy_usage;

	rnd_hid_register_hid(&xy_hid);
	rnd_hid_load_defaults(&xy_hid, xy_options, NUM_OPTIONS);

	vts0_init(&fmt_names);
	vts0_init(&fmt_ids);
	return 0;
}
