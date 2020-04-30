/* Print bill of materials file is which can be used for checking stock
   and purchasing needed materials. */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <genvector/vts0.h>

#include "build_run.h"
#include "board.h"
#include "data.h"
#include <librnd/core/error.h>
#include <librnd/core/pcb-printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/hid_attrib.h>
#include "hid_cam.h"
#include <librnd/core/hid_init.h>

const char *bom_cookie = "bom HID";

static rnd_export_opt_t bom_options[] = {
/* %start-doc options "8 BOM Creation"
@ftable @code
@item --bomfile <string>
Name of the BOM output file.
@end ftable
%end-doc
*/
	{"bomfile", "Name of the BOM output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_bomfile 0

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 1

};

#define NUM_OPTIONS (sizeof(bom_options)/sizeof(bom_options[0]))

static rnd_hid_attr_val_t bom_values[NUM_OPTIONS];

static const char *bom_filename;

typedef struct pcb_bom_list_s {
	char *descr;
	char *value;
	int num;
	vts0_t refdes;
	struct pcb_bom_list_s *next;
} pcb_bom_list_t;

static rnd_export_opt_t *bom_get_export_options(rnd_hid_t *hid, int *n)
{
	if ((PCB != NULL) && (bom_options[HA_bomfile].default_val.str == NULL))
		pcb_derive_default_filename(PCB->hidlib.filename, &bom_options[HA_bomfile], ".bom");

	if (n)
		*n = NUM_OPTIONS;
	return bom_options;
}

char *pcb_bom_clean_str(const char *in)
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


static pcb_bom_list_t *bom_insert(char *refdes, char *descr, char *value, pcb_bom_list_t * bom)
{
	pcb_bom_list_t *newlist, *cur, *prev = NULL;

	if (bom == NULL) {
		/* this is the first subcircuit so automatically create an entry */
		if ((newlist = malloc(sizeof(pcb_bom_list_t))) == NULL) {
			fprintf(stderr, "malloc() failed in bom_insert()\n");
			exit(1);
		}

		newlist->next = NULL;
		newlist->descr = rnd_strdup(descr);
		newlist->value = rnd_strdup(value);
		newlist->num = 1;
		vts0_init(&newlist->refdes);
		vts0_append(&newlist->refdes, rnd_strdup(refdes));
		return newlist;
	}

	/* search and see if we already have used one of these components */
	cur = bom;
	while(cur != NULL) {
		if ((RND_NSTRCMP(descr, cur->descr) == 0) && (RND_NSTRCMP(value, cur->value) == 0)) {
			cur->num++;
			vts0_append(&cur->refdes, rnd_strdup(refdes));
			break;
		}
		prev = cur;
		cur = cur->next;
	}

	if (cur == NULL) {
		if ((newlist = malloc(sizeof(pcb_bom_list_t))) == NULL) {
			fprintf(stderr, "malloc() failed in bom_insert()\n");
			exit(1);
		}

		prev->next = newlist;

		newlist->next = NULL;
		newlist->descr = rnd_strdup(descr);
		newlist->value = rnd_strdup(value);
		newlist->num = 1;
		vts0_init(&newlist->refdes);
		vts0_append(&newlist->refdes, rnd_strdup(refdes));
	}

	return bom;
}

/* If fp is not NULL then print out the bill of materials contained in
   bom. Either way, free all memory which has been allocated for bom. */
static void print_and_free(FILE * fp, pcb_bom_list_t *bom)
{
	pcb_bom_list_t *lastb;
	char *descr, *value;
	long n;

	while (bom != NULL) {
		if (fp) {
			descr = pcb_bom_clean_str(bom->descr);
			value = pcb_bom_clean_str(bom->value);
			fprintf(fp, "%d,\"%s\",\"%s\",", bom->num, descr, value);
			free(descr);
			free(value);
		}

		for(n = 0; n < bom->refdes.used; n++) {
			char *refdes = bom->refdes.array[n];
			if (fp)
				fprintf(fp, "%s ", refdes);
			free(refdes);
		}
		vts0_uninit(&bom->refdes);
		if (fp)
			fprintf(fp, "\n");
		lastb = bom;
		bom = bom->next;
		free(lastb);
	}
}

static int bom_print(void)
{
	char utcTime[64];
	FILE *fp;
	pcb_bom_list_t *bom = NULL;

	rnd_print_utc(utcTime, sizeof(utcTime), 0);

	PCB_SUBC_LOOP(PCB->Data);
	{
		/* insert this component into the bill of materials list */
		bom = bom_insert((char *) RND_UNKNOWN(subc->refdes),
			(char *) RND_UNKNOWN(pcb_subc_name(subc, "bom::footprint")),
			(char *) RND_UNKNOWN(rnd_attribute_get(&subc->Attributes, "value")),
			bom);
	}
	PCB_END_LOOP;

	fp = pcb_fopen_askovr(&PCB->hidlib, bom_filename, "w", NULL);
	if (!fp) {
		rnd_message(RND_MSG_ERROR, "Cannot open file %s for writing\n", bom_filename);
		print_and_free(NULL, bom);
		return 1;
	}

	fprintf(fp, "# $Id");
	fprintf(fp, "$\n");
	fprintf(fp, "# PcbBOM Version 1.0\n");
	fprintf(fp, "# Date: %s\n", utcTime);
	fprintf(fp, "# Author: %s\n", pcb_author());
	fprintf(fp, "# Title: %s - PCB BOM\n", RND_UNKNOWN(PCB->hidlib.name));
	fprintf(fp, "# Quantity, Description, Value, RefDes\n");
	fprintf(fp, "# --------------------------------------------\n");

	print_and_free(fp, bom);

	fclose(fp);

	return 0;
}

static void bom_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	int i;
	pcb_cam_t cam;

	if (!options) {
		bom_get_export_options(hid, 0);
		for (i = 0; i < NUM_OPTIONS; i++)
			bom_values[i] = bom_options[i].default_val;
		options = bom_values;
	}

	bom_filename = options[HA_bomfile].str;
	if (!bom_filename)
		bom_filename = "pcb-out.bom";

	pcb_cam_begin_nolayer(PCB, &cam, NULL, options[HA_cam].str, &bom_filename);

	bom_print();
	pcb_cam_end(&cam);
}

static int bom_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nbom exporter command line arguments:\n\n");
	rnd_hid_usage(bom_options, sizeof(bom_options) / sizeof(bom_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x bom [bom_options] foo.pcb\n\n");
	return 0;
}


static int bom_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts(bom_options, sizeof(bom_options) / sizeof(bom_options[0]), bom_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

rnd_hid_t bom_hid;

int pplg_check_ver_export_bom(int ver_needed) { return 0; }

void pplg_uninit_export_bom(void)
{
	rnd_export_remove_opts_by_cookie(bom_cookie);
}

int pplg_init_export_bom(void)
{
	PCB_API_CHK_VER;

	memset(&bom_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&bom_hid);

	bom_hid.struct_size = sizeof(rnd_hid_t);
	bom_hid.name = "bom";
	bom_hid.description = "Exports a Bill of Materials";
	bom_hid.exporter = 1;

	bom_hid.get_export_options = bom_get_export_options;
	bom_hid.do_export = bom_do_export;
	bom_hid.parse_arguments = bom_parse_arguments;

	bom_hid.usage = bom_usage;

	rnd_hid_register_hid(&bom_hid);
	return 0;
}
