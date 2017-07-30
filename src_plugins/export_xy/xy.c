#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "math_helper.h"
#include "build_run.h"
#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "compat_misc.h"
#include "obj_pinvia.h"
#include "netlist.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "hid_init.h"

extern char *CleanBOMString(const char *in);

const char *xy_cookie = "XY HID";

static const int verbose_rot = 0;

static const char *format_names[] = {
#define FORMAT_XY 0
	"pcb xy",
#define FORMAT_GXYRS 1
	"gxyrs",
#define FORMAT_MACROFAB 2
	"Macrofab",
#define FORMAT_TM220TM240 3
	"TM220/TM240",
#define FORMAT_KICADPOS 4
	"KiCad .pos",
#define FORMAT_NCAP 5
	"ncap export (WIP)",
	NULL
};



static pcb_hid_attribute_t xy_options[] = {
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
	{"xy-in-mm", ATTR_UNDOCUMENTED,
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xymm 2
	{"format", "file format (template)",
	 PCB_HATT_ENUM, 0, 0, {0, 0, 0}, format_names, 0},
#define HA_format 3
};

#define NUM_OPTIONS (sizeof(xy_options)/sizeof(xy_options[0]))

static pcb_hid_attr_val_t xy_values[NUM_OPTIONS];

static const char *xy_filename;
static const pcb_unit_t *xy_unit;

static pcb_hid_attribute_t *xy_get_export_options(int *n)
{
	static char *last_xy_filename = 0;
	static int last_unit_value = -1;

	if (xy_options[HA_unit].default_val.int_value == last_unit_value) {
		if (conf_core.editor.grid_unit)
			xy_options[HA_unit].default_val.int_value = conf_core.editor.grid_unit->index;
		else
			xy_options[HA_unit].default_val.int_value = get_unit_struct("mil")->index;
		last_unit_value = xy_options[HA_unit].default_val.int_value;
	}
	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &xy_options[HA_xyfile], ".xy", &last_xy_filename);

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
static double xyToAngle(double x, double y, pcb_bool morethan2pins)
{
	double d = atan2(-y, x) * 180.0 / M_PI;

		if (verbose_rot)
			pcb_trace(" xyToAngle: %f %f %d ->%f\n", x, y, morethan2pins, d);

	/* IPC 7351 defines different rules for 2 pin elements */
	if (morethan2pins) {
		/* Multi pin case:
		 * Output 0 degrees if pin1 in is top left or top, i.e. between angles of
		 * 80 to 170 degrees.
		 * Pin #1 can be at dead top (e.g. certain PLCCs) or anywhere in the top
		 * left.
		 */
		if (d < -100)
			return 90;								/* -180 to -100 */
		else if (d < -10)
			return 180;								/* -100 to -10 */
		else if (d < 80)
			return 270;								/* -10 to 80 */
		else if (d < 170)
			return 0;									/* 80 to 170 */
		else
			return 90;								/* 170 to 180 */
	}
	else {
		/* 2 pin element:
		 * Output 0 degrees if pin #1 is in top left or left, i.e. in sector
		 * between angles of 95 and 185 degrees.
		 */
		if (d < -175)
			return 0;									/* -180 to -175 */
		else if (d < -85)
			return 90;								/* -175 to -85 */
		else if (d < 5)
			return 180;								/* -85 to 5 */
		else if (d < 95)
			return 270;								/* 5 to 95 */
		else
			return 0;									/* 95 to 180 */
	}
}



/*
 * In order of preference.
 * Includes numbered and BGA pins.
 * Possibly BGA pins can be missing, so we add a few to try.
 */
#define MAXREFPINS 32 /* max length of following list */
static char *reference_pin_names[] = {"1", "2", "A1", "A2", "B1", "B2", 0};

typedef struct {
	char utcTime[64];
	char *name, *descr, *value, *fixed_rotation;
	const char *pad_netname;
	pcb_coord_t x, y;
	double theta, xray_theta;
	pcb_element_t *element;
	pcb_coord_t pad_cx, pad_cy;
	pcb_coord_t pad_w, pad_h;
	pcb_coord_t prpad_w, prpad_h;
	int element_num;
	pcb_coord_t ox, oy;
	int origin_score;
	char *origin_tmp;
} subst_ctx_t;

/* Find the pick and place 0;0 mark, if there is any */
static void find_origin_bump(void *ctx_, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	subst_ctx_t *ctx = ctx_;
	char *attr;
	int score;

	/* first look for the format-specific attribute */
	attr = pcb_attribute_get(&line->Attributes, ctx->origin_tmp);
	if (attr != NULL) {
		score = 2;
		goto found;
	}

	/* then for the generic pnp-specific attribute */
	attr = pcb_attribute_get(&line->Attributes, "pnp-origin");
	if (attr != NULL) {
		score = 1;
		goto found;
	}
	return;

	found:;
	if (score > ctx->origin_score) {
		ctx->origin_score = score;
		ctx->ox = (line->BoundingBox.X1 + line->BoundingBox.X2) / 2;
		ctx->oy = (line->BoundingBox.Y1 + line->BoundingBox.Y2) / 2;
	}
}


static void find_origin(subst_ctx_t *ctx, const char *format_name)
{
	char tmp[128];
	pcb_snprintf(tmp, sizeof(tmp), "pnp-origin-%s", format_name);

	ctx->origin_score = 0;
	ctx->ox = ctx->oy = 0;
	ctx->origin_tmp = tmp;

	pcb_loop_layers(ctx, NULL, find_origin_bump, NULL, NULL, NULL);
}

static void calc_pad_bbox_(subst_ctx_t *ctx, pcb_element_t *element, pcb_coord_t *pw, pcb_coord_t *ph, pcb_coord_t *pcx, pcb_coord_t *pcy)
{
	pcb_box_t box, tmp;
	box.X1 = box.Y1 = PCB_MAX_COORD;
	box.X2 = box.Y2 = -PCB_MAX_COORD;

	PCB_PIN_LOOP(element);
	{
		pcb_pin_copper_bbox(&tmp, pin);
		pcb_box_bump_box(&box, &tmp);
	}
	PCB_END_LOOP;

	PCB_PAD_LOOP(element);
	{
		pcb_pad_copper_bbox(&tmp, pad);
		pcb_box_bump_box(&box, &tmp);
	}
	PCB_END_LOOP;

	*pw = box.X2 - box.X1;
	*ph = box.Y2 - box.Y1;
	*pcx = (box.X2 + box.X1)/2;
	*pcy = (box.Y2 + box.Y1)/2;
}

static void count_pins_pads(subst_ctx_t *ctx, pcb_element_t *element, int *pins, int *pads)
{
	*pins = *pads = 0;

	PCB_PIN_LOOP(element);
	{
		(*pins)++;
	}
	PCB_END_LOOP;

	PCB_PAD_LOOP(element);
	{
		(*pads)++;
	}
	PCB_END_LOOP;
}

static void calc_pad_bbox(subst_ctx_t *ctx, int prerot, pcb_coord_t *pw, pcb_coord_t *ph, pcb_coord_t *pcx, pcb_coord_t *pcy)
{
	if (prerot) {
		/* this is what we would do if we wanted to return the pre-rotation state */
		if ((ctx->theta == 0) || (ctx->theta == 180)) {
			calc_pad_bbox_(ctx, ctx->element, pw, ph, pcx, pcy);
			return;
		}
		if ((ctx->theta == 90) || (ctx->theta == 270)) {
			calc_pad_bbox_(ctx, ctx->element, ph, pw, pcx, pcy);
			return;
		}
		pcb_message(PCB_MSG_ERROR, "XY can't calculate pad bbox for non-90-deg rotated elements yet\n");
	}

	calc_pad_bbox_(ctx, ctx->element, pw, ph, pcx, pcy);
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
		gds_append_str(s, PCB_UNKNOWN(PCB->Name));
		return 0;
	}
	if (strncmp(*input, "suffix%", 7) == 0) {
		*input += 7;
		gds_append_str(s, xy_unit->in_suffix);
		return 0;
	}
	if (strncmp(*input, "boardw%", 7) == 0) {
		*input += 7;
		pcb_append_printf(s, "%m+%mS", xy_unit->allow, PCB->MaxWidth);
		return 0;
	}
	if (strncmp(*input, "boardh%", 7) == 0) {
		*input += 7;
		pcb_append_printf(s, "%m+%mS", xy_unit->allow, PCB->MaxHeight);
		return 0;
	}
	if (strncmp(*input, "elem.", 5) == 0) {
		*input += 5;
		if (strncmp(*input, "name%", 5) == 0) {
			*input += 5;
			gds_append_str(s, ctx->name);
			return 0;
		}
		if (strncmp(*input, "name_%", 6) == 0) {
			*input += 6;
			append_clean(s, ctx->name);
			return 0;
		}
		if (strncmp(*input, "descr%", 6) == 0) {
			*input += 6;
			gds_append_str(s, ctx->descr);
			return 0;
		}
		if (strncmp(*input, "descr_%", 7) == 0) {
			*input += 7;
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
			pcb_append_printf(s, "%m+%mS", xy_unit->allow, ctx->x);
			return 0;
		}
		if (strncmp(*input, "y%", 2) == 0) {
			*input += 2;
			pcb_append_printf(s, "%m+%mS", xy_unit->allow, ctx->y);
			return 0;
		}
		if (strncmp(*input, "padcx%", 6) == 0) {
			*input += 6;
			pcb_append_printf(s, "%m+%mS", xy_unit->allow, ctx->pad_cx);
			return 0;
		}
		if (strncmp(*input, "padcy%", 6) == 0) {
			*input += 6;
			pcb_append_printf(s, "%m+%mS", xy_unit->allow, ctx->pad_cy);
			return 0;
		}
		if (strncmp(*input, "rot%", 4) == 0) {
			*input += 4;
			pcb_append_printf(s, "%g", ctx->theta);
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
			gds_append_str(s, PCB_FRONT(ctx->element) == 1 ? "top" : "bottom");
			return 0;
		}
		if (strncmp(*input, "element_num%", 12) == 0) {
			*input += 12;
			pcb_append_printf(s, "%d", ctx->element_num);
			return 0;
		}
		if (strncmp(*input, "num-side%", 9) == 0) {
			*input += 9;
			gds_append_str(s, PCB_FRONT(ctx->element) == 1 ? "1" : "2");
			return 0;
		}
		if (strncmp(*input, "pad_width%", 10) == 0) {
			*input += 10;
			pcb_append_printf(s, "%m+%mS", xy_unit->allow, ctx->pad_w);
			return 0;
		}
		if (strncmp(*input, "pad_height%", 11) == 0) {
			*input += 11;
			pcb_append_printf(s, "%m+%mS", xy_unit->allow, ctx->pad_h);
			return 0;
		}
		if (strncmp(*input, "pad_width_prerot%", 17) == 0) {
			*input += 17;
			pcb_append_printf(s, "%m+%mS", xy_unit->allow, ctx->prpad_w);
			return 0;
		}
		if (strncmp(*input, "pad_height_prerot%", 18) == 0) {
			*input += 18;
			pcb_append_printf(s, "%m+%mS", xy_unit->allow, ctx->prpad_h);
			return 0;
		}
		if (strncmp(*input, "smdvsthru%", 10) == 0) {
			*input += 10;
			count_pins_pads(ctx, ctx->element, &pin_cnt, &pad_cnt);
			if (pin_cnt > 0) {
				pcb_append_printf(s, "PTH");
			} else if (pad_cnt > 0) {
				pcb_append_printf(s, "SMD");
			} else {
				pcb_append_printf(s, "0");
			}
			return 0;
		}
		if (strncmp(*input, "pincount%", 9) == 0) {
			*input += 9;
			count_pins_pads(ctx, ctx->element, &pin_cnt, &pad_cnt);
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
	if (strncmp(*input, "pad.", 4) == 0) {
		*input += 4;
		if (strncmp(*input, "netname%", 8) == 0) {
			*input += 8;
			if (ctx->pad_netname != '\0')
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
		char *tmp = pcb_strdup_subst(templ, subst_cb, ctx);
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
			*y = PCB->MaxHeight - *y;
}

typedef struct {
	const char *hdr, *elem, *pad, *foot;
} template_t;

static int PrintXY(const template_t *templ, const char *format_name)
{
	double sumx, sumy;
	double pin1x = 0.0, pin1y = 0.0;
	int pin_cnt;
	FILE *fp;
	int pinfound[MAXREFPINS];
	double pinx[MAXREFPINS];
	double piny[MAXREFPINS];
	double pinangle[MAXREFPINS];
	double padcentrex, padcentrey;
	double centroidx, centroidy;
	int found_any_not_at_centroid, found_any, rpindex;
	subst_ctx_t ctx;

	fp = fopen(xy_filename, "w");
	if (!fp) {
		pcb_gui->log("Cannot open file %s for writing\n", xy_filename);
		return 1;
	}

	ctx.theta = 0;
	ctx.element_num = 0;

	pcb_print_utc(ctx.utcTime, sizeof(ctx.utcTime), 0);

	find_origin(&ctx, format_name);

	fprintf_templ(fp, &ctx, templ->hdr);

	/*
	 * For each element we calculate the centroid of the footprint.
	 */

	PCB_ELEMENT_LOOP(PCB->Data);
	{
		/* initialize our pin count and our totals for finding the
		   centriod */
		pin_cnt = 0;
		sumx = 0.0;
		sumy = 0.0;
		ctx.pad_w = ctx.pad_h = 0;
		ctx.element_num++;

		/*
		 * iterate over the pins and pads keeping a running count of how
		 * many pins/pads total and the sum of x and y coordinates
		 *
		 * While we're at it, store the location of pin/pad #1 and #2 if
		 * we can find them
		 */

		PCB_PIN_LOOP(element);
		{
			sumx += (double) pin->X;
			sumy += (double) pin->Y;
			pin_cnt++;

			for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
				if (PCB_NSTRCMP(pin->Number, reference_pin_names[rpindex]) == 0) {
					pinx[rpindex] = (double) pin->X;
					piny[rpindex] = (double) pin->Y;
					pinangle[rpindex] = 0.0;	/* pins have no notion of angle */
					pinfound[rpindex] = 1;
				}
			}
		}
		PCB_END_LOOP;

		PCB_PAD_LOOP(element);
		{
			sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
			sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
			pin_cnt++;

			for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
				if (PCB_NSTRCMP(pad->Number, reference_pin_names[rpindex]) == 0) {
					padcentrex = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
					padcentrey = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
					pinx[rpindex] = padcentrex;
					piny[rpindex] = padcentrey;
					/*
					 * NOTE: We swap the Y points because in PCB, the Y-axis
					 * is inverted.  Increasing Y moves down.  We want to deal
					 * in the usual increasing Y moves up coordinates though.
					 */
					pinangle[rpindex] = (180.0 / M_PI) * atan2(pad->Point1.Y - pad->Point2.Y, pad->Point2.X - pad->Point1.X);
					pinfound[rpindex] = 1;
				}
			}
		}
		PCB_END_LOOP;

		if (pin_cnt > 0) {
			centroidx = sumx / (double) pin_cnt;
			centroidy = sumy / (double) pin_cnt;

			if (PCB_NSTRCMP(pcb_attribute_get(&element->Attributes, "xy-centre"), "origin") == 0) {
				ctx.x = element->MarkX;
				ctx.y = element->MarkY;
			}
			else {
				ctx.x = centroidx;
				ctx.y = centroidy;
			}

			ctx.fixed_rotation = pcb_attribute_get(&element->Attributes, "xy-fixed-rotation");
			if (ctx.fixed_rotation) {
				/* The user specified a fixed rotation */
				ctx.theta = atof(ctx.fixed_rotation);
				found_any_not_at_centroid = 1;
				found_any = 1;
			}
			else {
				/* Find first reference pin not at the  centroid  */
				found_any_not_at_centroid = 0;
				found_any = 0;
				ctx.theta = 0.0;
				for (rpindex = 0; reference_pin_names[rpindex] && !found_any_not_at_centroid; rpindex++) {
					if (pinfound[rpindex]) {
						found_any = 1;

						/* Recenter pin "#1" onto the axis which cross at the part
						   centroid */
						pin1x = pinx[rpindex] - ctx.x;
						pin1y = piny[rpindex] - ctx.y;

						if (verbose_rot)
							pcb_trace("\nxy rot: %s pin_cnt=%d pin1x=%d pin1y=%d\n", PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), pin_cnt, pin1x, pin1y);

						/* if only 1 pin, use pin 1's angle */
						if (pin_cnt == 1) {
							ctx.theta = pinangle[rpindex];
							found_any_not_at_centroid = 1;
						}
						else {
							if ((pin1x != 0.0) || (pin1y != 0.0))
								ctx.xray_theta = xyToAngle(pin1x, pin1y, pin_cnt > 2);

							/* flip x, to reverse rotation for elements on back */
							if (PCB_FRONT(element) != 1)
								pin1x = -pin1x;

							if ((pin1x != 0.0) || (pin1y != 0.0)) {
								ctx.theta = xyToAngle(pin1x, pin1y, pin_cnt > 2);
								found_any_not_at_centroid = 1;
							}
						}
						if (verbose_rot)
							pcb_trace(" ->theta=%f\n", ctx.theta);
					}
				}

				if (!found_any) {
					pcb_message
						(PCB_MSG_WARNING, "PrintXY(): unable to figure out angle because I could\n"
						 "     not find a suitable reference pin of element %s\n"
						 "     Setting to %g degrees\n", PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), ctx.theta);
				}
				else if (!found_any_not_at_centroid) {
					pcb_message
						(PCB_MSG_WARNING, "PrintXY(): unable to figure out angle of element\n"
						 "     %s because the reference pin(s) are at the centroid of the part.\n"
						 "     Setting to %g degrees\n", PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), ctx.theta);
				}
			}
		}


		ctx.name = CleanBOMString((char *) PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)));
		ctx.descr = CleanBOMString((char *) PCB_UNKNOWN(PCB_ELEM_NAME_DESCRIPTION(element)));
		ctx.value = CleanBOMString((char *) PCB_UNKNOWN(PCB_ELEM_NAME_VALUE(element)));

		xy_translate(&ctx, &ctx.x, &ctx.y);

		ctx.element = element;
		calc_pad_bbox(&ctx, 0, &ctx.pad_w, &ctx.pad_h, &ctx.pad_cx, &ctx.pad_cy);
		calc_pad_bbox(&ctx, 1, &ctx.prpad_w, &ctx.prpad_h, &ctx.pad_cx, &ctx.pad_cy);
		xy_translate(&ctx, &ctx.pad_cx, &ctx.pad_cy);

		fprintf_templ(fp, &ctx, templ->elem);

		PCB_PIN_LOOP(element);
		{
			pcb_lib_menu_t *m = pcb_netlist_find_net4pin(PCB, pin);
			if (m != NULL)
				ctx.pad_netname = m->Name;
			else
				ctx.pad_netname = NULL;
			fprintf_templ(fp, &ctx, templ->pad);
		}
		PCB_END_LOOP;

		PCB_PAD_LOOP(element);
		{
			pcb_lib_menu_t *m = pcb_netlist_find_net4pad(PCB, pad);
			if (m != NULL)
				ctx.pad_netname = m->Name;
			else
				ctx.pad_netname = NULL;
			fprintf_templ(fp, &ctx, templ->pad);
		}
		PCB_END_LOOP;

		ctx.pad_netname = NULL;

		free(ctx.name);
		free(ctx.descr);
		free(ctx.value);
	}
	PCB_END_LOOP;

	fprintf_templ(fp, &ctx, templ->foot);

	fclose(fp);

	return (0);
}

#include "default_templ.h"

static void xy_do_export(pcb_hid_attr_val_t * options)
{
	int i;
	template_t templ;

	memset(&templ, 0, sizeof(templ));

	if (!options) {
		xy_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			xy_values[i] = xy_options[i].default_val;
		options = xy_values;
	}

	xy_filename = options[HA_xyfile].str_value;
	if (!xy_filename)
		xy_filename = "pcb-out.xy";

	if (options[HA_unit].int_value == -1)
		xy_unit = options[HA_xymm].int_value ? get_unit_struct("mm")
			: get_unit_struct("mil");
	else
		xy_unit = &get_unit_list()[options[HA_unit].int_value];


	switch(options[HA_format].int_value) {
		case FORMAT_XY:
			templ.hdr = templ_xy_hdr;
			templ.elem = templ_xy_elem;
			break;
		case FORMAT_GXYRS:
			templ.hdr = templ_gxyrs_hdr;
			templ.elem = templ_gxyrs_elem;
			break;
		case FORMAT_MACROFAB:
			xy_unit = get_unit_struct("mil"); /* Macrofab requires mils */
			templ.hdr = templ_macrofab_hdr;
			templ.elem = templ_macrofab_elem;
			break;
		case FORMAT_TM220TM240:
			templ.hdr = templ_TM220TM240_hdr;
			templ.elem = templ_TM220TM240_elem;
			break;
		case FORMAT_KICADPOS:
			templ.hdr = templ_KICADPOS_hdr;
			templ.elem = templ_KICADPOS_elem;
			break;
		case FORMAT_NCAP:
			templ.hdr = templ_NCAP_hdr;
			templ.elem = templ_NCAP_elem;
			templ.pad = templ_NCAP_pad;
			break;
		default:
			pcb_message(PCB_MSG_ERROR, "Invalid format\n");
			return;
	}

	PrintXY(&templ, options[HA_format].str_value);
}

static int xy_usage(const char *topic)
{
	fprintf(stderr, "\nXY exporter command line arguments:\n\n");
	pcb_hid_usage(xy_options, sizeof(xy_options) / sizeof(xy_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x xy [xy_options] foo.pcb\n\n");
	return 0;
}

static void xy_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(xy_options, sizeof(xy_options) / sizeof(xy_options[0]), xy_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

pcb_hid_t xy_hid;

int pplg_check_ver_export_xy(int ver_needed) { return 0; }

void pplg_uninit_export_xy(void)
{
	pcb_hid_remove_attributes_by_cookie(xy_cookie);
}

int pplg_init_export_xy(void)
{
	memset(&xy_hid, 0, sizeof(pcb_hid_t));

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
	return 0;
}
