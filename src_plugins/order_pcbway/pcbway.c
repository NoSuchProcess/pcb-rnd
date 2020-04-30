/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
#include <librnd/core/pcb-printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/paths.h>
#include <librnd/core/compat_fs.h>
#include "../src_plugins/lib_wget/lib_wget.h"
#include "../src_plugins/order/order.h"
#include "order_pcbway_conf.h"
#include "../src_plugins/order_pcbway/conf_internal.c"

conf_order_pcbway_t conf_order_pcbway;
#define ORDER_PCBWAY_CONF_FN "order_pcbway.conf"

#define CFG conf_order_pcbway.plugins.order_pcbway
#define SERVER "http://api-partner.pcbway.com"

static const char *order_xpm[] = {
"23 5 2 1",
" 	c None",
"+	c #000000",
" ++  +++ +++  +++  +++ ",
"+  + + + +  + +    + + ",
"+  + ++  +  + ++   ++  ",
"+  + + + +  + +    + + ",
" ++  + + +++  +++  + + ",
};

typedef struct pcbway_form_s {
	vtp0_t fields;   /* of pcb_order_field_t */
	vts0_t country_codes;
} pcbway_form_t;

static int pcbway_cahce_update_(rnd_hidlib_t *hidlib, const char *url, const char *path, int update, pcb_wget_opts_t *wopts)
{
	double mt, now = rnd_dtime();

	mt = pcb_file_mtime(hidlib, path);
	if (update || (mt < 0) || ((now - mt) > CFG.cache_update_sec)) {
		if (CFG.verbose) {
			if (update)
				rnd_message(RND_MSG_INFO, "pcbway: static '%s', updating it in the cache\n", path);
			else
				rnd_message(RND_MSG_INFO, "pcbway: stale '%s', updating it in the cache\n", path);
		}
		if (pcb_wget_disk(url, path, update, wopts) != 0) {
			rnd_message(RND_MSG_ERROR, "pcbway: failed to download %s\n", url);
			return -1;
		}
		
	}
	else if (CFG.verbose)
		rnd_message(RND_MSG_INFO, "pcbway: '%s' from cache\n", path);
	return 0;
}

static int pcbway_cache_update(rnd_hidlib_t *hidlib)
{
	char *hdr[5];
	pcb_wget_opts_t wopts;
	char *cachedir, *path;
	int res = 0;

	wopts.header = (const char **)hdr;
	hdr[0] = pcb_concat("api-key: ", CFG.api_key, NULL);
	hdr[1] = "Content-Type: application/xml";
	hdr[2] = "Accept: application/xml";
	hdr[3] = NULL;

	cachedir = pcb_build_fn(hidlib, conf_order.plugins.order.cache);

	pcb_mkdir(hidlib, cachedir, 0755);
	wopts.post_file = "/dev/null";
	path = pcb_strdup_printf("%s%cGetCountry", cachedir, RND_DIR_SEPARATOR_C);
	if (pcbway_cahce_update_(hidlib, SERVER "/api/Address/GetCountry", path, 0, &wopts) != 0) {
		res = -1;
		goto quit;
	}
	free(path);

	path = pcb_strdup_printf("%s%cPCBWay_Api.xml", cachedir, RND_DIR_SEPARATOR_C);
	if (pcbway_cahce_update_(hidlib, SERVER "/xml/PCBWay_Api.xml", path, 1, NULL) != 0) {
		res = -1;
		goto quit;
	}


	quit:;
	free(path);
	free(hdr[0]);
	free(cachedir);
	return res;
}



static xmlDoc *pcbway_xml_load(const char *fn)
{
	xmlDoc *doc;
	FILE *f;
	char *efn = NULL;

	f = pcb_fopen_fn(NULL, fn, "r", &efn);
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
				vts0_append(&form->country_codes, rnd_strdup(c->children->content));
	}

	xmlFreeDoc(doc);
}


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
				f->val.crd = PCB_MM_TO_COORD(strtod(dflt, NULL));
		}
		else if (strcmp(type, "string") == 0) {
			f->type = RND_HATT_STRING;
			if (dflt != NULL)
				f->val.str = rnd_strdup(dflt);
		}
		else {
			f->type = RND_HATT_LABEL;
		}
		if (strcmp(f->name, "Layers") == 0)         f->autoload = PCB_OAL_LAYERS;
		else if (strcmp(f->name, "Width") == 0)     f->autoload = PCB_OAL_WIDTH;
		else if (strcmp(f->name, "Length") == 0)    f->autoload = PCB_OAL_HEIGHT;

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

	if ((CFG.api_key == NULL) || (*CFG.api_key == '\0')) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: no api_key available.");
		return -1;
	}
	if (pcbway_cache_update(&PCB->hidlib) != 0) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: failed to update the cache.");
		return -1;
	}

	cachedir = pcb_build_fn(&PCB->hidlib, conf_order.plugins.order.cache);
	path = pcb_strdup_printf("%s%cPCBWay_Api.xml", cachedir, RND_DIR_SEPARATOR_C);
	doc = pcbway_xml_load(path);
	if (doc != NULL) {
		root = xmlDocGetRootElement(doc);
		if ((root != NULL) && (xmlStrcmp(root->name, (xmlChar *)"PCBWayAPI") == 0)) {
			octx->odata = calloc(sizeof(pcbway_form_t), 1);
			country_fn = pcb_strdup_printf("%s%cGetCountry", cachedir, RND_DIR_SEPARATOR_C);
			if (pcbway_load_countries(octx->odata, country_fn) != 0)
				res = -1;
			else if (pcbway_load_fields_(&PCB->hidlib, imp, octx, root) != 0) {
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

static void pcbway_dlg2fields(order_ctx_t *octx, pcbway_form_t *form)
{
	int n;
	for(n = 0; n < form->fields.used; n++) {
		pcb_order_field_t *f = form->fields.array[n];
		if (f->wid <= 0)
			continue;
		f->val = octx->dlg[f->wid].val;
	}
}

#define XML_TBL(dlg, node, price, pricetag) \
	do { \
		xmlNode *__n__; \
		double __prc__; \
		price = -1; \
		for(__n__ = node->children; __n__ != NULL; __n__ = __n__->next) { \
			char *val = NULL; \
			if ((__n__->children != NULL) && (__n__->children->type == XML_TEXT_NODE)) \
				val = __n__->children->content; \
			RND_DAD_LABEL(dlg, (char *)__n__->name); \
			if ((val != NULL) && (xmlStrcmp(__n__->name, (xmlChar *)pricetag) == 0)) { \
				char *__end__, *__tmp__ = pcb_concat("$", val, NULL); \
				RND_DAD_LABEL(dlg, __tmp__); \
				free(__tmp__); \
				__prc__ = strtod(val, &__end__); \
				if (*__end__ == '\0') price = __prc__; \
			} \
			else \
				RND_DAD_LABEL(dlg, val); \
		} \
	} while(0)

static int pcbway_present_quote(order_ctx_t *octx, const char *respfn)
{
	double shipcost, cost;
	xmlNode *root, *n, *error = NULL, *status = NULL, *ship = NULL, *prices = NULL;
	xmlDoc *doc = pcbway_xml_load(respfn);
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {NULL, 0}};
	RND_DAD_DECL(dlg);

	if (doc == NULL)
		return -1;

	root = xmlDocGetRootElement(doc);
	if ((root != NULL) && (xmlStrcmp(root->name, (xmlChar *)"PcbQuotationResponse") != 0)) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: wrong root node on quote answer\n");
		xmlFreeDoc(doc);
		return -1;
	}

	for(n = root->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"ErrorText") == 0) error = n;
		else if (xmlStrcmp(n->name, (xmlChar *)"Status") == 0) status = n;
		else if (xmlStrcmp(n->name, (xmlChar *)"Shipping") == 0) ship = n;
		else if (xmlStrcmp(n->name, (xmlChar *)"priceList") == 0) prices = n;
	}

	if ((status == NULL) || (status->children == NULL) || (status->children->type != XML_TEXT_NODE)) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: missing <Status> from the quote response\n");
		xmlFreeDoc(doc);
		return -1;
	}
	if (xmlStrcmp(status->children->content, (xmlChar *)"ok") != 0) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: server error in quote\n");
		if ((error != NULL) && (error->children != NULL) && (error->children->type == XML_TEXT_NODE))
			rnd_message(RND_MSG_ERROR, "  %s\n", error->children->content);
		xmlFreeDoc(doc);
		return -1;
	}

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);

		RND_DAD_BEGIN_TABLE(dlg, 2);
			RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			RND_DAD_LABEL(dlg, "=== Shipping ==="); RND_DAD_LABEL(dlg, "");
			XML_TBL(dlg, ship, shipcost, "ShipCost");
			for(n = prices->children; n != NULL; n = n->next) {
				char tmp[128];
				RND_DAD_LABEL(dlg, "=== Option ==="); RND_DAD_LABEL(dlg, "");
				XML_TBL(dlg, n, cost, "Price");
				if ((shipcost >= 0) && (cost >= 0))
					sprintf(tmp, "$%.2f    ", shipcost+cost);
				else
					*tmp = 0;

				RND_DAD_LABEL(dlg, "    Total:");
				RND_DAD_COMPFLAG(dlg, RND_HATF_TIGHT);

				RND_DAD_BEGIN_HBOX(dlg);
					RND_DAD_COMPFLAG(dlg, RND_HATF_TIGHT);
					RND_DAD_LABEL(dlg, tmp);
					RND_DAD_PICBUTTON(dlg, order_xpm);
				RND_DAD_END(dlg);

	
			}
		RND_DAD_END(dlg);

		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_NEW("pcbway_quote", dlg, "PCBWay: quote", NULL, pcb_true, NULL);
	RND_DAD_RUN(dlg);
	RND_DAD_FREE(dlg);

	xmlFreeDoc(doc);
	return 0;
}

static void pcbway_quote_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	order_ctx_t *octx = caller_data;
	pcbway_form_t *form = octx->odata;
	int n;
	FILE *fx;
	char *tmpfn, *respfn = "Resp.xml";

	pcbway_dlg2fields(octx, form);

	tmpfn = rnd_tempfile_name_new("pcbway_quote.xml");
	if (tmpfn == NULL) {
		rnd_message(RND_MSG_ERROR, "order_pcbway: can't get temp file name\n");
		return;
	}

	fx = pcb_fopen(&PCB->hidlib, tmpfn, "w");
	if (fx == NULL) {
		rnd_tempfile_unlink(tmpfn);
		rnd_message(RND_MSG_ERROR, "order_pcbway: can't open temp file\n");
		return;
	}


	fprintf(fx, "<PcbQuotationRequest xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://schemas.datacontract.org/2004/07/API.Models.Pcb\">\n");
	for(n = 0; n < form->fields.used; n++) {
		pcb_order_field_t *f = form->fields.array[n];
		fprintf(fx, " <%s>", (char *)f->name);

		switch(f->type) {
			case RND_HATT_ENUM:
				fprintf(fx, "%s", f->enum_vals[f->val.lng]);
				break;
			case RND_HATT_INTEGER:
				fprintf(fx, "%d", f->val.lng);
				break;
			case RND_HATT_COORD:
				pcb_fprintf(fx, "%mm", f->val.crd);
				break;

			case RND_HATT_STRING:
				if (f->val.str != NULL)
					fprintf(fx, "%s", f->val.str);
				break;

			default:
				break;
		}

		fprintf(fx, "</%s>\n", (char *)f->name);
	}
	fprintf(fx, "</PcbQuotationRequest>\n");
	fclose(fx);

	{
		char *hdr[5];
		pcb_wget_opts_t wopts;

		wopts.header = (const char **)hdr;
		hdr[0] = pcb_concat("api-key: ", CFG.api_key, NULL);
		hdr[1] = "Content-Type: application/xml";
		hdr[2] = "Accept: application/xml";
		hdr[3] = NULL;
		wopts.post_file = tmpfn;

		if (pcb_wget_disk(SERVER "/api/Pcb/PcbQuotation", respfn, 0, &wopts) != 0) {
			rnd_message(RND_MSG_ERROR, "pcbway: failed to get a quote from the server\n");
			goto err;
		}
	}

	pcbway_present_quote(octx, respfn);

	err:;
	rnd_tempfile_unlink(tmpfn);
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
			RND_DAD_BUTTON(octx->dlg, "Quote & order");
				RND_DAD_HELP(octx->dlg, "Generate a price quote");
				RND_DAD_CHANGE_CB(octx->dlg, pcbway_quote_cb);
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
	rnd_conf_unreg_file(ORDER_PCBWAY_CONF_FN, order_pcbway_conf_internal);
	rnd_conf_unreg_fields("plugins/order_pcbway/");
}

int pplg_init_order_pcbway(void)
{
	PCB_API_CHK_VER;

	rnd_conf_reg_file(ORDER_PCBWAY_CONF_FN, order_pcbway_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_order_pcbway, field,isarray,type_name,cpath,cname,desc,flags);
#include "order_pcbway_conf_fields.h"

	pcb_order_reg(&pcbway);
	return 0;
}
