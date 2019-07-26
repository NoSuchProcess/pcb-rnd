#include "config.h"
#include "conf_core.h"
#include "hidlib_conf.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <genvector/vts0.h>

#include "math_helper.h"
#include "build_run.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include "error.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "compat_misc.h"
#include "obj_pstk_inlines.h"
#include "obj_subc_op.h"
#include "layer.h"
#include "netlist2.h"
#include "safe_fs.h"
#include "macro.h"
#include "operation.h"
#include "xy_conf.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_attrib.h"
#include "hid_cam.h"
#include "hid_init.h"

#include "../src_plugins/lib_compat_help/elem_rot.h"
#include "../src_plugins/export_xy/conf_internal.c"

#define CONF_FN "export_xy.conf"

conf_xy_t conf_xy;

extern char *CleanBOMString(const char *in);

const char *xy_cookie = "XY HID";

/* Maximum length of a template name (in the config file, in the enum) */
#define MAX_TEMP_NAME_LEN 128


static pcb_export_opt_t xy_options[] = {
/* %start-doc options "8 XY Creation"
@ftable @code
@item --xyfile <string>
Name of the XY output file.
@end ftable
%end-doc
*/
	{"xyfile", "Name of the XY output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xyfile 0

/* %start-doc options "8 XY Creation"
@ftable @code
@item --xy-unit <unit>
Unit of XY dimensions. Defaults to mil.
@end ftable
%end-doc
*/
	{"xy-unit", "XY units",
	 PCB_HATT_UNIT, 0, 0, {-1, 0, 0}, NULL, 0},
#define HA_unit 1
	{"format", "file format (template)",
	 PCB_HATT_ENUM, 0, 0, {0, 0, 0}, NULL, 0},
#define HA_format 2
};

#define NUM_OPTIONS (sizeof(xy_options)/sizeof(xy_options[0]))

static pcb_hid_attr_val_t xy_values[NUM_OPTIONS];

static const char *xy_filename;
static const pcb_unit_t *xy_unit;
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

static pcb_export_opt_t *xy_get_export_options(pcb_hid_t *hid, int *n)
{
	static int last_unit_value = -1;
	conf_listitem_t *li;
	int idx;

	/* load all formats from the config */
	fmt_names.used = 0;
	fmt_ids.used = 0;

	free_fmts();
	conf_loop_list(&conf_xy.plugins.export_xy.templates, li, idx) {
		char id[MAX_TEMP_NAME_LEN];
		const char *sep = strchr(li->name, '.');
		int len;

		if (sep == NULL) {
			pcb_message(PCB_MSG_ERROR, "export_xy: ignoring invalid template name (missing period): '%s'\n", li->name);
			continue;
		}
		if (strcmp(sep+1, "name") != 0)
			continue;
		len = sep - li->name;
		if (len > sizeof(id)-1) {
			pcb_message(PCB_MSG_ERROR, "export_xy: ignoring invalid template name (too long): '%s'\n", li->name);
			continue;
		}
		memcpy(id, li->name, len);
		id[len] = '\0';
		vts0_append(&fmt_names, (char *)li->payload);
		vts0_append(&fmt_ids, pcb_strdup(id));
	}

	if (fmt_names.used == 0) {
		pcb_message(PCB_MSG_ERROR, "export_xy: can not set up export options: no template available\n");
		return NULL;
	}

	xy_options[HA_format].enumerations = (const char **)fmt_names.array;

	/* set default unit and filename */
	if (xy_options[HA_unit].default_val.lng == last_unit_value) {
		if (pcbhl_conf.editor.grid_unit)
			xy_options[HA_unit].default_val.lng = pcbhl_conf.editor.grid_unit->index;
		else
			xy_options[HA_unit].default_val.lng = get_unit_struct("mil")->index;
		last_unit_value = xy_options[HA_unit].default_val.lng;
	}
	if ((PCB != NULL)  && (xy_options[HA_xyfile].default_val.str_value == NULL))
		pcb_derive_default_filename(PCB->hidlib.filename, &xy_options[HA_xyfile], ".xy");

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
	pcb_coord_t x, y;
	double theta, xray_theta;
	pcb_subc_t *subc;
	pcb_coord_t pad_cx, pad_cy;
	pcb_coord_t pad_w, pad_h;
	pcb_coord_t prpad_cx, prpad_cy;
	pcb_coord_t prpad_w, prpad_h;
	pcb_cardinal_t count;
	pcb_coord_t ox, oy;
	int origin_score;
	char *origin_tmp;
	pcb_bool front;
} subst_ctx_t;

/* Find the pick and place 0;0 mark, if there is any */
static void find_origin(subst_ctx_t *ctx, const char *format_name)
{
	char tmp[128];
	pcb_data_it_t it;
	pcb_any_obj_t *obj;

	pcb_snprintf(tmp, sizeof(tmp), "pnp-origin-%s", format_name);

	ctx->origin_score = 0;
	ctx->ox = ctx->oy = 0;
	ctx->origin_tmp = tmp;

	for(obj = pcb_data_first(&it, PCB->Data, PCB_OBJ_CLASS_REAL); obj != NULL; obj = pcb_data_next(&it)) {
		int score;

		if (pcb_attribute_get(&obj->Attributes, ctx->origin_tmp) != NULL)
			score = 2; /* first look for the format-specific attribute */
		else if (pcb_attribute_get(&obj->Attributes, "pnp-origin") != NULL)
			score = 1; /* then for the generic pnp-specific attribute */
		else
			continue;

		if (score > ctx->origin_score) {
			ctx->origin_score = score;
			pcb_obj_center(obj, &ctx->ox, &ctx->oy);
		}
	}
}

static void calc_pad_bbox_(subst_ctx_t *ctx, pcb_coord_t *pw, pcb_coord_t *ph, pcb_coord_t *pcx, pcb_coord_t *pcy)
{
	pcb_box_t box;
	box.X1 = box.Y1 = PCB_MAX_COORD;
	box.X2 = box.Y2 = -PCB_MAX_COORD;

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
				pcb_box_bump_box(&box, &o->bbox_naked);
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
			if ((lyt & PCB_LYT_COPPER) && (lyt & (PCB_LYT_TOP || PCB_LYT_TOP)))
				(*pads)++;
		}
	}
}

static void calc_pad_bbox(subst_ctx_t *ctx, int prerot, pcb_coord_t *pw, pcb_coord_t *ph, pcb_coord_t *pcx, pcb_coord_t *pcy)
{
	if (prerot) {
		pcb_subc_t *save, *sc_rot0;
		pcb_data_t *tmp = pcb_buffers[PCB_MAX_BUFFER-1].Data;
		double ang;
		pcb_opctx_t op;

		/* this is what we would do if we wanted to return the pre-rotation state */
		if ((ctx->theta == 0) || (ctx->theta == 180) || (ctx->theta == -180)) {
			calc_pad_bbox_(ctx, pw, ph, pcx, pcy);
			return;
		}
		if ((ctx->theta == 90) || (ctx->theta == 270) || (ctx->theta == -90) || (ctx->theta == -270)) {
			calc_pad_bbox_(ctx, ph, pw, pcx, pcy);
			return;
		}
		
		sc_rot0 = pcb_subc_dup_at(NULL, tmp, ctx->subc, 0, 0, 0);
		ang = ctx->theta / PCB_RAD_TO_DEG;
		pcb_subc_rotate(sc_rot0, 0, 0, cos(ang), sin(ang), ctx->theta);

		save = ctx->subc;
		ctx->subc = sc_rot0;
		calc_pad_bbox_(ctx, pw, ph, pcx, pcy);
		ctx->subc = save;

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
		gds_append_str(s, PCB_UNKNOWN(PCB->hidlib.name));
		return 0;
	}
	if (strncmp(*input, "suffix%", 7) == 0) {
		*input += 7;
		gds_append_str(s, xy_unit->suffix);
		return 0;
	}
	if (strncmp(*input, "boardw%", 7) == 0) {
		*input += 7;
		pcb_append_printf(s, "%m+%mN", xy_unit->allow, PCB->hidlib.size_x);
		return 0;
	}
	if (strncmp(*input, "boardh%", 7) == 0) {
		*input += 7;
		pcb_append_printf(s, "%m+%mN", xy_unit->allow, PCB->hidlib.size_y);
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
				pcb_message(PCB_MSG_ERROR, "xy tempalte error: attribute name '%s' too long\n", *input);
				return 1;
			}
			memcpy(aname, *input, len);
			aname[len] = '\0';
			if (*end == '|') { /* "or unknown" */
				*input = end+1;
				end = strchr(*input, '%');
				len = end - *input;
				if (len >= sizeof(unk_buf) - 1) {
					pcb_message(PCB_MSG_ERROR, "xy tempalte error: elem atribute '|unknown' field '%s' too long\n", *input);
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
					pcb_message(PCB_MSG_ERROR, "xy tempalte error: elem atribute trenary field '%s' too long\n", *input);
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
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->x);
			return 0;
		}
		if (strncmp(*input, "y%", 2) == 0) {
			*input += 2;
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->y);
			return 0;
		}
		if (strncmp(*input, "padcx%", 6) == 0) {
			*input += 6;
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->pad_cx);
			return 0;
		}
		if (strncmp(*input, "padcy%", 6) == 0) {
			*input += 6;
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->pad_cy);
			return 0;
		}
		if (strncmp(*input, "padcx_prerot%", 13) == 0) {
			*input += 13;
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->prpad_cx);
			return 0;
		}
		if (strncmp(*input, "padcy_prerot%", 13) == 0) {
			*input += 13;
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->prpad_cy);
			return 0;
		}
		if (strncmp(*input, "rot%", 4) == 0) {
			*input += 4;
			pcb_append_printf(s, "%g", ctx->theta);
			return 0;
		}
		if (strncmp(*input, "negrot%", 7) == 0) {
			*input += 7;
			pcb_append_printf(s, "%g", -ctx->theta);
			return 0;
		}
		if (strncmp(*input, "siderot%", 8) == 0) {
			*input += 8;
			pcb_append_printf(s, "%g", ctx->xray_theta);
			return 0;
		}
		if (strncmp(*input, "270-rot%", 8) == 0) {
			*input += 8;
			pcb_append_printf(s, "%g", (270-ctx->theta));
			return 0;
		}
		if (strncmp(*input, "side270-rot%", 12) == 0) {
			*input += 12;
			pcb_append_printf(s, "%g", (270-ctx->theta));
			return 0;
		}
		if (strncmp(*input, "90rot%", 6) == 0) {
			*input += 6;
			if (ctx->theta == 0) {
				pcb_append_printf(s, "0");
			} else if (ctx->theta == 90) {
				pcb_append_printf(s, "1");
			} else if (ctx->theta == 180) {
				pcb_append_printf(s, "2");
			} else {
				pcb_append_printf(s, "3");
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
			pcb_append_printf(s, "%d", ctx->count);
			return 0;
		}
		if (strncmp(*input, "num-side%", 9) == 0) {
			*input += 9;
			gds_append_str(s, ctx->front ? "1" : "2");
			return 0;
		}
		if (strncmp(*input, "pad_width%", 10) == 0) {
			*input += 10;
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->pad_w);
			return 0;
		}
		if (strncmp(*input, "pad_height%", 11) == 0) {
			*input += 11;
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->pad_h);
			return 0;
		}
		if (strncmp(*input, "pad_width_prerot%", 17) == 0) {
			*input += 17;
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->prpad_w);
			return 0;
		}
		if (strncmp(*input, "pad_height_prerot%", 18) == 0) {
			*input += 18;
			pcb_append_printf(s, "%m+%mN", xy_unit->allow, ctx->prpad_h);
			return 0;
		}
		if (strncmp(*input, "smdvsthru%", 10) == 0) {
			*input += 10;
			count_pins_pads(ctx, &pin_cnt, &pad_cnt);
			if (pin_cnt > 0) {
				pcb_append_printf(s, "PTH");
			} else if (pad_cnt > 0) {
				pcb_append_printf(s, "SMD");
			} else {
				pcb_append_printf(s, "0");
			}
			return 0;
		}
		if (strncmp(*input, "smdvsthrunum%", 13) == 0) {
			*input += 13;
			count_pins_pads(ctx, &pin_cnt, &pad_cnt);
			if (pin_cnt > 0) {
				pcb_append_printf(s, "2");
			} else if (pad_cnt > 0) {
				pcb_append_printf(s, "1");
			} else {
				pcb_append_printf(s, "0");
			}
			return 0;
		}
		if (strncmp(*input, "pincount%", 9) == 0) {
			*input += 9;
			count_pins_pads(ctx, &pin_cnt, &pad_cnt);
			if (pin_cnt > 0) {
				pcb_append_printf(s, "%d", pin_cnt);
			} else if (pad_cnt > 0) {
				pcb_append_printf(s, "%d", pad_cnt);
			} else {
				pcb_append_printf(s, "0");
			}
			return 0;
		}
	}
	if (strncmp(*input, "term.", 5) == 0) {
		*input += 5;
		if (strncmp(*input, "netname%", 8) == 0) {
			*input += 8;
			if (*ctx->pad_netname != '\0')
				pcb_append_printf(s, "%s", ctx->pad_netname);
			else
				pcb_append_printf(s, "NC");
			return 0;
		}
	}
	return -1;
}

static void fprintf_templ(FILE *f, subst_ctx_t *ctx, const char *templ)
{
	if (templ != NULL) {
		char *tmp = pcb_strdup_subst(templ, subst_cb, ctx, PCB_SUBST_PERCENT);
		fprintf(f, "%s", tmp);
		free(tmp);
	}
}

static void xy_translate(subst_ctx_t *ctx, pcb_coord_t *x, pcb_coord_t *y)
{
		/* translate the xy coords using explicit or implicit origin; implicit origin
		   is lower left corner (looking from top) of board extents */
		if (ctx->origin_score > 0) {
			*y = ctx->oy - *y;
			*x = *x - ctx->ox;
		}
		else
			*y = PCB->hidlib.size_y - *y;
}

typedef struct {
	const char *hdr, *subc, *term, *foot;
} template_t;


static int PrintXY(const template_t *templ, const char *format_name)
{
	FILE *fp;
	subst_ctx_t ctx;

	fp = pcb_fopen(&PCB->hidlib, xy_filename, "w");
	if (!fp) {
		pcb_message(PCB_MSG_ERROR, "Cannot open file %s for writing\n", xy_filename);
		return 1;
	}

	ctx.count = 0;

	pcb_print_utc(ctx.utcTime, sizeof(ctx.utcTime), 0);

	find_origin(&ctx, format_name);

	fprintf_templ(fp, &ctx, templ->hdr);

	/* For each subcircuit we calculate the centroid of the footprint. */
	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		int bott;
		ctx.count++;

		ctx.pad_w = ctx.pad_h = 0;
		ctx.theta = ctx.xray_theta = 0.0;

		ctx.name = CleanBOMString((char *) PCB_UNKNOWN(pcb_attribute_get(&subc->Attributes, "refdes")));
		ctx.descr = CleanBOMString((char *) PCB_UNKNOWN(pcb_subc_name(subc, "export_xy::footprint")));
		ctx.value = CleanBOMString((char *) PCB_UNKNOWN(pcb_attribute_get(&subc->Attributes, "value")));

		/* prefer the pnp-origin but if that doesn't exist, pick the subc origin */
		if (!pcb_subc_find_aux_point(subc, "pnp-origin", &ctx.x, &ctx.y))
			if (pcb_subc_get_origin(subc, &ctx.x, &ctx.y) != 0)
				pcb_message(PCB_MSG_ERROR, "xy: can't get subc origin for %s\n", ctx.name);

		if (pcb_subc_get_rotation(subc, &ctx.theta) != 0) pcb_message(PCB_MSG_ERROR, "xy: can't get subc rotation for %s\n", ctx.name);
		if (pcb_subc_get_side(subc, &bott) != 0) pcb_message(PCB_MSG_ERROR, "xy: can't get subc side for %s\n", ctx.name);

		ctx.theta = -ctx.theta;
		if (ctx.theta == -0)
			ctx.theta = 0;
	
		xy_translate(&ctx, &ctx.x, &ctx.y);

		ctx.subc = subc;
		ctx.front = !bott;

TODO("padstack: do not depend on this, just use the normal bbox and rotate that back")
		calc_pad_bbox(&ctx, 0, &ctx.pad_w, &ctx.pad_h, &ctx.pad_cx, &ctx.pad_cy);
		calc_pad_bbox(&ctx, 1, &ctx.prpad_w, &ctx.prpad_h, &ctx.prpad_cx, &ctx.prpad_cy);
		xy_translate(&ctx, &ctx.pad_cx, &ctx.pad_cy);

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

	return 0;
}

static void gather_templates(void)
{
	conf_listitem_t *i;
	int n;

	conf_loop_list(&conf_xy.plugins.export_xy.templates, i, n) {
		char buff[256], *id, *sect;
		int nl = strlen(i->name);
		if (nl > sizeof(buff)-1) {
			pcb_message(PCB_MSG_ERROR, "export_xy: ignoring template '%s': name too long\n", i->name);
			continue;
		}
		memcpy(buff, i->name, nl+1);
		id = buff;
		sect = strchr(id, '.');
		if (sect == NULL) {
			pcb_message(PCB_MSG_ERROR, "export_xy: ignoring template '%s': does not have a .section suffix\n", i->name);
			continue;
		}
		*sect = '\0';
		sect++;
	}
}

static const char *get_templ(const char *tid, const char *type)
{
	char path[MAX_TEMP_NAME_LEN + 16];
	conf_listitem_t *li;
	int idx;

	sprintf(path, "%s.%s", tid, type); /* safe: tid's length is checked before it was put in the vector, type is hardwired in code and is never longer than a few chars */
	conf_loop_list(&conf_xy.plugins.export_xy.templates, li, idx)
		if (strcmp(li->name, path) == 0)
			return li->payload;
	return NULL;
}

static void xy_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	int i;
	template_t templ;
	char **tid;

	memset(&templ, 0, sizeof(templ));

	gather_templates();

	if (!options) {
		xy_get_export_options(hid, 0);
		for (i = 0; i < NUM_OPTIONS; i++)
			xy_values[i] = xy_options[i].default_val;
		options = xy_values;
	}

	xy_filename = options[HA_xyfile].str_value;
	if (!xy_filename)
		xy_filename = "pcb-out.xy";

	if (options[HA_unit].lng == -1)
		xy_unit = get_unit_struct("mil");
	else
		xy_unit = &pcb_units[options[HA_unit].lng];

	tid = vts0_get(&fmt_ids, options[HA_format].lng, 0);
	if ((tid == NULL) || (*tid == NULL)) {
		pcb_message(PCB_MSG_ERROR, "export_xy: invalid template selected\n");
		return;
	}
	templ.hdr  = get_templ(*tid, "hdr");
	templ.subc = get_templ(*tid, "subc");
	templ.term = get_templ(*tid, "term");

	PrintXY(&templ, options[HA_format].str_value);
}

static int xy_usage(pcb_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nXY exporter command line arguments:\n\n");
	pcb_hid_usage(xy_options, sizeof(xy_options) / sizeof(xy_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x xy [xy_options] foo.pcb\n\n");
	return 0;
}

static int xy_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	pcb_hid_register_attributes(xy_options, sizeof(xy_options) / sizeof(xy_options[0]), xy_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}

pcb_hid_t xy_hid;

int pplg_check_ver_export_xy(int ver_needed) { return 0; }

void pplg_uninit_export_xy(void)
{
	pcb_hid_remove_attributes_by_cookie(xy_cookie);
	conf_unreg_file(CONF_FN, export_xy_conf_internal);
	conf_unreg_fields("plugins/export_xy/");
	free_fmts();
	vts0_uninit(&fmt_names);
	vts0_uninit(&fmt_ids);
}


int pplg_init_export_xy(void)
{
	PCB_API_CHK_VER;

	conf_reg_file(CONF_FN, export_xy_conf_internal);

	memset(&xy_hid, 0, sizeof(pcb_hid_t));

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_xy, field,isarray,type_name,cpath,cname,desc,flags);
#include "xy_conf_fields.h"

	pcb_hid_nogui_init(&xy_hid);

	xy_hid.struct_size = sizeof(pcb_hid_t);
	xy_hid.name = "XY";
	xy_hid.description = "Exports a XY (centroid)";
	xy_hid.exporter = 1;

	xy_hid.get_export_options = xy_get_export_options;
	xy_hid.do_export = xy_do_export;
	xy_hid.parse_arguments = xy_parse_arguments;

	xy_hid.usage = xy_usage;

	pcb_hid_register_hid(&xy_hid);

	vts0_init(&fmt_names);
	vts0_init(&fmt_ids);
	return 0;
}
