#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <time.h>

#include "math_helper.h"
#include "board.h"
#include "config.h"
#include "data.h"
#include "error.h"
#include "draw.h"
#include "layer.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "hid_helper.h"
#include "compat_misc.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_draw_helpers.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_flags.h"
#include "conf_core.h"

const char *gerber_cookie = "gerber HID";

#define CRASH(func) fprintf(stderr, "HID error: pcb called unimplemented Gerber function %s.\n", func); abort()

/*----------------------------------------------------------------------------*/
/* Function prototypes                                                        */
/*----------------------------------------------------------------------------*/

static pcb_hid_attribute_t *gerber_get_export_options(int *n);
static void gerber_do_export(pcb_hid_attr_val_t * options);
static void gerber_parse_arguments(int *argc, char ***argv);
static int gerber_set_layer(const char *name, int group, int empty);
static pcb_hid_gc_t gerber_make_gc(void);
static void gerber_destroy_gc(pcb_hid_gc_t gc);
static void gerber_use_mask(int use_it);
static void gerber_set_color(pcb_hid_gc_t gc, const char *name);
static void gerber_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style);
static void gerber_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width);
static void gerber_set_draw_xor(pcb_hid_gc_t gc, int _xor);
static void gerber_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
static void gerber_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle);
static void gerber_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
static void gerber_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius);
static void gerber_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
static void gerber_calibrate(double xval, double yval);
static void gerber_set_crosshair(int x, int y, int action);
static void gerber_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y);

/*----------------------------------------------------------------------------*/
/* Utility routines                                                           */
/*----------------------------------------------------------------------------*/

/* These are for films */
#define gerberX(pcb, x) ((pcb_coord_t) (x))
#define gerberY(pcb, y) ((pcb_coord_t) ((pcb)->MaxHeight - (y)))
#define gerberXOffset(pcb, x) ((pcb_coord_t) (x))
#define gerberYOffset(pcb, y) ((pcb_coord_t) (-(y)))

/* These are for drills (printed as mils but are really 1/10th mil) */
#define gerberDrX(pcb, x) ((pcb_coord_t) (x) * 10)
#define gerberDrY(pcb, y) ((pcb_coord_t) ((pcb)->MaxHeight - (y)) * 10)

/*----------------------------------------------------------------------------*/
/* Private data structures                                                    */
/*----------------------------------------------------------------------------*/

static int verbose;
static int all_layers;
static int is_mask, was_drill;
static int is_drill;
static int current_mask;
static int flash_drills;
static int copy_outline_mode;
static int name_style;
static pcb_layer_t *outline_layer;

enum ApertureShape {
	ROUND,												/* Shaped like a circle */
	OCTAGON,											/* octagonal shape */
	SQUARE,												/* Shaped like a square */
	ROUNDCLEAR,										/* clearance in negatives */
	SQUARECLEAR,
	THERMAL												/* negative thermal relief */
};
typedef enum ApertureShape ApertureShape;

/* This is added to the global aperture array indexes to get gerber
   dcode and macro numbers.  */
#define DCODE_BASE 11

typedef struct aperture {
	int dCode;										/* The RS-274X D code */
	pcb_coord_t width;									/* Size in pcb units */
	ApertureShape shape;					/* ROUND/SQUARE etc */
	struct aperture *next;
} Aperture;

typedef struct {
	Aperture *data;
	int count;
} ApertureList;

static ApertureList *layer_aptr_list;
static ApertureList *curr_aptr_list;
static int layer_list_max;
static int layer_list_idx;

typedef struct {
	pcb_coord_t diam;
	pcb_coord_t x;
	pcb_coord_t y;
} PendingDrills;
PendingDrills *pending_drills = NULL;
int n_pending_drills = 0, max_pending_drills = 0;

/*----------------------------------------------------------------------------*/
/* Defined Constants                                                          */
/*----------------------------------------------------------------------------*/
#define AUTO_OUTLINE_WIDTH PCB_MIL_TO_COORD(8)	/* Auto-geneated outline width of 8 mils */

/*----------------------------------------------------------------------------*/
/* Aperture Routines                                                          */
/*----------------------------------------------------------------------------*/

/* Initialize aperture list */
static void initApertureList(ApertureList * list)
{
	list->data = NULL;
	list->count = 0;
}

static void deinitApertureList(ApertureList * list)
{
	Aperture *search = list->data;
	Aperture *next;
	while (search) {
		next = search->next;
		free(search);
		search = next;
	}
	initApertureList(list);
}

static void resetApertures()
{
	int i;
	for (i = 0; i < layer_list_max; ++i)
		deinitApertureList(&layer_aptr_list[i]);
	free(layer_aptr_list);
	layer_aptr_list = NULL;
	curr_aptr_list = NULL;
	layer_list_max = 0;
	layer_list_idx = 0;
}

/* Create and add a new aperture to the list */
static Aperture *addAperture(ApertureList * list, pcb_coord_t width, ApertureShape shape)
{
	static int aperture_count;

	Aperture *app = (Aperture *) malloc(sizeof *app);
	if (app == NULL)
		return NULL;

	app->width = width;
	app->shape = shape;
	app->dCode = DCODE_BASE + aperture_count++;
	app->next = list->data;

	list->data = app;
	++list->count;

	return app;
}

/* Fetch an aperture from the list with the specified
 *  width/shape, creating a new one if none exists */
static Aperture *findAperture(ApertureList * list, pcb_coord_t width, ApertureShape shape)
{
	Aperture *search;

	/* we never draw zero-width lines */
	if (width == 0)
		return NULL;

	/* Search for an appropriate aperture. */
	for (search = list->data; search; search = search->next)
		if (search->width == width && search->shape == shape)
			return search;

	/* Failing that, create a new one */
	return addAperture(list, width, shape);
}

/* Output aperture data to the file */
static void fprintAperture(FILE * f, Aperture * aptr)
{
	switch (aptr->shape) {
	case ROUND:
		pcb_fprintf(f, "%%ADD%dC,%.4mi*%%\r\n", aptr->dCode, aptr->width);
		break;
	case SQUARE:
		pcb_fprintf(f, "%%ADD%dR,%.4miX%.4mi*%%\r\n", aptr->dCode, aptr->width, aptr->width);
		break;
	case OCTAGON:
		pcb_fprintf(f, "%%AMOCT%d*5,0,8,0,0,%.4mi,22.5*%%\r\n"
								"%%ADD%dOCT%d*%%\r\n", aptr->dCode, (pcb_coord_t) ((double) aptr->width / PCB_COS_22_5_DEGREE), aptr->dCode, aptr->dCode);
		break;
#if 0
	case THERMAL:
		fprintf(f, "%%AMTHERM%d*7,0,0,%.4f,%.4f,%.4f,45*%%\r\n"
						"%%ADD%dTHERM%d*%%\r\n", dCode, gap / 100000.0, width / 100000.0, finger / 100000.0, dCode, dCode);
		break;
	case ROUNDCLEAR:
		fprintf(f, "%%ADD%dC,%.4fX%.4f*%%\r\n", dCode, gap / 100000.0, width / 100000.0);
		break;
	case SQUARECLEAR:
		fprintf(f, "%%ADD%dR,%.4fX%.4fX%.4fX%.4f*%%\r\n",
						dCode, gap / 100000.0, gap / 100000.0, width / 100000.0, width / 100000.0);
		break;
#else
	default:
		break;
#endif
	}
}

/* Set the aperture list for the current layer,
 * expanding the list buffer if needed  */
static ApertureList *setLayerApertureList(int layer_idx)
{
	if (layer_idx >= layer_list_max) {
		int i = layer_list_max;
		layer_list_max = 2 * (layer_idx + 1);
		layer_aptr_list = (ApertureList *)
			realloc(layer_aptr_list, layer_list_max * sizeof(*layer_aptr_list));
		for (; i < layer_list_max; ++i)
			initApertureList(&layer_aptr_list[i]);
	}
	curr_aptr_list = &layer_aptr_list[layer_idx];
	return curr_aptr_list;
}

/* --------------------------------------------------------------------------- */

static pcb_hid_t gerber_hid;

typedef struct hid_gc_s {
	pcb_cap_style_t cap;
	int width;
	int color;
	int erase;
	int drill;
} hid_gc_s;

static FILE *f = NULL;
static char *filename = NULL;
static char *filesuff = NULL;
static char *layername = NULL;
static int lncount = 0;

static int finding_apertures = 0;
static int pagecount = 0;
static int linewidth = -1;
static int lastgroup = -1;
static int lastcap = -1;
static int lastcolor = -1;
static int print_group[PCB_MAX_LAYERGRP];
static int print_layer[PCB_MAX_LAYER];
static int lastX, lastY;				/* the last X and Y coordinate */

static const char *copy_outline_names[] = {
#define COPY_OUTLINE_NONE 0
	"none",
#define COPY_OUTLINE_MASK 1
	"mask",
#define COPY_OUTLINE_SILK 2
	"silk",
#define COPY_OUTLINE_ALL 3
	"all",
	NULL
};

static const char *name_style_names[] = {
#define NAME_STYLE_FIXED 0
	"fixed",
#define NAME_STYLE_SINGLE 1
	"single",
#define NAME_STYLE_FIRST 2
	"first",
#define NAME_STYLE_EAGLE 3
	"eagle",
	NULL
};

static pcb_hid_attribute_t gerber_options[] = {

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --gerberfile <string>
Gerber output file prefix. Can include a path.
@end ftable
%end-doc
*/
	{"gerberfile", "Gerber output file base",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_gerberfile 0

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --all-layers
Output contains all layers, even empty ones.
@end ftable
%end-doc
*/
	{"all-layers", "Output all layers, even empty ones",
	 HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_all_layers 1

/* %start-doc options "90 Gerber Export"
@ftable @code
@item --verbose
Print file names and aperture counts on stdout.
@end ftable
%end-doc
*/
	{"verbose", "Print file names and aperture counts on stdout",
	 HID_Boolean, 0, 0, {0, 0, 0}, 0, 0},
#define HA_verbose 2
	{"copy-outline", "Copy outline onto other layers",
	 HID_Enum, 0, 0, {0, 0, 0}, copy_outline_names, 0},
#define HA_copy_outline 3
	{"name-style", "Naming style for individual gerber files",
	 HID_Enum, 0, 0, {0, 0, 0}, name_style_names, 0},
#define HA_name_style 4
};

#define NUM_OPTIONS (sizeof(gerber_options)/sizeof(gerber_options[0]))

static pcb_hid_attr_val_t gerber_values[NUM_OPTIONS];

static pcb_hid_attribute_t *gerber_get_export_options(int *n)
{
	static char *last_made_filename = NULL;
	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &gerber_options[HA_gerberfile], "", &last_made_filename);

	if (n)
		*n = NUM_OPTIONS;
	return gerber_options;
}

static int group_for_layer(int l)
{
	if (l < pcb_max_copper_layer + 2 && l >= 0)
		return pcb_layer_get_group(l);
	/* else something unique */
	return pcb_max_group + 3 + l;
}

static int layer_sort(const void *va, const void *vb)
{
	int a = *(int *) va;
	int b = *(int *) vb;
	int d = group_for_layer(b) - group_for_layer(a);
	if (d)
		return d;
	return b - a;
}

static void maybe_close_f(FILE * f)
{
	if (f) {
		if (was_drill)
			fprintf(f, "M30\r\n");
		else
			fprintf(f, "M02*\r\n");
		fclose(f);
	}
}

static pcb_box_t region;

/* Very similar to pcb_layer_type_to_file_name() but appends only a
   three-character suffix compatible with Eagle's defaults.  */
static void assign_eagle_file_suffix(char *dest, int idx)
{
	int group;
	int nlayers;
	const char *suff = "out";

	switch (idx) {
	case SL(SILK, TOP):
		suff = "plc";
		break;
	case SL(SILK, BOTTOM):
		suff = "pls";
		break;
	case SL(MASK, TOP):
		suff = "stc";
		break;
	case SL(MASK, BOTTOM):
		suff = "sts";
		break;
	case SL(PDRILL, 0):
		suff = "drd";
		break;
	case SL(UDRILL, 0):
		suff = "dru";
		break;
	case SL(PASTE, TOP):
		suff = "crc";
		break;
	case SL(PASTE, BOTTOM):
		suff = "crs";
		break;
	case SL(INVISIBLE, 0):
		suff = "inv";
		break;
	case SL(FAB, 0):
		suff = "fab";
		break;
	case SL(ASSY, TOP):
		suff = "ast";
		break;
	case SL(ASSY, BOTTOM):
		suff = "asb";
		break;

	default:
		group = pcb_layer_get_group(idx);
		nlayers = PCB->LayerGroups.Number[group];
		if (group == pcb_layer_get_group(pcb_component_silk_layer)) {
			suff = "cmp";
		}
		else if (group == pcb_layer_get_group(pcb_solder_silk_layer)) {
			suff = "sol";
		}
		else if (nlayers == 1
						 && (strcmp(PCB->Data->Layer[idx].Name, "route") == 0 || strcmp(PCB->Data->Layer[idx].Name, "outline") == 0)) {
			suff = "oln";
		}
		else {
			static char buf[20];
			sprintf(buf, "ly%d", group);
			suff = buf;
		}
		break;
	}

	strcpy(dest, suff);
}

static void assign_file_suffix(char *dest, int idx)
{
	int fns_style;
	const char *sext = ".gbr";

	switch (name_style) {
	default:
	case NAME_STYLE_FIXED:
		fns_style = PCB_FNS_fixed;
		break;
	case NAME_STYLE_SINGLE:
		fns_style = PCB_FNS_single;
		break;
	case NAME_STYLE_FIRST:
		fns_style = PCB_FNS_first;
		break;
	case NAME_STYLE_EAGLE:
		assign_eagle_file_suffix(dest, idx);
		return;
	}

	switch (idx) {
	case SL(PDRILL, 0):
		sext = ".cnc";
		break;
	case SL(UDRILL, 0):
		sext = ".cnc";
		break;
	}

	strcpy(dest, pcb_layer_type_to_file_name(idx, fns_style));
	strcat(dest, sext);
}

static void gerber_do_export(pcb_hid_attr_val_t * options)
{
	const char *fnbase;
	int i;
	static int saved_layer_stack[PCB_MAX_LAYER];
	int save_ons[PCB_MAX_LAYER + 2];

	conf_force_set_bool(conf_core.editor.thin_draw, 0);
	conf_force_set_bool(conf_core.editor.thin_draw_poly, 0);
	conf_force_set_bool(conf_core.editor.check_planes, 0);

	if (!options) {
		gerber_get_export_options(NULL);
		for (i = 0; i < NUM_OPTIONS; i++)
			gerber_values[i] = gerber_options[i].default_val;
		options = gerber_values;
	}

	fnbase = options[HA_gerberfile].str_value;
	if (!fnbase)
		fnbase = "pcb-out";

	verbose = options[HA_verbose].int_value;
	all_layers = options[HA_all_layers].int_value;

	copy_outline_mode = options[HA_copy_outline].int_value;
	name_style = options[HA_name_style].int_value;

	outline_layer = NULL;

	for (i = 0; i < pcb_max_copper_layer; i++) {
		pcb_layer_t *layer = PCB->Data->Layer + i;
		if (strcmp(layer->Name, "outline") == 0 || strcmp(layer->Name, "route") == 0) {
			outline_layer = layer;
		}
	}

	i = strlen(fnbase);
	filename = (char *) realloc(filename, i + 40);
	strcpy(filename, fnbase);
	strcat(filename, ".");
	filesuff = filename + strlen(filename);

	if (all_layers) {
		memset(print_group, 1, sizeof(print_group));
		memset(print_layer, 1, sizeof(print_layer));
	}
	else {
		memset(print_group, 0, sizeof(print_group));
		memset(print_layer, 0, sizeof(print_layer));
	}

	pcb_hid_save_and_show_layer_ons(save_ons);
	for (i = 0; i < pcb_max_copper_layer; i++) {
		pcb_layer_t *layer = PCB->Data->Layer + i;
		if (!PCB_LAYER_IS_EMPTY(layer))
			print_group[pcb_layer_get_group(i)] = 1;
	}
	print_group[pcb_layer_get_group(pcb_solder_silk_layer)] = 1;
	print_group[pcb_layer_get_group(pcb_component_silk_layer)] = 1;
	for (i = 0; i < pcb_max_copper_layer; i++)
		if (print_group[pcb_layer_get_group(i)])
			print_layer[i] = 1;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));
	qsort(pcb_layer_stack, pcb_max_copper_layer, sizeof(pcb_layer_stack[0]), layer_sort);
	linewidth = -1;
	lastcap = -1;
	lastgroup = -1;
	lastcolor = -1;

	region.X1 = 0;
	region.Y1 = 0;
	region.X2 = PCB->MaxWidth;
	region.Y2 = PCB->MaxHeight;

	pagecount = 1;
	resetApertures();

	lastgroup = -1;
	layer_list_idx = 0;
	finding_apertures = 1;
	pcb_hid_expose_callback(&gerber_hid, &region, 0);

	layer_list_idx = 0;
	finding_apertures = 0;
	pcb_hid_expose_callback(&gerber_hid, &region, 0);

	memcpy(pcb_layer_stack, saved_layer_stack, sizeof(pcb_layer_stack));

	maybe_close_f(f);
	f = NULL;
	pcb_hid_restore_layer_ons(save_ons);
	conf_update(NULL); /* resotre forced sets */
}

static void gerber_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(gerber_options, NUM_OPTIONS, gerber_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

static int drill_sort(const void *va, const void *vb)
{
	PendingDrills *a = (PendingDrills *) va;
	PendingDrills *b = (PendingDrills *) vb;
	if (a->diam != b->diam)
		return a->diam - b->diam;
	if (a->x != b->x)
		return a->x - a->x;
	return b->y - b->y;
}

static int gerber_set_layer(const char *name, int group, int empty)
{
	int want_outline;
	char *cp;
	int idx = (group >= 0 && group < pcb_max_group) ? PCB->LayerGroups.Entries[group][0] : group;

	if (name == NULL)
		name = PCB->Data->Layer[idx].Name;

	if (idx >= 0 && idx < pcb_max_copper_layer && !print_layer[idx])
		return 0;

	if (strcmp(name, "invisible") == 0)
		return 0;
	if (SL_TYPE(idx) == SL_ASSY)
		return 0;

	flash_drills = 0;
	if (strcmp(name, "outline") == 0 || strcmp(name, "route") == 0)
		flash_drills = 1;

	if (is_drill && n_pending_drills) {
		int i;
		/* dump pending drills in sequence */
		qsort(pending_drills, n_pending_drills, sizeof(pending_drills[0]), drill_sort);
		for (i = 0; i < n_pending_drills; i++) {
			if (i == 0 || pending_drills[i].diam != pending_drills[i - 1].diam) {
				Aperture *ap = findAperture(curr_aptr_list, pending_drills[i].diam, ROUND);
				fprintf(f, "T%02d\r\n", ap->dCode);
			}
			pcb_fprintf(f, "X%06.0mlY%06.0ml\r\n", gerberDrX(PCB, pending_drills[i].x), gerberDrY(PCB, pending_drills[i].y));
		}
		free(pending_drills);
		n_pending_drills = max_pending_drills = 0;
		pending_drills = NULL;
	}

	is_drill = (SL_TYPE(idx) == SL_PDRILL || SL_TYPE(idx) == SL_UDRILL);
	is_mask = (SL_TYPE(idx) == SL_MASK);
	current_mask = 0;
#if 0
	printf("Layer %s group %d drill %d mask %d\n", name, group, is_drill, is_mask);
#endif

	if (group < 0 || group != lastgroup) {
		time_t currenttime;
		char utcTime[64];
		ApertureList *aptr_list;
		Aperture *search;

		lastgroup = group;
		lastX = -1;
		lastY = -1;
		lastcolor = 0;
		linewidth = -1;
		lastcap = -1;

		aptr_list = setLayerApertureList(layer_list_idx++);

		if (finding_apertures)
			goto emit_outline;

		if (aptr_list->count == 0 && !all_layers)
			return 0;

		maybe_close_f(f);
		f = NULL;

		pagecount++;
		assign_file_suffix(filesuff, idx);
		f = fopen(filename, "wb");	/* Binary needed to force CR-LF */
		if (f == NULL) {
			pcb_message(PCB_MSG_DEFAULT, "Error:  Could not open %s for writing.\n", filename);
			return 1;
		}

		was_drill = is_drill;

		if (verbose) {
			int c = aptr_list->count;
			printf("Gerber: %d aperture%s in %s\n", c, c == 1 ? "" : "s", filename);
		}

		if (is_drill) {
			/* We omit the ,TZ here because we are not omitting trailing zeros.  Our format is
			   always six-digit 0.1 mil resolution (i.e. 001100 = 0.11") */
			fprintf(f, "M48\r\n" "INCH\r\n");
			for (search = aptr_list->data; search; search = search->next)
				pcb_fprintf(f, "T%02dC%.3mi\r\n", search->dCode, search->width);
			fprintf(f, "%%\r\n");
			/* FIXME */
			return 1;
		}

		fprintf(f, "G04 start of page %d for group %d idx %d *\r\n", pagecount, group, idx);

		/* Create a portable timestamp. */
		currenttime = time(NULL);
		{
			/* avoid gcc complaints */
			const char *fmt = "%c UTC";
			strftime(utcTime, sizeof utcTime, fmt, gmtime(&currenttime));
		}
		/* Print a cute file header at the beginning of each file. */
		fprintf(f, "G04 Title: %s, %s *\r\n", PCB_UNKNOWN(PCB->Name), PCB_UNKNOWN(name));
		fprintf(f, "G04 Creator: pcb-rnd " VERSION " *\r\n");
		fprintf(f, "G04 CreationDate: %s *\r\n", utcTime);

		/* ID the user. */
		fprintf(f, "G04 For: %s *\r\n", pcb_get_user_name());

		fprintf(f, "G04 Format: Gerber/RS-274X *\r\n");
		pcb_fprintf(f, "G04 PCB-Dimensions: %.0mc %.0mc *\r\n", PCB->MaxWidth, PCB->MaxHeight);
		fprintf(f, "G04 PCB-Coordinate-Origin: lower left *\r\n");

		/* Signal data in inches. */
		fprintf(f, "%%MOIN*%%\r\n");

		/* Signal Leading zero suppression, Absolute Data, 2.5 format */
		fprintf(f, "%%FSLAX25Y25*%%\r\n");

		/* build a legal identifier. */
		if (layername)
			free(layername);
		layername = pcb_strdup(filesuff);
		if (strrchr(layername, '.'))
			*strrchr(layername, '.') = 0;

		for (cp = layername; *cp; cp++) {
			if (isalnum((int) *cp))
				*cp = toupper((int) *cp);
			else
				*cp = '_';
		}
		fprintf(f, "%%LN%s*%%\r\n", layername);
		lncount = 1;

		for (search = aptr_list->data; search; search = search->next)
			fprintAperture(f, search);
		if (aptr_list->count == 0)
			/* We need to put *something* in the file to make it be parsed
			   as RS-274X instead of RS-274D. */
			fprintf(f, "%%ADD11C,0.0100*%%\r\n");
	}

emit_outline:
	/* If we're printing a copper layer other than the outline layer,
	   and we want to "print outlines", and we have an outline layer,
	   print the outline layer on this layer also.  */
	want_outline = 0;
	if (copy_outline_mode == COPY_OUTLINE_MASK && SL_TYPE(idx) == SL_MASK)
		want_outline = 1;
	if (copy_outline_mode == COPY_OUTLINE_SILK && SL_TYPE(idx) == SL_SILK)
		want_outline = 1;
	if (copy_outline_mode == COPY_OUTLINE_ALL
			&& (SL_TYPE(idx) == SL_SILK
					|| SL_TYPE(idx) == SL_MASK || SL_TYPE(idx) == SL_FAB || SL_TYPE(idx) == SL_ASSY || SL_TYPE(idx) == 0))
		want_outline = 1;

	if (want_outline && strcmp(name, "outline")
			&& strcmp(name, "route")) {
		if (outline_layer && outline_layer != PCB->Data->Layer + idx)
			pcb_draw_layer(outline_layer, &region);
		else if (!outline_layer) {
			pcb_hid_gc_t gc = pcb_gui->make_gc();
			printf("name %s idx %d\n", name, idx);
			if (SL_TYPE(idx) == SL_SILK)
				pcb_gui->set_line_width(gc, PCB->minSlk);
			else if (group >= 0)
				pcb_gui->set_line_width(gc, PCB->minWid);
			else
				pcb_gui->set_line_width(gc, AUTO_OUTLINE_WIDTH);
			pcb_gui->draw_line(gc, 0, 0, PCB->MaxWidth, 0);
			pcb_gui->draw_line(gc, 0, 0, 0, PCB->MaxHeight);
			pcb_gui->draw_line(gc, PCB->MaxWidth, 0, PCB->MaxWidth, PCB->MaxHeight);
			pcb_gui->draw_line(gc, 0, PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight);
			pcb_gui->destroy_gc(gc);
		}
	}

	return 1;
}

static pcb_hid_gc_t gerber_make_gc(void)
{
	pcb_hid_gc_t rv = (pcb_hid_gc_t) calloc(1, sizeof(*rv));
	rv->cap = Trace_Cap;
	return rv;
}

static void gerber_destroy_gc(pcb_hid_gc_t gc)
{
	free(gc);
}

static void gerber_use_mask(int use_it)
{
	current_mask = use_it;
}

static void gerber_set_color(pcb_hid_gc_t gc, const char *name)
{
	if (strcmp(name, "erase") == 0) {
		gc->color = 1;
		gc->erase = 1;
		gc->drill = 0;
	}
	else if (strcmp(name, "drill") == 0) {
		gc->color = 1;
		gc->erase = 0;
		gc->drill = 1;
	}
	else {
		gc->color = 0;
		gc->erase = 0;
		gc->drill = 0;
	}
}

static void gerber_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

static void gerber_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}

static void gerber_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
	;
}

static void use_gc(pcb_hid_gc_t gc, int radius)
{
	if (radius) {
		radius *= 2;
		if (radius != linewidth || lastcap != Round_Cap) {
			Aperture *aptr = findAperture(curr_aptr_list, radius, ROUND);
			if (aptr == NULL)
				pcb_fprintf(stderr, "error: aperture for radius %$mS type ROUND is null\n", radius);
			else if (f && !is_drill)
				fprintf(f, "G54D%d*", aptr->dCode);
			linewidth = radius;
			lastcap = Round_Cap;
		}
	}
	else if (linewidth != gc->width || lastcap != gc->cap) {
		Aperture *aptr;
		ApertureShape shape;

		linewidth = gc->width;
		lastcap = gc->cap;
		switch (gc->cap) {
		case Round_Cap:
		case Trace_Cap:
			shape = ROUND;
			break;
		default:
		case Square_Cap:
			shape = SQUARE;
			break;
		}
		aptr = findAperture(curr_aptr_list, linewidth, shape);
		if (aptr == NULL)
			pcb_fprintf(stderr, "error: aperture for width %$mS type %s is null\n", linewidth, shape == ROUND ? "ROUND" : "SQUARE");
		if (f)
			fprintf(f, "G54D%d*", aptr->dCode);
	}
#if 0
	if (lastcolor != gc->color) {
		c = gc->color;
		if (is_drill)
			return;
		if (is_mask)
			c = (gc->erase ? 0 : 1);
		lastcolor = gc->color;
		if (f) {
			if (c) {
				fprintf(f, "%%LN%s_C%d*%%\r\n", layername, lncount++);
				fprintf(f, "%%LPC*%%\r\n");
			}
			else {
				fprintf(f, "%%LN%s_D%d*%%\r\n", layername, lncount++);
				fprintf(f, "%%LPD*%%\r\n");
			}
		}
	}
#endif
}

static void gerber_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	gerber_draw_line(gc, x1, y1, x1, y2);
	gerber_draw_line(gc, x1, y1, x2, y1);
	gerber_draw_line(gc, x1, y2, x2, y2);
	gerber_draw_line(gc, x2, y1, x2, y2);
}

static void gerber_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	pcb_bool m = pcb_false;

	if (x1 != x2 && y1 != y2 && gc->cap == Square_Cap) {
		pcb_coord_t x[5], y[5];
		double tx, ty, theta;

		theta = atan2(y2 - y1, x2 - x1);

		/* T is a vector half a thickness long, in the direction of
		   one of the corners.  */
		tx = gc->width / 2.0 * cos(theta + M_PI / 4) * sqrt(2.0);
		ty = gc->width / 2.0 * sin(theta + M_PI / 4) * sqrt(2.0);

		x[0] = x1 - tx;
		y[0] = y1 - ty;
		x[1] = x2 + ty;
		y[1] = y2 - tx;
		x[2] = x2 + tx;
		y[2] = y2 + ty;
		x[3] = x1 - ty;
		y[3] = y1 + tx;

		x[4] = x[0];
		y[4] = y[0];
		gerber_fill_polygon(gc, 5, x, y);
		return;
	}

	use_gc(gc, 0);
	if (!f)
		return;

	if (x1 != lastX) {
		m = pcb_true;
		lastX = x1;
		pcb_fprintf(f, "X%.0mc", gerberX(PCB, lastX));
	}
	if (y1 != lastY) {
		m = pcb_true;
		lastY = y1;
		pcb_fprintf(f, "Y%.0mc", gerberY(PCB, lastY));
	}
	if ((x1 == x2) && (y1 == y2))
		fprintf(f, "D03*\r\n");
	else {
		if (m)
			fprintf(f, "D02*");
		if (x2 != lastX) {
			lastX = x2;
			pcb_fprintf(f, "X%.0mc", gerberX(PCB, lastX));
		}
		if (y2 != lastY) {
			lastY = y2;
			pcb_fprintf(f, "Y%.0mc", gerberY(PCB, lastY));

		}
		fprintf(f, "D01*\r\n");
	}

}

static void gerber_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	pcb_bool m = pcb_false;
	double arcStartX, arcStopX, arcStartY, arcStopY;

	/* we never draw zero-width lines */
	if (gc->width == 0)
		return;

	use_gc(gc, 0);
	if (!f)
		return;

	/* full circle is full.... truncate so that the arc split code never needs to
	   do more than 180 deg */
	if (delta_angle < -360.0)
		delta_angle = -360.0;
	if (delta_angle > +360.0)
		delta_angle = +360.0;


	/* some gerber interpreters (gerbv for one) have hard time dealing with
	   full-circle arcs - split large arcs up into 2 smaller ones */
	if (delta_angle < -180.0) {
		gerber_draw_arc(gc, cx, cy, width, height, start_angle, -180.0);
		gerber_draw_arc(gc, cx, cy, width, height, start_angle-180, delta_angle+180.0);
		return;
	}
	if (delta_angle > +180.0) {
		gerber_draw_arc(gc, cx, cy, width, height, start_angle, 180.0);
		gerber_draw_arc(gc, cx, cy, width, height, start_angle+180, delta_angle-180.0);
		return;
	}


	arcStartX = cx - width * cos(PCB_TO_RADIANS(start_angle));
	arcStartY = cy + height * sin(PCB_TO_RADIANS(start_angle));

	if (fabs(delta_angle) < 0.01) {
		gerber_draw_line(gc, arcStartX, arcStartY, arcStartX, arcStartY);
		return;
	}

	/* I checked three different gerber viewers, and they all disagreed
	   on how ellipses should be drawn.  The spec just calls G74/G75
	   "circular interpolation" so there's a chance it just doesn't
	   support ellipses at all.  Thus, we draw them out with line
	   segments.  Note that most arcs in pcb are circles anyway.  */
	if (width != height) {
		double step, angle;
		pcb_coord_t max = width > height ? width : height;
		pcb_coord_t minr = max - gc->width / 10;
		int nsteps;
		pcb_coord_t x0, y0, x1, y1;

		if (minr >= max)
			minr = max - 1;
		step = acos((double) minr / (double) max) * 180.0 / M_PI;
		if (step > 5)
			step = 5;
		nsteps = fabs(delta_angle) / step + 1;
		step = (double) delta_angle / nsteps;

		x0 = arcStartX;
		y0 = arcStartY;
		angle = start_angle;
		while (nsteps > 0) {
			nsteps--;
			x1 = cx - width * cos(PCB_TO_RADIANS(angle + step));
			y1 = cy + height * sin(PCB_TO_RADIANS(angle + step));
			gerber_draw_line(gc, x0, y0, x1, y1);
			x0 = x1;
			y0 = y1;
			angle += step;
		}
		return;
	}

	arcStopX = cx - width * cos(PCB_TO_RADIANS(start_angle + delta_angle));
	arcStopY = cy + height * sin(PCB_TO_RADIANS(start_angle + delta_angle));
	if (arcStartX != lastX) {
		m = pcb_true;
		lastX = arcStartX;
		pcb_fprintf(f, "X%.0mc", gerberX(PCB, lastX));
	}
	if (arcStartY != lastY) {
		m = pcb_true;
		lastY = arcStartY;
		pcb_fprintf(f, "Y%.0mc", gerberY(PCB, lastY));
	}
	if (m)
		fprintf(f, "D02*");
	pcb_fprintf(f,
							"G75*G0%1dX%.0mcY%.0mcI%.0mcJ%.0mcD01*G01*\r\n",
							(delta_angle < 0) ? 2 : 3,
							gerberX(PCB, arcStopX), gerberY(PCB, arcStopY),
							gerberXOffset(PCB, cx - arcStartX), gerberYOffset(PCB, cy - arcStartY));
	lastX = arcStopX;
	lastY = arcStopY;
}

static void gerber_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	if (radius <= 0)
		return;
	if (is_drill)
		radius = 50 * pcb_round(radius / 50.0);
	use_gc(gc, radius);
	if (!f)
		return;
	if (is_drill) {
		if (n_pending_drills >= max_pending_drills) {
			max_pending_drills += 100;
			pending_drills = (PendingDrills *) realloc(pending_drills, max_pending_drills * sizeof(pending_drills[0]));
		}
		pending_drills[n_pending_drills].x = cx;
		pending_drills[n_pending_drills].y = cy;
		pending_drills[n_pending_drills].diam = radius * 2;
		n_pending_drills++;
		return;
	}
	else if (gc->drill && !flash_drills)
		return;
	if (cx != lastX) {
		lastX = cx;
		pcb_fprintf(f, "X%.0mc", gerberX(PCB, lastX));
	}
	if (cy != lastY) {
		lastY = cy;
		pcb_fprintf(f, "Y%.0mc", gerberY(PCB, lastY));
	}
	fprintf(f, "D03*\r\n");
}

static void gerber_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	pcb_bool m = pcb_false;
	int i;
	int firstTime = 1;
	pcb_coord_t startX = 0, startY = 0;

	if (is_mask && current_mask == HID_MASK_BEFORE)
		return;

	use_gc(gc, 10 * 100);
	if (!f)
		return;
	fprintf(f, "G36*\r\n");
	for (i = 0; i < n_coords; i++) {
		if (x[i] != lastX) {
			m = pcb_true;
			lastX = x[i];
			pcb_fprintf(f, "X%.0mc", gerberX(PCB, lastX));
		}
		if (y[i] != lastY) {
			m = pcb_true;
			lastY = y[i];
			pcb_fprintf(f, "Y%.0mc", gerberY(PCB, lastY));
		}
		if (firstTime) {
			firstTime = 0;
			startX = x[i];
			startY = y[i];
			if (m)
				fprintf(f, "D02*");
		}
		else if (m)
			fprintf(f, "D01*\r\n");
		m = pcb_false;
	}
	if (startX != lastX) {
		m = pcb_true;
		lastX = startX;
		pcb_fprintf(f, "X%.0mc", gerberX(PCB, startX));
	}
	if (startY != lastY) {
		m = pcb_true;
		lastY = startY;
		pcb_fprintf(f, "Y%.0mc", gerberY(PCB, lastY));
	}
	if (m)
		fprintf(f, "D01*\r\n");
	fprintf(f, "G37*\r\n");
}

static void gerber_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	pcb_coord_t x[5];
	pcb_coord_t y[5];
	x[0] = x[4] = x1;
	y[0] = y[4] = y1;
	x[1] = x1;
	y[1] = y2;
	x[2] = x2;
	y[2] = y2;
	x[3] = x2;
	y[3] = y1;
	gerber_fill_polygon(gc, 5, x, y);
}

static void gerber_calibrate(double xval, double yval)
{
	CRASH("gerber_calibrate");
}

static int gerber_usage(const char *topic)
{
	fprintf(stderr, "\ngerber exporter command line arguments:\n\n");
	pcb_hid_usage(gerber_options, sizeof(gerber_options) / sizeof(gerber_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x gerber foo.pcb [gerber options]\n\n");
	return 0;
}

static void gerber_set_crosshair(int x, int y, int action)
{
}

pcb_uninit_t hid_export_gerber_init()
{
	memset(&gerber_hid, 0, sizeof(gerber_hid));

	pcb_hid_nogui_init(&gerber_hid);
	pcb_dhlp_draw_helpers_init(&gerber_hid);

	gerber_hid.struct_size = sizeof(gerber_hid);
	gerber_hid.name = "gerber";
	gerber_hid.description = "RS-274X (Gerber) export";
	gerber_hid.exporter = 1;

	gerber_hid.get_export_options = gerber_get_export_options;
	gerber_hid.do_export = gerber_do_export;
	gerber_hid.parse_arguments = gerber_parse_arguments;
	gerber_hid.set_layer = gerber_set_layer;
	gerber_hid.make_gc = gerber_make_gc;
	gerber_hid.destroy_gc = gerber_destroy_gc;
	gerber_hid.use_mask = gerber_use_mask;
	gerber_hid.set_color = gerber_set_color;
	gerber_hid.set_line_cap = gerber_set_line_cap;
	gerber_hid.set_line_width = gerber_set_line_width;
	gerber_hid.set_draw_xor = gerber_set_draw_xor;
	gerber_hid.draw_line = gerber_draw_line;
	gerber_hid.draw_arc = gerber_draw_arc;
	gerber_hid.draw_rect = gerber_draw_rect;
	gerber_hid.fill_circle = gerber_fill_circle;
	gerber_hid.fill_polygon = gerber_fill_polygon;
	gerber_hid.fill_rect = gerber_fill_rect;
	gerber_hid.calibrate = gerber_calibrate;
	gerber_hid.set_crosshair = gerber_set_crosshair;
	gerber_hid.usage = gerber_usage;

	pcb_hid_register_hid(&gerber_hid);
	return NULL;
}
