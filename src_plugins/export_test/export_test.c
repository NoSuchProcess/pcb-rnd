#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "build_run.h"
#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "plugins.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "hid_init.h"

const char *export_test_cookie = "export_test HID";

static pcb_hid_attribute_t export_test_options[] = {
/* %start-doc options "8 export_test Creation"
@ftable @code
@item --export_testfile <string>
Name of the export_test output file. Use stdout if not specified.
@end ftable
%end-doc
*/
	{"export_testfile", "Name of the export_test output file",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_export_testfile 0
};

#define NUM_OPTIONS (sizeof(export_test_options)/sizeof(export_test_options[0]))

static pcb_hid_attr_val_t export_test_values[NUM_OPTIONS];

static const char *export_test_filename;

static pcb_hid_attribute_t *export_test_get_export_options(int *n)
{
	static char *last_export_test_filename = 0;

	if (PCB) {
		pcb_derive_default_filename(PCB->Filename, &export_test_options[HA_export_testfile], ".export_test", &last_export_test_filename);
	}

	if (n)
		*n = NUM_OPTIONS;
	return export_test_options;
}


/* 
 * If fp is not NULL then print out the bill of materials contained in
 * export_test.  Either way, free all memory which has been allocated for export_test.
 */
static void print_and_free(FILE * fp, /*export_testList*/ void * export_test)
{
#if 0
	export_testList *lastb;
	StringList *lasts;
	char *descr, *value;

	while (export_test != NULL) {
		if (fp) {
			descr = Cleanexport_testString(export_test->descr);
			value = Cleanexport_testString(export_test->value);
			fprintf(fp, "%d,\"%s\",\"%s\",", export_test->num, descr, value);
			free(descr);
			free(value);
		}

		while (export_test->refdes != NULL) {
			if (fp) {
				fprintf(fp, "%s ", export_test->refdes->str);
			}
			free(export_test->refdes->str);
			lasts = export_test->refdes;
			export_test->refdes = export_test->refdes->next;
			free(lasts);
		}
		if (fp) {
			fprintf(fp, "\n");
		}
		lastb = export_test;
		export_test = export_test->next;
		free(lastb);
	}
#endif
}

static int Printexport_test(void)
{
#if 0
	char utcTime[64];
	pcb_coord_t x, y;
	double theta = 0.0;
	double sumx, sumy;
	double pin1x = 0.0, pin1y = 0.0, pin1angle = 0.0;
	double pin2x = 0.0, pin2y = 0.0;
	int found_pin1;
	int found_pin2;
	int pin_cnt;
	time_t currenttime;
	FILE *fp;
	export_testList *export_test = NULL;
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
	 * In addition, we need to extract some notion of rotation.  
	 * While here generate the export_test list
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

		/* insert this component into the bill of materials list */
		export_test = export_test_insert((char *) UNKNOWN(NAMEONPCB_NAME(element)),
										 (char *) UNKNOWN(DESCRIPTION_NAME(element)), (char *) UNKNOWN(VALUE_NAME(element)), export_test);


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
								("Printexport_test(): unable to figure out angle of element\n"
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
					("Printexport_test(): unable to figure out angle because I could\n"
					 "     not find pin #1 of element %s\n" "     Setting to %g degrees\n", UNKNOWN(NAMEONPCB_NAME(element)), theta);
			}

			name = Cleanexport_testString((char *) UNKNOWN(NAMEONPCB_NAME(element)));
			descr = Cleanexport_testString((char *) UNKNOWN(DESCRIPTION_NAME(element)));
			value = Cleanexport_testString((char *) UNKNOWN(VALUE_NAME(element)));

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

	/* Now print out a Bill of Materials file */

	fp = fopen(export_test_filename, "w");
	if (!fp) {
		gui->log("Cannot open file %s for writing\n", export_test_filename);
		print_and_free(NULL, export_test);
		return 1;
	}

	fprintf(fp, "# $Id");
	fprintf(fp, "$\n");
	fprintf(fp, "# Pcbexport_test Version 1.0\n");
	fprintf(fp, "# Date: %s\n", utcTime);
	fprintf(fp, "# Author: %s\n", pcb_author());
	fprintf(fp, "# Title: %s - PCB export_test\n", UNKNOWN(PCB->Name));
	fprintf(fp, "# Quantity, Description, Value, RefDes\n");
	fprintf(fp, "# --------------------------------------------\n");

	print_and_free(fp, export_test);

	fclose(fp);

#endif
	return (0);
}

static void export_test_do_export(pcb_hid_attr_val_t * options)
{
	int i;

	if (!options) {
		export_test_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			export_test_values[i] = export_test_options[i].default_val;
		options = export_test_values;
	}

	export_test_filename = options[HA_export_testfile].str_value;
	if (!export_test_filename)
		export_test_filename = "pcb-out.export_test";
	else {
#warning TODO: set some FILE *fp to stdout
	}

	Printexport_test();
}

static int export_test_usage(const char *topic)
{
	fprintf(stderr, "\nexport_test exporter command line arguments:\n\n");
	pcb_hid_usage(export_test_options, sizeof(export_test_options) / sizeof(export_test_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x export_test foo.pcb [export_test_options]\n\n");
	return 0;
}


static void export_test_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(export_test_options, sizeof(export_test_options) / sizeof(export_test_options[0]), export_test_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

pcb_hid_t export_test_hid;

pcb_uninit_t hid_export_test_init()
{
	memset(&export_test_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&export_test_hid);

	export_test_hid.struct_size = sizeof(pcb_hid_t);
	export_test_hid.name = "export_test";
	export_test_hid.description = "Exports a dump of HID calls";
	export_test_hid.exporter = 1;

	export_test_hid.get_export_options = export_test_get_export_options;
	export_test_hid.do_export = export_test_do_export;
	export_test_hid.parse_arguments = export_test_parse_arguments;

	export_test_hid.usage = export_test_usage;

	pcb_hid_register_hid(&export_test_hid);
	return NULL;
}
