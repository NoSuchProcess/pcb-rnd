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
				char *__end__, *__tmp__ = rnd_concat("$", val, NULL); \
				RND_DAD_LABEL(dlg, __tmp__); \
				free(__tmp__); \
				__prc__ = strtod(val, &__end__); \
				if (*__end__ == '\0') price = __prc__; \
			} \
			else \
				RND_DAD_LABEL(dlg, val); \
		} \
	} while(0)

static void pcbway_order_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	
}

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
						RND_DAD_CHANGE_CB(octx->dlg, pcbway_order_cb);
				RND_DAD_END(dlg);

	
			}
		RND_DAD_END(dlg);

		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_NEW("pcbway_quote", dlg, "PCBWay: quote", NULL, rnd_true, NULL);
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

	fx = rnd_fopen(&PCB->hidlib, tmpfn, "w");
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
				rnd_fprintf(fx, "%mm", f->val.crd);
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
		rnd_wget_opts_t wopts;

		wopts.header = (const char **)hdr;
		hdr[0] = rnd_concat("api-key: ", CFG.api_key, NULL);
		hdr[1] = "Content-Type: application/xml";
		hdr[2] = "Accept: application/xml";
		hdr[3] = NULL;
		wopts.post_file = tmpfn;

		if (rnd_wget_disk(SERVER "/api/Pcb/PcbQuotation", respfn, 0, &wopts) != 0) {
			rnd_message(RND_MSG_ERROR, "pcbway: failed to get a quote from the server\n");
			goto err;
		}
	}

	pcbway_present_quote(octx, respfn);

	err:;
	rnd_tempfile_unlink(tmpfn);
}
