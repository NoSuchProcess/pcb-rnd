#include "config.h"
#include "conf_core.h"
#include <librnd/core/rnd_conf.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <genvector/vts0.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include <genht/ht_utils.h>

#include <librnd/core/math_helper.h>
#include "build_run.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/rotate.h>
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



typedef struct {
	char utcTime[64];
	char *name, *footprint, *value;
	pcb_subc_t *subc;
	long count;
	gds_t tmp;
	const char *needs_escape; /* list of characters that need escaping */
	const char *escape; /* escape character or NULL for replacing with _*/
} subst_ctx_t;

static void append_clean(subst_ctx_t *ctx, int escape, gds_t *dst, const char *text)
{
	const char *s;

	if (!escape) {
		gds_append_str(dst, text);
		return;
	}

	for(s = text; *s != '\0'; s++) {
		switch(*s) {
			case '\n': gds_append_str(dst, "\\n"); break;
			case '\r': gds_append_str(dst, "\\r"); break;
			case '\t': gds_append_str(dst, "\\t"); break;
			default:
				if ((ctx->needs_escape != NULL) && (strchr(ctx->needs_escape, *s) != NULL)) {
					if ((ctx->escape != NULL) && (*ctx->escape != '\0')) {
						gds_append(dst, *ctx->escape);
						gds_append(dst, *s);
					}
					else
						gds_append(dst, '_');
				}
				else
					gds_append(dst, *s);
				break;
		}
	}
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
	subst_ctx_t *ctx = ctx_;
	int escape = 0;

	if (strncmp(*input, "escape.", 7) == 0) {
		*input += 7;
		escape = 1;
	}


	if (strncmp(*input, "UTC%", 4) == 0) {
		*input += 4;
		append_clean(ctx, escape, s, ctx->utcTime);
		return 0;
	}
	if (strncmp(*input, "author%", 7) == 0) {
		*input += 7;
		append_clean(ctx, escape, s, pcb_author());
		return 0;
	}
	if (strncmp(*input, "title%", 6) == 0) {
		*input += 6;
		append_clean(ctx, escape, s, RND_UNKNOWN(PCB->hidlib.name));
		return 0;
	}

	if (strncmp(*input, "names%", 6) == 0) {
		*input += 6;
		append_clean(ctx, escape, s, ctx->name);
		return 0;
	}

	if (strncmp(*input, "count%", 6) == 0) {
		*input += 6;
		rnd_append_printf(s, "%ld", ctx->count);
		return 0;
	}

	if (strncmp(*input, "subc.", 5) == 0) {
		*input += 5;

		/* subc attribute print:
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
				rnd_message(RND_MSG_ERROR, "bom2 tempalte error: attribute name '%s' too long\n", *input);
				return 1;
			}
			memcpy(aname, *input, len);
			aname[len] = '\0';
			if (*end == '|') { /* "or unknown" */
				*input = end+1;
				end = strchr(*input, '%');
				len = end - *input;
				if (len >= sizeof(unk_buf) - 1) {
					rnd_message(RND_MSG_ERROR, "bom2 tempalte error: elem atribute '|unknown' field '%s' too long\n", *input);
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
					rnd_message(RND_MSG_ERROR, "bom2 tempalte error: elem atribute trenary field '%s' too long\n", *input);
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
					append_clean(ctx, escape, s, unk_buf);
				else
					append_clean(ctx, escape, s, nope);

				return 0;
			}
			else
				*input = end;
			(*input)++;

			val = pcb_attribute_get(&ctx->subc->Attributes, aname);
			if (val == NULL)
				val = unk;
			append_clean(ctx, escape, s, val);
			return 0;
		}
		if (strncmp(*input, "refdes%", 7) == 0) {
			*input += 8;
			append_clean(ctx, escape, s, ctx->name);
			return 0;
		}
		if (strncmp(*input, "prefix%", 7) == 0) {
			const char *t;
			*input += 7;

			for(t = ctx->name; isalpha(*t); t++)
				gds_append(s, *t);
			return 0;
		}

		if (strncmp(*input, "footprint%", 10) == 0) {
			*input += 11;
			append_clean(ctx, escape, s, ctx->footprint);
			return 0;
		}
		if (strncmp(*input, "value%", 6) == 0) {
			*input += 7;
			append_clean(ctx, escape, s, ctx->value);
			return 0;
		}
	}
	return -1;
}

static void fprintf_templ(FILE *f, subst_ctx_t *ctx, const char *templ)
{
	if (templ != NULL) {
		char *tmp = rnd_strdup_subst(templ, subst_cb, ctx, RND_SUBST_PERCENT);
		fprintf(f, "%s", tmp);
		free(tmp);
	}
}

static char *render_templ(subst_ctx_t *ctx, const char *templ)
{
	if (templ != NULL)
		return rnd_strdup_subst(templ, subst_cb, ctx, RND_SUBST_PERCENT);
	return NULL;
}

typedef struct {
	const char *header, *item, *footer, *subc2id;
	const char *needs_escape; /* list of characters that need escaping */
	const char *escape; /* escape character */
} template_t;

typedef struct {
	pcb_subc_t *subc; /* one of the subcircuits picked randomly, for the attributes */
	char *id; /* key for sorting */
	gds_t refdes_list;
	long cnt;
} item_t;

static int item_cmp(const void *item1, const void *item2)
{
	const item_t * const *i1 = item1;
	const item_t * const *i2 = item2;
	return strcmp((*i1)->id, (*i2)->id);
}

static int PrintBOM(const template_t *templ, const char *format_name)
{
	FILE *fp;
	subst_ctx_t ctx = {0};
	htsp_t tbl;
	vtp0_t arr = {0};
	long n;

	fp = rnd_fopen_askovr(&PCB->hidlib, bom2_filename, "w", NULL);
	if (!fp) {
		rnd_message(RND_MSG_ERROR, "Cannot open file %s for writing\n", bom2_filename);
		return 1;
	}

	gds_init(&ctx.tmp);

	rnd_print_utc(ctx.utcTime, sizeof(ctx.utcTime), 0);

	fprintf_templ(fp, &ctx, templ->header);

	htsp_init(&tbl, strhash, strkeyeq);

	ctx.escape = templ->escape;
	ctx.needs_escape = templ->needs_escape;

	/* For each subcircuit calculate an ID and count recurring IDs in a hash table and an array (for sorting) */
	PCB_SUBC_LOOP(PCB->Data);
	{
		char *id, *freeme;
		item_t *i;
		const char *refdes = RND_UNKNOWN(pcb_attribute_get(&subc->Attributes, "refdes"));

		ctx.subc = subc;
		ctx.name = (char *)refdes;

		id = freeme = render_templ(&ctx, templ->subc2id);
		i = htsp_get(&tbl, id);
		if (i == NULL) {
			i = malloc(sizeof(item_t));
			i->id = id;
			i->subc = subc;
			i->cnt = 1;
			gds_init(&i->refdes_list);

			htsp_set(&tbl, id, i);
			vtp0_append(&arr, i);
			freeme = NULL;
		}
		else {
			i->cnt++;
			gds_append(&i->refdes_list, ' ');
		}

		gds_append_str(&i->refdes_list, refdes);
		rnd_trace("id='%s' %ld\n", id, i->cnt);

		free(freeme);
	}
	PCB_END_LOOP;

	/* clean up and sort the array */
	ctx.subc = NULL;
	qsort(arr.array, arr.used, sizeof(item_t *), item_cmp);

	/* produce the actual output from the sorted array */
	for(n = 0; n < arr.used; n++) {
		item_t *i = arr.array[n];
		ctx.subc = i->subc;
		ctx.name = i->refdes_list.array;
		ctx.count = i->cnt;
		fprintf_templ(fp, &ctx, templ->item);
	}

	fprintf_templ(fp, &ctx, templ->footer);

	fclose(fp);
	gds_uninit(&ctx.tmp);

	genht_uninit_deep(htsp, &tbl, {
		item_t *i = htent->value;
		free(i->id);
		gds_uninit(&i->refdes_list);
		free(i);
	});
	vtp0_uninit(&arr);

	return 0;
}

static void gather_templates(void)
{
	rnd_conf_listitem_t *i;
	int n;

	rnd_conf_loop_list(&conf_bom2.plugins.export_bom2.templates, i, n) {
		char buff[256], *id, *sect;
		int nl = strlen(i->name);
		if (nl > sizeof(buff)-1) {
			rnd_message(RND_MSG_ERROR, "export_bom2: ignoring template '%s': name too long\n", i->name);
			continue;
		}
		memcpy(buff, i->name, nl+1);
		id = buff;
		sect = strchr(id, '.');
		if (sect == NULL) {
			rnd_message(RND_MSG_ERROR, "export_bom2: ignoring template '%s': does not have a .section suffix\n", i->name);
			continue;
		}
		*sect = '\0';
		sect++;
	}
}

static const char *get_templ(const char *tid, const char *type)
{
	char path[MAX_TEMP_NAME_LEN + 16];
	rnd_conf_listitem_t *li;
	int idx;

	sprintf(path, "%s.%s", tid, type); /* safe: tid's length is checked before it was put in the vector, type is hardwired in code and is never longer than a few chars */
	rnd_conf_loop_list(&conf_bom2.plugins.export_bom2.templates, li, idx)
		if (strcmp(li->name, path) == 0)
			return li->payload;
	return NULL;
}

static void bom2_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	template_t templ;
	char **tid;
	pcb_cam_t cam;

	memset(&templ, 0, sizeof(templ));


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
	templ.header       = get_templ(*tid, "header");
	templ.item         = get_templ(*tid, "item");
	templ.footer       = get_templ(*tid, "footer");
	templ.subc2id      = get_templ(*tid, "subc2id");
	templ.escape       = get_templ(*tid, "escape");
	templ.needs_escape = get_templ(*tid, "needs_escape");

	PrintBOM(&templ, options[HA_format].str);
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
