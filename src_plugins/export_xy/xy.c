#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "compat_misc.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "hid_init.h"

extern char *CleanBOMString(const char *in);


const char *xy_cookie = "bom HID";

static HID_Attribute xy_options[] = {
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

static HID_Attr_Val xy_values[NUM_OPTIONS];

static const char *xy_filename;
static const Unit *xy_unit;

static HID_Attribute *xy_get_export_options(int *n)
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

static double xyToAngle(double x, double y)
{
	double theta;

	if ((x > 0.0) && (y >= 0.0))
		theta = 180.0;
	else if ((x <= 0.0) && (y > 0.0))
		theta = 90.0;
	else if ((x < 0.0) && (y <= 0.0))
		theta = 0.0;
	else if ((x >= 0.0) && (y < 0.0))
		theta = 270.0;
	else {
		theta = 0.0;
		Message(PCB_MSG_DEFAULT, "xyToAngle(): unable to figure out angle of element\n"
						"     because the pin is at the centroid of the part.\n"
						"     This is a BUG!!!\n" "     Setting to %g degrees\n", theta);
	}

	return (theta);
}

static int PrintXY(void)
{
	char utcTime[64];
	Coord x, y;
	double theta = 0.0;
	double sumx, sumy;
	double pin1x = 0.0, pin1y = 0.0, pin1angle = 0.0;
	double pin2x = 0.0, pin2y = 0.0;
	int found_pin1;
	int found_pin2;
	int pin_cnt;
	time_t currenttime;
	FILE *fp;
	char *name, *descr, *value;

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
		found_pin1 = 0;
		found_pin2 = 0;

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

			if (NSTRCMP(pin->Number, "1") == 0) {
				pin1x = (double) pin->X;
				pin1y = (double) pin->Y;
				pin1angle = 0.0;				/* pins have no notion of angle */
				found_pin1 = 1;
			}
			else if (NSTRCMP(pin->Number, "2") == 0) {
				pin2x = (double) pin->X;
				pin2y = (double) pin->Y;
				found_pin2 = 1;
			}
		}
		END_LOOP;

		PAD_LOOP(element);
		{
			sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
			sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
			pin_cnt++;

			if (NSTRCMP(pad->Number, "1") == 0) {
				pin1x = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
				pin1y = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
				/*
				 * NOTE:  We swap the Y points because in PCB, the Y-axis
				 * is inverted.  Increasing Y moves down.  We want to deal
				 * in the usual increasing Y moves up coordinates though.
				 */
				pin1angle = (180.0 / M_PI) * atan2(pad->Point1.Y - pad->Point2.Y, pad->Point2.X - pad->Point1.X);
				found_pin1 = 1;
			}
			else if (NSTRCMP(pad->Number, "2") == 0) {
				pin2x = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
				pin2y = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
				found_pin2 = 1;
			}

		}
		END_LOOP;

		if (pin_cnt > 0) {
			x = sumx / (double) pin_cnt;
			y = sumy / (double) pin_cnt;

			if (found_pin1) {
				/* recenter pin #1 onto the axis which cross at the part
				   centroid */
				pin1x -= x;
				pin1y -= y;
				pin1y = -1.0 * pin1y;

				/* if only 1 pin, use pin 1's angle */
				if (pin_cnt == 1)
					theta = pin1angle;
				else {
					/* if pin #1 is at (0,0) use pin #2 for rotation */
					if ((pin1x == 0.0) && (pin1y == 0.0)) {
						if (found_pin2)
							theta = xyToAngle(pin2x, pin2y);
						else {
							Message
								(PCB_MSG_ERROR,
								 "PrintXY(): unable to figure out angle of element\n"
								 "     %s because pin #1 is at the centroid of the part.\n"
								 "     and I could not find pin #2's location\n"
								 "     Setting to %g degrees\n", UNKNOWN(NAMEONPCB_NAME(element)), theta);
						}
					}
					else
						theta = xyToAngle(pin1x, pin1y);
				}
			}
			/* we did not find pin #1 */
			else {
				theta = 0.0;
				Message
					(PCB_MSG_ERROR,
					 "PrintXY(): unable to figure out angle because I could\n"
					 "     not find pin #1 of element %s\n" "     Setting to %g degrees\n", UNKNOWN(NAMEONPCB_NAME(element)), theta);
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
	}
	END_LOOP;

	fclose(fp);

	return (0);
}

static void xy_do_export(HID_Attr_Val * options)
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

HID xy_hid;

pcb_uninit_t hid_export_xy_init()
{
	memset(&xy_hid, 0, sizeof(HID));

	common_nogui_init(&xy_hid);

	xy_hid.struct_size = sizeof(HID);
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
