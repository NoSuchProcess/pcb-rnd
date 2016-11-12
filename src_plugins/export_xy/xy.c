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

#include "hid.h"
#include "hid_nogui.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "hid_init.h"

extern char *CleanBOMString(const char *in);


const char *xy_cookie = "bom HID";

static pcb_hid_attribute_t xy_options[] = {
/* %start-doc options "8 XY Creation"
@ftable @code
@item --xyfile <string>
Name of the XY output file.
@end ftable
%end-doc
*/
	{"xyfile", "Name of the XY output file",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xyfile 0

/* %start-doc options "8 BOM Creation"
@ftable @code
@item --xy-unit <unit>
Unit of XY dimensions. Defaults to mil.
@end ftable
%end-doc
*/
	{"xy-unit", "XY units",
	 HID_Unit, 0, 0, {-1, 0, 0}, NULL, 0},
#define HA_unit 1
	{"xy-in-mm", ATTR_UNDOCUMENTED,
	 HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_xymm 2
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
		derive_default_filename(PCB->Filename, &xy_options[HA_xyfile], ".xy", &last_xy_filename);

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

static int PrintXY(void)
{
	char utcTime[64];
	Coord x, y;
	double theta = 0.0;
	double sumx, sumy;
	double pin1x = 0.0, pin1y = 0.0;
	int pin_cnt;
	time_t currenttime;
	FILE *fp;
	char *name, *descr, *value, *fixed_rotation;
	int pinfound[MAXREFPINS];
	double pinx[MAXREFPINS];
	double piny[MAXREFPINS];
	double pinangle[MAXREFPINS];
	double padcentrex, padcentrey;
	double centroidx, centroidy;
	int found_any_not_at_centroid, found_any, rpindex;


	fp = fopen(xy_filename, "w");
	if (!fp) {
		gui->log("Cannot open file %s for writing\n", xy_filename);
		return 1;
	}

	/* Create a portable timestamp. */
	currenttime = time(NULL);
	{
		/* avoid gcc complaints */
		const char *fmt = "%c UTC";
		strftime(utcTime, sizeof(utcTime), fmt, gmtime(&currenttime));
	}
	fprintf(fp, "# $Id");
	fprintf(fp, "$\n");
	fprintf(fp, "# PcbXY Version 1.0\n");
	fprintf(fp, "# Date: %s\n", utcTime);
	fprintf(fp, "# Author: %s\n", pcb_author());
	fprintf(fp, "# Title: %s - PCB X-Y\n", UNKNOWN(PCB->Name));
	fprintf(fp, "# RefDes, Description, Value, X, Y, rotation, top/bottom\n");
	fprintf(fp, "# X,Y in %s.  rotation in degrees.\n", xy_unit->in_suffix);
	fprintf(fp, "# --------------------------------------------\n");

	/*
	 * For each element we calculate the centroid of the footprint.
	 */

	ELEMENT_LOOP(PCB->Data);
	{
		/* initialize our pin count and our totals for finding the
		   centriod */
		pin_cnt = 0;
		sumx = 0.0;
		sumy = 0.0;

		/*
		 * iterate over the pins and pads keeping a running count of how
		 * many pins/pads total and the sum of x and y coordinates
		 *
		 * While we're at it, store the location of pin/pad #1 and #2 if
		 * we can find them
		 */

		PIN_LOOP(element);
		{
			sumx += (double) pin->X;
			sumy += (double) pin->Y;
			pin_cnt++;

			for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
				if (NSTRCMP(pin->Number, reference_pin_names[rpindex]) == 0) {
					pinx[rpindex] = (double) pin->X;
					piny[rpindex] = (double) pin->Y;
					pinangle[rpindex] = 0.0;	/* pins have no notion of angle */
					pinfound[rpindex] = 1;
				}
			}
		}
		END_LOOP;

		PAD_LOOP(element);
		{
			sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
			sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
			pin_cnt++;

			for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
				if (NSTRCMP(pad->Number, reference_pin_names[rpindex]) == 0) {
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
		END_LOOP;

		if (pin_cnt > 0) {
			centroidx = sumx / (double) pin_cnt;
			centroidy = sumy / (double) pin_cnt;

			if (NSTRCMP(AttributeGetFromList(&element->Attributes, "xy-centre"), "origin") == 0) {
				x = element->MarkX;
				y = element->MarkY;
			}
			else {
				x = centroidx;
				y = centroidy;
			}

			fixed_rotation = AttributeGetFromList(&element->Attributes, "xy-fixed-rotation");
			if (fixed_rotation) {
				/* The user specified a fixed rotation */
				theta = atof(fixed_rotation);
				found_any_not_at_centroid = 1;
				found_any = 1;
			}
			else {
				/* Find first reference pin not at the  centroid  */
				found_any_not_at_centroid = 0;
				found_any = 0;
				theta = 0.0;
				for (rpindex = 0; reference_pin_names[rpindex] && !found_any_not_at_centroid; rpindex++) {
					if (pinfound[rpindex]) {
						found_any = 1;

						/* Recenter pin "#1" onto the axis which cross at the part
						   centroid */
						pin1x = pinx[rpindex] - x;
						pin1y = piny[rpindex] - y;

						/* flip x, to reverse rotation for elements on back */
						if (FRONT(element) != 1)
							pin1x = -pin1x;

						/* if only 1 pin, use pin 1's angle */
						if (pin_cnt == 1) {
							theta = pinangle[rpindex];
							found_any_not_at_centroid = 1;
						}
						else if ((pin1x != 0.0) || (pin1y != 0.0)) {
							theta = xyToAngle(pin1x, pin1y, pin_cnt > 2);
							found_any_not_at_centroid = 1;
						}
					}
				}

				if (!found_any) {
					Message
						(PCB_MSG_WARNING, "PrintBOM(): unable to figure out angle because I could\n"
						 "     not find a suitable reference pin of element %s\n"
						 "     Setting to %g degrees\n", UNKNOWN(NAMEONPCB_NAME(element)), theta);
				}
				else if (!found_any_not_at_centroid) {
					Message
						(PCB_MSG_WARNING, "PrintBOM(): unable to figure out angle of element\n"
						 "     %s because the reference pin(s) are at the centroid of the part.\n"
						 "     Setting to %g degrees\n", UNKNOWN(NAMEONPCB_NAME(element)), theta);
				}
			}
		}


		name = CleanBOMString((char *) UNKNOWN(NAMEONPCB_NAME(element)));
		descr = CleanBOMString((char *) UNKNOWN(DESCRIPTION_NAME(element)));
		value = CleanBOMString((char *) UNKNOWN(VALUE_NAME(element)));

		y = PCB->MaxHeight - y;
		pcb_fprintf(fp, "%m+%s,\"%s\",\"%s\",%mS,%.2mS,%g,%s\n",
								xy_unit->allow, name, descr, value, x, y, theta, FRONT(element) == 1 ? "top" : "bottom");
		free(name);
		free(descr);
		free(value);
	}
	END_LOOP;

	fclose(fp);

	return (0);
}

static void xy_do_export(pcb_hid_attr_val_t * options)
{
	int i;

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
	PrintXY();
}

static int xy_usage(const char *topic)
{
	fprintf(stderr, "\nXY exporter command line arguments:\n\n");
	hid_usage(xy_options, sizeof(xy_options) / sizeof(xy_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x xy foo.pcb [xy_options]\n\n");
	return 0;
}

static void xy_parse_arguments(int *argc, char ***argv)
{
	hid_register_attributes(xy_options, sizeof(xy_options) / sizeof(xy_options[0]), xy_cookie, 0);
	hid_parse_command_line(argc, argv);
}

pcb_hid_t xy_hid;

pcb_uninit_t hid_export_xy_init()
{
	memset(&xy_hid, 0, sizeof(pcb_hid_t));

	common_nogui_init(&xy_hid);

	xy_hid.struct_size = sizeof(pcb_hid_t);
	xy_hid.name = "XY";
	xy_hid.description = "Exports a XY (centroid)";
	xy_hid.exporter = 1;

	xy_hid.get_export_options = xy_get_export_options;
	xy_hid.do_export = xy_do_export;
	xy_hid.parse_arguments = xy_parse_arguments;

	xy_hid.usage = xy_usage;

	hid_register_hid(&xy_hid);
	return NULL;
}
