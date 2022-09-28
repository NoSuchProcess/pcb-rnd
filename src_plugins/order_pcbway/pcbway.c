/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2022 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <genvector/vtp0.h>
#include <genvector/vts0.h>

#include "board.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/paths.h>
#include <librnd/core/compat_fs.h>
#include <librnd/plugins/lib_wget/lib_wget.h>
#include "../src_plugins/order/order.h"
#include "order_pcbway_conf.h"
#include "../src_plugins/order_pcbway/conf_internal.c"

conf_order_pcbway_t conf_order_pcbway;

#define CFG conf_order_pcbway.plugins.order_pcbway
#define SERVER "http://api-partner.pcbway.com"

typedef struct pcbway_form_s {
	vtp0_t fields;   /* of pcb_order_field_t */
	vts0_t country_codes;
} pcbway_form_t;

static int pcbway_cache_update_(rnd_hidlib_t *hidlib, const char *url, const char *path, int update, rnd_wget_opts_t *wopts)
{
	double mt, now = rnd_dtime(), timeout;

	mt = rnd_file_mtime(hidlib, path);
	if (update)
		timeout = 3600; /* harder update still checks only once an hour */
	else
		timeout = CFG.cache_update_sec;

	if ((mt < 0) || ((now - mt) > timeout)) {
		if (CFG.verbose) {
			if (update)
				rnd_message(RND_MSG_INFO, "pcbway: static '%s', updating it in the cache\n", path);
			else
				rnd_message(RND_MSG_INFO, "pcbway: stale '%s', updating it in the cache\n", path);
		}
		if (rnd_wget_disk(url, path, update, wopts) != 0) {
			rnd_message(RND_MSG_ERROR, "pcbway: failed to download %s\n", url);
			return -1;
		}

		{
			/* append a newline at the end to update last mod date on the file system:
			   wget tends to set the date to match last modified date on the server */
			char spc = '\n';
			FILE *f = rnd_fopen(&PCB->hidlib, path, "a");
			fwrite(&spc, 1, 1, f);
			fclose(f);
		}
	}
	else if (CFG.verbose)
		rnd_message(RND_MSG_INFO, "pcbway: '%s' from cache\n", path);
	return 0;
}

static int pcbway_cache_update(rnd_hidlib_t *hidlib)
{
	char *hdr[5];
	rnd_wget_opts_t wopts;
	char *cachedir, *path;
	int res = 0;

	wopts.header = (const char **)hdr;
/*	hdr[0] = rnd_concat("api-key: ", CFG.api_key, NULL);*/
	hdr[0] = "Content-Type: application/xml";
	hdr[1] = "Accept: application/xml";
	hdr[2] = NULL;

	cachedir = rnd_build_fn(hidlib, conf_order.plugins.order.cache);

	rnd_mkdir(hidlib, cachedir, 0755);
	wopts.post_file = "/dev/null";
#if 0
	path = rnd_strdup_printf("%s%cGetCountry", cachedir, RND_DIR_SEPARATOR_C);
	if (pcbway_cache_update_(hidlib, SERVER "/api/Address/GetCountry", path, 0, &wopts) != 0) {
		res = -1;
		goto quit;
	}
	free(path);
#endif

	path = rnd_strdup_printf("%s%cPCBWay_Api2.xml", cachedir, RND_DIR_SEPARATOR_C);
	if (pcbway_cache_update_(hidlib, SERVER "/xml/PCBWay_Api2.xml", path, 1, NULL) != 0) {
		res = -1;
		goto quit;
	}


	quit:;
	free(path);
/*	free(hdr[0]);*/
	free(cachedir);
	return res;
}



static xmlDoc *pcbway_xml_load(const char *fn)
{
	xmlDoc *doc;
	FILE *f;
	char *efn = NULL;

	f = rnd_fopen_fn(NULL, fn, "r", &efn);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "pcbway: can't open '%s' (%s) for read\n", fn, efn);
		free(efn);
		return NULL;
	}
	fclose(f);

	doc = xmlReadFile(efn, NULL, 0);
	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "xml parsing error on file %s (%s)\n", fn, efn);
		free(efn);
		return NULL;
	}
	free(efn);

	return doc;
}

#if 0
static int pcbway_load_countries(pcbway_form_t *form, const char *fn)
{
	xmlDoc *doc = pcbway_xml_load(fn);
	xmlNode *root, *n, *c;

	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: failed to parse the country xml\n");
		return -1;
	}
	root = xmlDocGetRootElement(doc);
	if ((root != NULL) && (xmlStrcmp(root->name, (xmlChar *)"Country") != 0)) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: wrong root node in the country xml\n");
		return -1;
	}

	for(root = root->children; (root != NULL) && (xmlStrcmp(root->name, (xmlChar *)"Countrys") != 0); root = root->next) ;
	if (root == NULL) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: failed to find a <Countrys> node\n");
		return -1;
	}

	for(n = root->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"CountryModel") != 0)
			continue;
		for(c = n->children; c != NULL; c = c->next)
			if ((c->children != NULL) && (c->children->type == XML_TEXT_NODE) && (xmlStrcmp(c->name, (xmlChar *)"CountryCode") == 0))
				vts0_append(&form->country_codes, rnd_strdup((char *)c->children->content));
	}

	xmlFreeDoc(doc);
	return 0;
}
#endif


static int pcbway_load_fields_(rnd_hidlib_t *hidlib, pcb_order_imp_t *imp, order_ctx_t *octx, xmlNode *root)
{
	xmlNode *n, *v;
	pcbway_form_t *form = (pcbway_form_t *)octx->odata;
	for(root = root->children; (root != NULL) && (xmlStrcmp(root->name, (xmlChar *)"PcbQuotationRequest") != 0); root = root->next) ;

	if (root == NULL)
		return -1;

	for(n = root->children; n != NULL; n = n->next) {
		const char *type, *note, *dflt;
		pcb_order_field_t *f;
		if ((n->type == XML_TEXT_NODE) || (n->name == NULL))
			continue;
		type = (const char *)xmlGetProp(n, (const xmlChar *)"type");
		note = (const char *)xmlGetProp(n, (const xmlChar *)"note");
		dflt = (const char *)xmlGetProp(n, (const xmlChar *)"default");

		if ((type != NULL && strlen(type) > 128) || (strlen((char *)n->name) > 128) || (note != NULL && strlen(note) > 256) || (dflt != NULL && strlen(dflt) > 128)) {
			rnd_message(RND_MSG_ERROR, "order_pcbway: invalid field description: too long\n");
			return -1;
		}

		f = calloc(sizeof(pcb_order_field_t) + strlen((char *)n->name), 1);
		strcpy(f->name, (char *)n->name);
		if (note != NULL)
			f->help = rnd_strdup(note);
		if (type == NULL) {
			f->type = RND_HATT_LABEL;
		}
		else if (strcmp(type, "enum") == 0) {
			int di = 0, i;
			vtp0_t tmp;

			f->type = RND_HATT_ENUM;
			vtp0_init(&tmp);
			for(v = n->children, i = 0; v != NULL; v = v->next) {
				char *s;
				if ((n->type == XML_TEXT_NODE) || (xmlStrcmp(v->name, (xmlChar *)"Value") != 0) || (n->children->type != XML_TEXT_NODE))
					continue;
				s = rnd_strdup((char *)v->children->content);
				vtp0_append(&tmp, s);
				if ((dflt != NULL) && (strcmp(s, dflt) == 0))
					di = i;
				i++;
			}
			vtp0_append(&tmp, NULL);
			f->enum_vals = (char **)tmp.array;
			if (dflt != NULL)
				f->val.lng = di;
		}
		else if (strcmp(type, "enum:country:code") == 0) {
			f->type = RND_HATT_ENUM;
			f->enum_vals = form->country_codes.array;
		}
		else if (strcmp(type, "integer") == 0) {
			f->type = RND_HATT_INTEGER;
			if (dflt != NULL)
				f->val.lng = atoi(dflt);
		}
		else if (strcmp(type, "mm") == 0) {
			f->type = RND_HATT_COORD;
			if (dflt != NULL)
				f->val.crd = RND_MM_TO_COORD(strtod(dflt, NULL));
		}
		else if (strcmp(type, "string") == 0) {
			f->type = RND_HATT_STRING;
			if (dflt != NULL)
				f->val.str = rnd_strdup(dflt);
		}
		else {
			f->type = RND_HATT_LABEL;
		}
		if (strcmp(f->name, "boardLayer") == 0)          f->autoload = PCB_OAL_LAYERS;
		else if (strcmp(f->name, "boardWidth") == 0)     f->autoload = PCB_OAL_WIDTH;
		else if (strcmp(f->name, "boardHeight") == 0)    f->autoload = PCB_OAL_HEIGHT;

		pcb_order_autoload_field(octx, f);

		vtp0_append(&form->fields, f);
		if (form->fields.used > 128) {
			rnd_message(RND_MSG_ERROR, "order_pcbway: too many fields for a form\n");
			return -1;
		}
	}
	return 0;
}

static int pcbway_load_fields(pcb_order_imp_t *imp, order_ctx_t *octx)
{
	char *cachedir, *path, *country_fn;
	xmlDoc *doc;
	xmlNode *root;
	int res = 0;

	octx->odata = NULL;

#if 0
	if ((CFG.api_key == NULL) || (*CFG.api_key == '\0')) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: no api_key available.");
		return -1;
	}
#endif
	if (pcbway_cache_update(&PCB->hidlib) != 0) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: failed to update the cache.");
		return -1;
	}

	cachedir = rnd_build_fn(&PCB->hidlib, conf_order.plugins.order.cache);
	path = rnd_strdup_printf("%s%cPCBWay_Api2.xml", cachedir, RND_DIR_SEPARATOR_C);
	doc = pcbway_xml_load(path);
	if (doc != NULL) {
		root = xmlDocGetRootElement(doc);
		if ((root != NULL) && (xmlStrcmp(root->name, (xmlChar *)"PCBWayAPI") == 0)) {
			octx->odata = calloc(sizeof(pcbway_form_t), 1);
			country_fn = rnd_strdup_printf("%s%cGetCountry", cachedir, RND_DIR_SEPARATOR_C);
#if 0
			if (pcbway_load_countries(octx->odata, country_fn) != 0)
				res = -1;
			else 
#endif
			if (pcbway_load_fields_(&PCB->hidlib, imp, octx, root) != 0) {
				rnd_message(RND_MSG_ERROR, "order_pcbway: xml error: invalid API xml\n");
				res = -1;
			}
			free(country_fn);
		}
		else
			rnd_message(RND_MSG_ERROR, "order_pcbway: xml error: root is not <PCBWayAPI>\n");
	}
	else
		rnd_message(RND_MSG_ERROR, "order_pcbway: xml error: failed to parse the xml\n");

	xmlFreeDoc(doc);
	free(cachedir);
	free(path);

	return res;
}

static void pcbway_free_fields(pcb_order_imp_t *imp, order_ctx_t *octx)
{
	int n;
	pcbway_form_t *form = octx->odata;
	for(n = 0; n < form->fields.used; n++) {
		pcb_order_field_t *f = form->fields.array[n];
		pcb_order_free_field_data(octx, f);
		free(f);
	}
	for(n = 0; n < form->country_codes.used; n++)
		free(form->country_codes.array[n]);
	vtp0_uninit(&form->fields);
	vts0_uninit(&form->country_codes);
	free(form);
}



static void pcbway_order_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hidlib_t *hidlib = &PCB->hidlib;
	order_ctx_t *octx = caller_data;
	pcbway_form_t *form = octx->odata;
	FILE *f, *ft, *fr;
	char sep[32], *s, *sephdr;
	static const char rch[] = "0123456789abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ";
	char *postfile = "POST.txt", *tarname = "gerb.tar";
	char *hdr[5];
	rnd_wget_opts_t wopts = {0};
	int n, found;

TODO("Do not hardwire postile and tarname");
TODO("Use CAM export to generate the tarball");

	/* generate unique content separator */
	strcpy(sep, "----pcb-rnd-");
	for(s = sep+12; s < sep+sizeof(sep)-1; s++)
		*s = rch[rand() % sizeof(rch)];
	*s = '\0';

	f = rnd_fopen(hidlib, postfile, "w");

	/* header */
	sephdr = rnd_concat("Content-Type: multipart/form-data; boundary=", sep, NULL);
	hdr[0] = sephdr;
	hdr[1] = NULL;

	wopts.header = (const char **)hdr;
	wopts.post_file = postfile;

	/* data fields */
	for(n = 0; n < form->fields.used; n++) {
		pcb_order_field_t *fld = form->fields.array[n];
		fprintf(f, "--%s\r\n", sep);
		fprintf(f, "Content-Disposition: form-data; name=\"%s\"\r\n", fld->name);
		fprintf(f, "\r\n");
		switch(fld->type) {
			case RND_HATT_INTEGER: fprintf(f, "%ld\r\n", fld->val.lng); break;
			case RND_HATT_COORD: rnd_fprintf(f, "%mm\r\n", fld->val.crd); break;
			case RND_HATT_STRING: fprintf(f, "%s\r\n", fld->val.str); break;
			case RND_HATT_ENUM: fprintf(f, "%s\r\n", fld->enum_vals[fld->val.lng]); break;
			default:
				rnd_message(RND_MSG_ERROR, "internal error: pcbway_order_cb: field type %d not handled\nPlease report this bug!\n", fld->type);
			break;
		}
	}

	/* tarball with the gerbers */
	ft = rnd_fopen(hidlib, tarname, "r");
	if (ft != NULL) {
/*		long l = rnd_file_size(hidlib, tarname);*/
		fprintf(f, "--%s\r\n", sep);
		fprintf(f, "Content-Disposition: form-data; name=\"gerberfile\"; filename=\"gerber.tar\"\r\n");
		fprintf(f, "Content-Type: application/tar\r\n");
		fprintf(f, "\r\n");
		for(;;) {
			char buff[8192];
			long len;
			len = fread(buff, 1, sizeof(buff), ft);
			if (len <= 0)
				break;
			fwrite(buff, 1, len, f);
		}
		fprintf(f, "\r\n");
		fclose(ft);
	}
	else
		rnd_message(RND_MSG_ERROR, "internal error: pcbway_order_cb: can not open gerber tarball '%s'\nPlease report this bug!\n", tarname);

	/* closing sep */
	fprintf(f, "--%s--\r\n", sep);
	fclose(f);


	fr = rnd_wget_popen("https://www.pcbway.com/common/PCB-rndUpFile", 0, &wopts);
	found = 0;
	if (fr != NULL) {
		char *line, tmp[1024], *re, *end, *url;
		while((line = fgets(tmp, sizeof(tmp), fr)) != NULL) {
			re = strstr(line, "\"redirect\":");
			if (re != NULL) {
				re += 12;
				end = strchr(re, '\"');
				if (end != NULL) {
					url = re;
					*end = '\0';
					found = 1;
					rnd_trace("*** PCBWAY url: '%s'\n", url);
				}
				break;
			}
		}
		fclose(fr);

		if (!found)
			rnd_message(RND_MSG_ERROR, "pcbway_order: server failed to process the order and return a redirect URL\n");
	}
	else
		rnd_message(RND_MSG_ERROR, "pcbway_order: failed upload the order\n(may be temporary network error)\n");

	free(sephdr);
}


static void pcbway_populate_dad(pcb_order_imp_t *imp, order_ctx_t *octx)
{
	int n;
	pcbway_form_t *form = octx->odata;

	RND_DAD_BEGIN_VBOX(octx->dlg);
		RND_DAD_COMPFLAG(octx->dlg, RND_HATF_SCROLL | RND_HATF_EXPFILL);

		for(n = 0; n < form->fields.used; n++)
			pcb_order_dad_field(octx, form->fields.array[n]);

		RND_DAD_BEGIN_VBOX(octx->dlg);
			RND_DAD_COMPFLAG(octx->dlg, RND_HATF_EXPFILL);
			RND_DAD_LABEL(octx->dlg, "");
			RND_DAD_LABEL(octx->dlg, "");
		RND_DAD_END(octx->dlg);

		RND_DAD_BEGIN_HBOX(octx->dlg);
			RND_DAD_BUTTON(octx->dlg, "Update data");
				RND_DAD_HELP(octx->dlg, "Copy data from board to form");
			RND_DAD_BEGIN_VBOX(octx->dlg);
				RND_DAD_COMPFLAG(octx->dlg, RND_HATF_EXPFILL);
			RND_DAD_END(octx->dlg);
			RND_DAD_BUTTON(octx->dlg, "Order (web)");
				RND_DAD_HELP(octx->dlg, "Upload gerbers and fab parameters and\ngenerate an URL for the ordering form");
				RND_DAD_CHANGE_CB(octx->dlg, pcbway_order_cb);
		RND_DAD_END(octx->dlg);

	RND_DAD_END(octx->dlg);
}

static pcb_order_imp_t pcbway = {
	"PCBWay",
	NULL,
	pcbway_load_fields,
	pcbway_free_fields,
	pcbway_populate_dad
};


int pplg_check_ver_order_pcbway(int ver_needed) { return 0; }

void pplg_uninit_order_pcbway(void)
{
	rnd_conf_unreg_intern(order_pcbway_conf_internal);
	rnd_conf_unreg_fields("plugins/order_pcbway/");
}

int pplg_init_order_pcbway(void)
{
	RND_API_CHK_VER;

	rnd_conf_reg_intern(order_pcbway_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_order_pcbway, field,isarray,type_name,cpath,cname,desc,flags);
#include "order_pcbway_conf_fields.h"

	pcb_order_reg(&pcbway);
	return 0;
}
