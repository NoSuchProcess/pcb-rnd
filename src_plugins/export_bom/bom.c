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
#include "compat_misc.h"
#include "safe_fs.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "hid_init.h"
#include "macro.h"

const char *bom_cookie = "bom HID";

static pcb_hid_attribute_t bom_options[] = {
/* %start-doc options "8 BOM Creation"
@ftable @code
@item --bomfile <string>
Name of the BOM output file.
@end ftable
%end-doc
*/
	{"bomfile", "Name of the BOM output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_bomfile 0
/* %start-doc options "8 BOM Creation"
@ftable @code
@item --xyfile <string>
Name of the XY output file.
@end ftable
%end-doc
*/
};

#define NUM_OPTIONS (sizeof(bom_options)/sizeof(bom_options[0]))

static pcb_hid_attr_val_t bom_values[NUM_OPTIONS];

static const char *bom_filename;

typedef struct _StringList {
	char *str;
	struct _StringList *next;
} StringList;

typedef struct _BomList {
	char *descr;
	char *value;
	int num;
	StringList *refdes;
	struct _BomList *next;
} BomList;

static pcb_hid_attribute_t *bom_get_export_options(int *n)
{
	static char *last_bom_filename = NULL;

	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &bom_options[HA_bomfile], ".bom", &last_bom_filename);

	if (n)
		*n = NUM_OPTIONS;
	return bom_options;
}

/* ---------------------------------------------------------------------------
 * prints a centroid file in a format which includes data needed by a
 * pick and place machine.  Further formatting for a particular factory setup
 * can easily be generated with awk or perl.  In addition, a bill of materials
 * file is generated which can be used for checking stock and purchasing needed
 * materials.
 * returns != zero on error
 */
char *CleanBOMString(const char *in)
{
	char *out;
	int i;

	if ((out = (char *) malloc((strlen(in) + 1) * sizeof(char))) == NULL) {
		fprintf(stderr, "Error:  CleanBOMString() malloc() failed\n");
		exit(1);
	}

	/*
	 * copy over in to out with some character conversions.
	 * Go all the way to then end to get the terminating \0
	 */
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

static StringList *string_insert(char *str, StringList * list)
{
	StringList *newlist, *cur;

	if ((newlist = (StringList *) malloc(sizeof(StringList))) == NULL) {
		fprintf(stderr, "malloc() failed in string_insert()\n");
		exit(1);
	}

	newlist->next = NULL;
	newlist->str = pcb_strdup(str);

	if (list == NULL)
		return newlist;

	cur = list;
	while (cur->next != NULL)
		cur = cur->next;

	cur->next = newlist;

	return list;
}

static BomList *bom_insert(char *refdes, char *descr, char *value, BomList * bom)
{
	BomList *newlist, *cur, *prev = NULL;

	if (bom == NULL) {
		/* this is the first subcircuit so automatically create an entry */
		if ((newlist = (BomList *) malloc(sizeof(BomList))) == NULL) {
			fprintf(stderr, "malloc() failed in bom_insert()\n");
			exit(1);
		}

		newlist->next = NULL;
		newlist->descr = pcb_strdup(descr);
		newlist->value = pcb_strdup(value);
		newlist->num = 1;
		newlist->refdes = string_insert(refdes, NULL);
		return newlist;
	}

	/* search and see if we already have used one of these
	   components */
	cur = bom;
	while (cur != NULL) {
		if ((PCB_NSTRCMP(descr, cur->descr) == 0) && (PCB_NSTRCMP(value, cur->value) == 0)) {
			cur->num++;
			cur->refdes = string_insert(refdes, cur->refdes);
			break;
		}
		prev = cur;
		cur = cur->next;
	}

	if (cur == NULL) {
		if ((newlist = (BomList *) malloc(sizeof(BomList))) == NULL) {
			fprintf(stderr, "malloc() failed in bom_insert()\n");
			exit(1);
		}

		prev->next = newlist;

		newlist->next = NULL;
		newlist->descr = pcb_strdup(descr);
		newlist->value = pcb_strdup(value);
		newlist->num = 1;
		newlist->refdes = string_insert(refdes, NULL);
	}

	return bom;

}

/*
 * If fp is not NULL then print out the bill of materials contained in
 * bom.  Either way, free all memory which has been allocated for bom.
 */
static void print_and_free(FILE * fp, BomList * bom)
{
	BomList *lastb;
	StringList *lasts;
	char *descr, *value;

	while (bom != NULL) {
		if (fp) {
			descr = CleanBOMString(bom->descr);
			value = CleanBOMString(bom->value);
			fprintf(fp, "%d,\"%s\",\"%s\",", bom->num, descr, value);
			free(descr);
			free(value);
		}

		while (bom->refdes != NULL) {
			if (fp) {
				fprintf(fp, "%s ", bom->refdes->str);
			}
			free(bom->refdes->str);
			lasts = bom->refdes;
			bom->refdes = bom->refdes->next;
			free(lasts);
		}
		if (fp) {
			fprintf(fp, "\n");
		}
		lastb = bom;
		bom = bom->next;
		free(lastb);
	}
}

static int PrintBOM(void)
{
	char utcTime[64];
	FILE *fp;
	BomList *bom = NULL;

	pcb_print_utc(utcTime, sizeof(utcTime), 0);

	PCB_SUBC_LOOP(PCB->Data);
	{
		/* insert this component into the bill of materials list */
		bom = bom_insert((char *) PCB_UNKNOWN(subc->refdes),
			(char *) PCB_UNKNOWN(pcb_attribute_get(&subc->Attributes, "footprint")),
			(char *) PCB_UNKNOWN(pcb_attribute_get(&subc->Attributes, "value")),
			bom);
	}
	PCB_END_LOOP;

	fp = pcb_fopen(bom_filename, "w");
	if (!fp) {
		pcb_gui->log("Cannot open file %s for writing\n", bom_filename);
		print_and_free(NULL, bom);
		return 1;
	}

	fprintf(fp, "# $Id");
	fprintf(fp, "$\n");
	fprintf(fp, "# PcbBOM Version 1.0\n");
	fprintf(fp, "# Date: %s\n", utcTime);
	fprintf(fp, "# Author: %s\n", pcb_author());
	fprintf(fp, "# Title: %s - PCB BOM\n", PCB_UNKNOWN(PCB->Name));
	fprintf(fp, "# Quantity, Description, Value, RefDes\n");
	fprintf(fp, "# --------------------------------------------\n");

	print_and_free(fp, bom);

	fclose(fp);

	return 0;
}

static void bom_do_export(pcb_hid_attr_val_t * options)
{
	int i;

	if (!options) {
		bom_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			bom_values[i] = bom_options[i].default_val;
		options = bom_values;
	}

	bom_filename = options[HA_bomfile].str_value;
	if (!bom_filename)
		bom_filename = "pcb-out.bom";

	PrintBOM();
}

static int bom_usage(const char *topic)
{
	fprintf(stderr, "\nbom exporter command line arguments:\n\n");
	pcb_hid_usage(bom_options, sizeof(bom_options) / sizeof(bom_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x bom [bom_options] foo.pcb\n\n");
	return 0;
}


static void bom_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(bom_options, sizeof(bom_options) / sizeof(bom_options[0]), bom_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

pcb_hid_t bom_hid;

int pplg_check_ver_export_bom(int ver_needed) { return 0; }

void pplg_uninit_export_bom(void)
{
	pcb_hid_remove_attributes_by_cookie(bom_cookie);
}

int pplg_init_export_bom(void)
{
	memset(&bom_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&bom_hid);

	bom_hid.struct_size = sizeof(pcb_hid_t);
	bom_hid.name = "bom";
	bom_hid.description = "Exports a Bill of Materials";
	bom_hid.exporter = 1;

	bom_hid.get_export_options = bom_get_export_options;
	bom_hid.do_export = bom_do_export;
	bom_hid.parse_arguments = bom_parse_arguments;

	bom_hid.usage = bom_usage;

	pcb_hid_register_hid(&bom_hid);
	return 0;
}
