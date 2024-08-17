#include "config.h"
#include "conf_core.h"
#include <librnd/core/rnd_conf.h>

#include <stdlib.h>

#include "build_run.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include <librnd/core/error.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>
#include "bom_conf.h"

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>
#include <librnd/hid/hid_attrib.h>
#include "hid_cam.h"
#include <librnd/hid/hid_init.h>

#include "../src_plugins/export_bom/conf_internal.c"

#define CONF_FN "export_bom.conf"

conf_bom_t conf_bom;

const char *bom_cookie = "BOM2 HID";

/* Maximum length of a template name (in the config file, in the enum) */
#define MAX_TEMP_NAME_LEN 128


/* This one can not be const because format enumeration is loaded run-time */
static rnd_export_opt_t bom_options[] = {
	{"bomfile", "Name of the BoM output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_bomfile 0

	{"format", "file format (template)",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, NULL},
#define HA_format 1

	{"part-rnd", "export a part-rnd tEDAx that contains the selected template and all data",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, NULL},
#define HA_part_rnd 2

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_cam 3
};

#define NUM_OPTIONS (sizeof(bom_options)/sizeof(bom_options[0]))

static rnd_hid_attr_val_t bom_values[NUM_OPTIONS];

static const char *bom_filename;

typedef pcb_subc_t bom_obj_t;
#include "../../src_3rd/rnd_inclib/lib_bom/lib_bom.h"

static const rnd_export_opt_t *bom_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	const char *val = bom_values[HA_bomfile].str;

	/* load all formats from the config */
	bom_build_fmts(&conf_bom.plugins.export_bom.templates);

	if (bom_fmt_names.used == 0) {
		rnd_message(RND_MSG_ERROR, "export_bom: can not set up export options: no template available\n");
		return NULL;
	}

	bom_options[HA_format].enumerations = (const char **)bom_fmt_names.array;

	/* set default filename */
	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &bom_values[HA_bomfile], ".bom");

	if (n)
		*n = NUM_OPTIONS;
	return bom_options;
}

static const char *bom_name_prefix(bom_subst_ctx_t *ctx)
{
	static char tmp[256]; /* this is safe: caller will make a copy right after we return */
	char *o = tmp;
	const char *t;
	int n = 0;

	for(t = ctx->name; isalpha(*t) && (n < sizeof(tmp)-1); t++,n++,o++)
		*o = *t;
	*o = '\0';
	return tmp;
}

static const char *subst_user(bom_subst_ctx_t *ctx, const char *key)
{
	if (strcmp(key, "author") == 0) return pcb_author();
	if (strcmp(key, "title") == 0) return RND_UNKNOWN(PCB->hidlib.name);
	if (strncmp(key, "subc.", 5) == 0) {
		key += 5;

		if (strncmp(key, "a.", 2) == 0) return pcb_attribute_get(&ctx->obj->Attributes, key+2);
		else if (strcmp(key, "name") == 0) return ctx->name;
		if (strcmp(key, "prefix") == 0) return bom_name_prefix(ctx);
	}

	return NULL;
}

static void part_rnd_print(bom_subst_ctx_t *ctx, bom_obj_t *obj, const char *name)
{
	if (name == NULL) {
		bom_tdx_fprint_safe_kv(ctx->f, "author", pcb_author());
		bom_tdx_fprint_safe_kv(ctx->f, "title", RND_UNKNOWN(PCB->hidlib.name));
	}
	else {
		int n;

		bom_tdx_fprint_safe_kv(ctx->f, "name", name);
		bom_tdx_fprint_safe_kv(ctx->f, "prefix", bom_name_prefix(ctx));

		/* print all attributes */
		for(n = 0; n < obj->Attributes.Number; n++) {
			pcb_attribute_t *a = obj->Attributes.List + n;
			bom_tdx_fprint_safe_kkv(ctx->f, "a.", a->name, a->value);
		}
	}
}


static int print_bom(const bom_template_t *templ, rnd_hid_attr_val_t *options)
{
	FILE *f;
	bom_subst_ctx_t ctx = {0};

	f = rnd_fopen_askovr(&PCB->hidlib, bom_filename, "w", NULL);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Cannot open file %s for writing\n", bom_filename);
		return 1;
	}

	bom_part_rnd_mode = options[HA_part_rnd].lng ? part_rnd_print : NULL;

	bom_print_begin(&ctx, f, templ);

	/* For each subcircuit calculate an ID and count recurring IDs in a hash table and an array (for sorting) */
	PCB_SUBC_LOOP(PCB->Data);
	{
		if (subc->extobj == NULL) {
			const char *refdes = RND_UNKNOWN(pcb_attribute_get(&subc->Attributes, "refdes"));
			bom_print_add(&ctx, subc, refdes);
		}
	}
	PCB_END_LOOP;

	bom_print_all(&ctx);
	bom_print_end(&ctx);
	fclose(f);

	return 0;
}

static void bom_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	bom_template_t templ = {0};
	char **tid;
	pcb_cam_t cam;

	if (!options) {
		bom_get_export_options(hid, 0, design, appspec);
		options = bom_values;
	}

	bom_filename = options[HA_bomfile].str;
	if (!bom_filename)
		bom_filename = "pcb-rnd-out.bom";

	pcb_cam_begin_nolayer(PCB, &cam, NULL, options[HA_cam].str, &bom_filename);

	tid = vts0_get(&bom_fmt_ids, options[HA_format].lng, 0);
	if ((tid == NULL) || (*tid == NULL)) {
		rnd_message(RND_MSG_ERROR, "export_bom: invalid template selected\n");
		return;
	}

	bom_init_template(&templ, &conf_bom.plugins.export_bom.templates, *tid);
	print_bom(&templ, options);
	pcb_cam_end(&cam);
}

static int bom_usage(rnd_hid_t *hid, const char *topic)
{
	int n;
	fprintf(stderr, "\nBOM exporter command line arguments:\n\n");
	bom_get_export_options(hid, &n, NULL, NULL);
	rnd_hid_usage(bom_options, sizeof(bom_options) / sizeof(bom_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x bom [bom_options] foo.pcb\n\n");
	return 0;
}

static int bom_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, bom_options, sizeof(bom_options) / sizeof(bom_options[0]), bom_cookie, 0);

	/* when called from the export() command this field may be uninitialized yet */
	if (bom_options[HA_format].enumerations == NULL)
		bom_get_export_options(hid, NULL, NULL, NULL);

	return rnd_hid_parse_command_line(argc, argv);
}

#include "../../src_3rd/rnd_inclib/lib_bom/lib_bom.c"

rnd_hid_t bom_hid;

int pplg_check_ver_export_bom(int ver_needed) { return 0; }

void pplg_uninit_export_bom(void)
{
	rnd_export_remove_opts_by_cookie(bom_cookie);
	rnd_conf_unreg_file(CONF_FN, export_bom_conf_internal);
	rnd_conf_unreg_fields("plugins/export_bom/");
	bom_fmt_uninit();
	rnd_hid_remove_hid(&bom_hid);
}

int pplg_init_export_bom(void)
{
	RND_API_CHK_VER;

	rnd_conf_reg_file(CONF_FN, export_bom_conf_internal);

	memset(&bom_hid, 0, sizeof(rnd_hid_t));

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_bom, field,isarray,type_name,cpath,cname,desc,flags);
#include "bom_conf_fields.h"

	rnd_hid_nogui_init(&bom_hid);

	bom_hid.struct_size = sizeof(rnd_hid_t);
	bom_hid.name = "bom";
	bom_hid.description = "Exports a BoM (Bill of Material) using templates";
	bom_hid.exporter = 1;

	bom_hid.get_export_options = bom_get_export_options;
	bom_hid.do_export = bom_do_export;
	bom_hid.parse_arguments = bom_parse_arguments;
	bom_hid.argument_array = bom_values;

	bom_hid.usage = bom_usage;

	rnd_hid_register_hid(&bom_hid);
	rnd_hid_load_defaults(&bom_hid, bom_options, NUM_OPTIONS);

	bom_fmt_init();
	return 0;
}
