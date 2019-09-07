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

#include "board.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "safe_fs.h"
#include "paths.h"
#include "../src_plugins/lib_wget/lib_wget.h"
#include "../src_plugins/order/order.h"
#include "order_pcbway_conf.h"
#include "../src_plugins/order_pcbway/conf_internal.c"

conf_order_pcbway_t conf_order_pcbway;
#define ORDER_PCBWAY_CONF_FN "order_pcbway.conf"

#define CFG conf_order_pcbway.plugins.order_pcbway
#define SERVER "http://api-partner.pcbway.com"

static int pcbway_cahce_update_(pcb_hidlib_t *hidlib, const char *url, const char *path, int update, pcb_wget_opts_t *wopts)
{
	double mt, now = pcb_dtime();

	mt = pcb_file_mtime(hidlib, path);
	if (update || (mt < 0) || ((now - mt) > CFG.cache_update_sec)) {
		if (CFG.verbose) {
			if (update)
				pcb_message(PCB_MSG_INFO, "pcbway: static '%s', updating it in the cache\n", path);
			else
				pcb_message(PCB_MSG_INFO, "pcbway: stale '%s', updating it in the cache\n", path);
		}
		if (pcb_wget_disk(url, path, update, wopts) != 0) {
			pcb_message(PCB_MSG_ERROR, "pcbway: failed to download %s\n", url);
			return -1;
		}
		
	}
	else if (CFG.verbose)
		pcb_message(PCB_MSG_INFO, "pcbway: '%s' from cache\n", path);
	return 0;
}

static int pcbway_cache_update(pcb_hidlib_t *hidlib)
{
	char *hdr[5];
	pcb_wget_opts_t wopts;
	char *cachedir, *path;
	int res = 0;

	wopts.header = hdr;
	hdr[0] = pcb_concat("api-key: ", CFG.api_key, NULL);
	hdr[1] = "Content-Type: application/xml";
	hdr[2] = "Accept: application/xml";
	hdr[3] = NULL;

	cachedir = pcb_build_fn(hidlib, conf_order.plugins.order.cache);

	pcb_mkdir(hidlib, cachedir, 0755);
	wopts.post_file = "/dev/null";
	path = pcb_strdup_printf("%s%cGetCountry", cachedir, PCB_DIR_SEPARATOR_C);
	if (pcbway_cahce_update_(hidlib, SERVER "/api/Address/GetCountry", path, 0, &wopts) != 0) {
		res = -1;
		goto quit;
	}
	free(path);

	path = pcb_strdup_printf("%s%cPCBWay_Api.xml", cachedir, PCB_DIR_SEPARATOR_C);
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
		pcb_message(PCB_MSG_ERROR, "pcbway: can't open '%s' (%s) for read\n", fn, efn);
		free(efn);
		return NULL;
	}
	fclose(f);

	doc = xmlReadFile(efn, NULL, 0);
	if (doc == NULL) {
		pcb_message(PCB_MSG_ERROR, "xml parsing error on file %s (%s)\n", fn, efn);
		free(efn);
		return NULL;
	}
	free(efn);

	return doc;
}

static int pcbway_populate_dad_(pcb_hidlib_t *hidlib, pcb_order_imp_t *imp, order_ctx_t *octx, xmlNode *root)
{
	xmlNode *n, *v;
	for(root = root->children; (root != NULL) && (xmlStrcmp(root->name, (xmlChar *)"PcbQuotationRequest") != 0); root = root->next) ;

	if (root == NULL)
		return -1;

	PCB_DAD_BEGIN_VBOX(octx->dlg);
		PCB_DAD_COMPFLAG(octx->dlg, PCB_HATF_SCROLL | PCB_HATF_EXPFILL);
		for(n = root->children; n != NULL; n = n->next) {
			const char *type, *note, *dflt;
			if (n->type == XML_TEXT_NODE)
				continue;
			type = (const char *)xmlGetProp(n, (const xmlChar *)"type");
			note = (const char *)xmlGetProp(n, (const xmlChar *)"note");
			dflt = (const char *)xmlGetProp(n, (const xmlChar *)"default");
			if (type == NULL)
				type = "label";
			PCB_DAD_BEGIN_HBOX(octx->dlg);
				PCB_DAD_LABEL(octx->dlg, (char *)n->name);
				PCB_DAD_BEGIN_VBOX(octx->dlg);
					PCB_DAD_COMPFLAG(octx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_END(octx->dlg);
				if (strcmp(type, "enum") == 0) {
					int di = 0, i;
					vtp0_t tmp;
					vtp0_init(&tmp);
					for(v = n->children, i = 0; v != NULL; v = v->next) {
						char *s;
						if ((n->type == XML_TEXT_NODE) || (xmlStrcmp(v->name, (xmlChar *)"Value") != 0) || (n->children->type != XML_TEXT_NODE))
							continue;
						s = pcb_strdup(v->children->content);
						vtp0_append(&tmp, s);
						if ((dflt != NULL) && (strcmp(s, dflt) == 0)) {
							di = i;
							printf("enum dflt: '%s' is %d\n", dflt, di);
						}
						i++;
					}
					vtp0_append(&tmp, NULL);
					PCB_DAD_ENUM(octx->dlg, tmp.array);
					if (dflt != NULL)
						PCB_DAD_DEFAULT_NUM(octx->dlg, di);
				}
				else if (strcmp(type, "integer") == 0) {
					PCB_DAD_INTEGER(octx->dlg, "");
					if (dflt != NULL)
						PCB_DAD_DEFAULT_NUM(octx->dlg, atoi(dflt));
				}
				else if (strcmp(type, "mm") == 0) {
					PCB_DAD_COORD(octx->dlg, "");
					if (dflt != NULL)
						PCB_DAD_DEFAULT_NUM(octx->dlg, PCB_MM_TO_COORD(atoi(dflt)));
				}
				else if (strcmp(type, "string") == 0) {
					PCB_DAD_STRING(octx->dlg);
					if (dflt != NULL)
						PCB_DAD_DEFAULT_PTR(octx->dlg, dflt);
				}
			PCB_DAD_END(octx->dlg);
		}
	PCB_DAD_END(octx->dlg);
	return 0;
}

static void pcbway_populate_dad(pcb_order_imp_t *imp, order_ctx_t *octx)
{
	char *cachedir, *path;
	xmlDoc *doc;
	xmlNode *root;

	if ((CFG.api_key == NULL) || (*CFG.api_key == '\0')) {
		PCB_DAD_LABEL(octx->dlg, "Error: no api_key available.");
		return -1;
	}
	if (pcbway_cache_update(&PCB->hidlib) != 0) {
		PCB_DAD_LABEL(octx->dlg, "Error: failed to update the cache.");
		return -1;
	}

	cachedir = pcb_build_fn(&PCB->hidlib, conf_order.plugins.order.cache);
	path = pcb_strdup_printf("%s%cPCBWay_Api.xml", cachedir, PCB_DIR_SEPARATOR_C);
	doc = pcbway_xml_load(path);
	if (doc != NULL) {
		root = xmlDocGetRootElement(doc);
		if ((root != NULL) && (xmlStrcmp(root->name, (xmlChar *)"PCBWayAPI") == 0)) {
			if (pcbway_populate_dad_(&PCB->hidlib, imp, octx, root) != 0)
				PCB_DAD_LABEL(octx->dlg, "xml error: invalid API xml\n");
		}
		else
			PCB_DAD_LABEL(octx->dlg, "xml error: root is not <PCBWayAPI>\n");
	}
	else
		PCB_DAD_LABEL(octx->dlg, "xml error: failed to parse the xml\n");

	xmlFreeDoc(doc);
	free(cachedir);
	free(path);
}

static pcb_order_imp_t pcbway = {
	"PCBWay",
	NULL,
	pcbway_populate_dad
};


int pplg_check_ver_order_pcbway(int ver_needed) { return 0; }

void pplg_uninit_order_pcbway(void)
{
	pcb_conf_unreg_file(ORDER_PCBWAY_CONF_FN, order_pcbway_conf_internal);
	pcb_conf_unreg_fields("plugins/order_pcbway/");
}

int pplg_init_order_pcbway(void)
{
	PCB_API_CHK_VER;

	pcb_conf_reg_file(ORDER_PCBWAY_CONF_FN, order_pcbway_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_order_pcbway, field,isarray,type_name,cpath,cname,desc,flags);
#include "order_pcbway_conf_fields.h"

	pcb_order_reg(&pcbway);
	return 0;
}
