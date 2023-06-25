#include "config.h"
#include "conf_core.h"
#include <librnd/core/rnd_conf.h>

#include <stdlib.h>

#include "build_run.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include "bom2_conf.h"

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>
#include <librnd/hid/hid_attrib.h>
#include "hid_cam.h"
#include <librnd/hid/hid_init.h>

#include "../src_plugins/export_bom2/conf_internal.c"

#define CONF_FN "export_bom2.conf"

conf_bom2_t conf_bom2;

const char *bom2_cookie = "BOM2 HID";

/* Maximum length of a template name (in the config file, in the enum) */
#define MAX_TEMP_NAME_LEN 128


/* This one can not be const because format enumeration is loaded run-time */
static rnd_export_opt_t bom2_options[] = {
	{"bom2file", "Name of the BoM output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_bom2file 0

	{"format", "file format (template)",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, NULL},
#define HA_format 1

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_cam 2
};

#define NUM_OPTIONS (sizeof(bom2_options)/sizeof(bom2_options[0]))

static rnd_hid_attr_val_t bom2_values[NUM_OPTIONS];

static const char *bom2_filename;

#include "lib_bom.h"

static const rnd_export_opt_t *bom2_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	rnd_conf_listitem_t *li;
	const char *val = bom2_values[HA_bom2file].str;
	int idx;

	/* load all formats from the config */
	fmt_names.used = 0;
	fmt_ids.used = 0;

	free_fmts();
	rnd_conf_loop_list(&conf_bom2.plugins.export_bom2.templates, li, idx) {
		char id[MAX_TEMP_NAME_LEN];
		const char *sep = strchr(li->name, '.');
		int len;

		if (sep == NULL) {
			rnd_message(RND_MSG_ERROR, "export_bom2: ignoring invalid template name (missing period): '%s'\n", li->name);
			continue;
		}
		if (strcmp(sep+1, "name") != 0)
			continue;
		len = sep - li->name;
		if (len > sizeof(id)-1) {
			rnd_message(RND_MSG_ERROR, "export_bom2: ignoring invalid template name (too long): '%s'\n", li->name);
			continue;
		}
		memcpy(id, li->name, len);
		id[len] = '\0';
		vts0_append(&fmt_names, (char *)li->payload);
		vts0_append(&fmt_ids, rnd_strdup(id));
	}

	if (fmt_names.used == 0) {
		rnd_message(RND_MSG_ERROR, "export_bom2: can not set up export options: no template available\n");
		return NULL;
	}

	bom2_options[HA_format].enumerations = (const char **)fmt_names.array;

	/* set default filename */
	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &bom2_values[HA_bom2file], ".bom2");

	if (n)
		*n = NUM_OPTIONS;
	return bom2_options;
}

static const char *subst_user(subst_ctx_t *ctx, const char *key)
{
	if (strcmp(key, "author") == 0) return pcb_author();
	if (strcmp(key, "title") == 0) return RND_UNKNOWN(PCB->hidlib.name);
	if (strncmp(key, "subc.", 5) == 0) {
		key += 5;

		if (strncmp(key, "a.", 2) == 0) return pcb_attribute_get(&ctx->subc->Attributes, key+2);
		else if (strcmp(key, "name") == 0) return ctx->name;
		if (strcmp(key, "prefix") == 0) {
			static char tmp[256]; /* this is safe: caller will make a copy right after we return */
			char *o = tmp;
			const char *t;
			int n = 0;

			for(t = ctx->name; isalpha(*t) && (n < sizeof(tmp)-1); t++,n++,o++)
				*o = *t;
			*o = '\0';
			return tmp;
		}
	}

	return NULL;
}

static int print_bom(const template_t *templ)
{
	FILE *f;
	subst_ctx_t ctx = {0};

	f = rnd_fopen_askovr(&PCB->hidlib, bom2_filename, "w", NULL);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Cannot open file %s for writing\n", bom2_filename);
		return 1;
	}

	bom_print_begin(&ctx, f, templ);

	/* For each subcircuit calculate an ID and count recurring IDs in a hash table and an array (for sorting) */
	PCB_SUBC_LOOP(PCB->Data);
	{
		const char *refdes = RND_UNKNOWN(pcb_attribute_get(&subc->Attributes, "refdes"));
		bom_print_add(&ctx, subc, refdes);
	}
	PCB_END_LOOP;

	bom_print_all(&ctx);
	bom_print_end(&ctx);
	fclose(f);

	return 0;
}

static void bom2_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	template_t templ = {0};
	char **tid;
	pcb_cam_t cam;

	gather_templates();

	if (!options) {
		bom2_get_export_options(hid, 0, design, appspec);
		options = bom2_values;
	}

	bom2_filename = options[HA_bom2file].str;
	if (!bom2_filename)
		bom2_filename = "pcb-rnd-out.bom2";

	pcb_cam_begin_nolayer(PCB, &cam, NULL, options[HA_cam].str, &bom2_filename);

	tid = vts0_get(&fmt_ids, options[HA_format].lng, 0);
	if ((tid == NULL) || (*tid == NULL)) {
		rnd_message(RND_MSG_ERROR, "export_bom2: invalid template selected\n");
		return;
	}

	bom_init_template(&templ, *tid);
	print_bom(&templ);
	pcb_cam_end(&cam);
}

static int bom2_usage(rnd_hid_t *hid, const char *topic)
{
	int n;
	fprintf(stderr, "\nBOM exporter command line arguments:\n\n");
	bom2_get_export_options(hid, &n, NULL, NULL);
	rnd_hid_usage(bom2_options, sizeof(bom2_options) / sizeof(bom2_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x bom2 [bom2_options] foo.pcb\n\n");
	return 0;
}

static int bom2_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, bom2_options, sizeof(bom2_options) / sizeof(bom2_options[0]), bom2_cookie, 0);

	/* when called from the export() command this field may be uninitialized yet */
	if (bom2_options[HA_format].enumerations == NULL)
		bom2_get_export_options(hid, NULL, NULL, NULL);

	return rnd_hid_parse_command_line(argc, argv);
}

#include "lib_bom.c"

rnd_hid_t bom2_hid;

int pplg_check_ver_export_bom2(int ver_needed) { return 0; }

void pplg_uninit_export_bom2(void)
{
	rnd_export_remove_opts_by_cookie(bom2_cookie);
	rnd_conf_unreg_file(CONF_FN, export_bom2_conf_internal);
	rnd_conf_unreg_fields("plugins/export_bom2/");
	free_fmts();
	vts0_uninit(&fmt_names);
	vts0_uninit(&fmt_ids);
	rnd_hid_remove_hid(&bom2_hid);
}

int pplg_init_export_bom2(void)
{
	RND_API_CHK_VER;

	rnd_conf_reg_file(CONF_FN, export_bom2_conf_internal);

	memset(&bom2_hid, 0, sizeof(rnd_hid_t));

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_bom2, field,isarray,type_name,cpath,cname,desc,flags);
#include "bom2_conf_fields.h"

	rnd_hid_nogui_init(&bom2_hid);

	bom2_hid.struct_size = sizeof(rnd_hid_t);
	bom2_hid.name = "bom2";
	bom2_hid.description = "Exports a BoM (Bill of Material) using templates";
	bom2_hid.exporter = 1;

	bom2_hid.get_export_options = bom2_get_export_options;
	bom2_hid.do_export = bom2_do_export;
	bom2_hid.parse_arguments = bom2_parse_arguments;
	bom2_hid.argument_array = bom2_values;

	bom2_hid.usage = bom2_usage;

	rnd_hid_register_hid(&bom2_hid);
	rnd_hid_load_defaults(&bom2_hid, bom2_options, NUM_OPTIONS);

	vts0_init(&fmt_names);
	vts0_init(&fmt_ids);
	return 0;
}
