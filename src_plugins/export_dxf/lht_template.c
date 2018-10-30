/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

/* Generate output using a lihata template */

#ifndef TESTER
#include "config.h"
#endif


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <liblihata/dom.h>
#include <liblihata/tree.h>
#include "lht_template.h"

static void verbatim(FILE *f, const char *prefix, const char *value, int trim_indent)
{
	const char *s, *next;
	int indent_len = 0;
	if (*value == '\n')
		value++;

	if (trim_indent)
		for(s = value; isspace(*s); s++)
			indent_len++;

	for(s = value; *s != '\0'; s = next) {
		if (trim_indent) {
			int n;
			for(n = 0; (n < indent_len) && (isspace(*s)); n++) s++;
		}
		next = strpbrk(s, "\r\n");
		if (next == NULL) {
			fprintf(f, "%s%s", prefix, s);
			break;
		}
		while((*next == '\r') || (*next == '\n')) next++;
		fprintf(f, "%s", prefix);
		fwrite(s, 1, next-s, f);
	}
}

int lht_temp_exec(FILE *f, const char *prefix, lht_doc_t *doc, const char *name, lht_temp_insert_t ins, lht_err_t *err)
{
	lht_node_t *t, *n, *cfg;
	int trim_indent = 0;

	if (prefix == NULL)
		prefix = "";

	t = lht_tree_path(doc, "/template", name, 1, err);
	if (t == NULL)
		return -1;

	if (t->type != LHT_LIST) {
		*err = LHTE_INVALID_TYPE;
		return -1;
	}

	cfg = lht_tree_path(doc, "/", "trim_indent", 1, NULL);
	if ((cfg != NULL) && (cfg->type == LHT_TEXT) && (cfg->data.text.value != NULL))
		trim_indent = ((strcmp(cfg->data.text.value, "1") == 0) || (strcmp(cfg->data.text.value, "yes") == 0) || (strcmp(cfg->data.text.value, "true") == 0));

	for(n = t->data.list.first; n != NULL; n = n->next) {
		if (strcmp(n->name, "verbatim") == 0)
			verbatim(f, prefix, n->data.text.value, trim_indent);
		if (strcmp(n->name, "insert") == 0) {
			int res = ins(f, prefix, n->data.text.value, err);
			if (res != 0)
				return res;
		}
	}

	return 0;
}

#ifdef TESTER
int ins_(FILE *f, const char *prefix, char *name, lht_err_t *err)
{
	printf("INSERT: %s\n", name);
	return 0;
}

int main()
{
	lht_doc_t *doc;
	lht_err_t err;
	char *errmsg;
	const char *fn = "dxf_templ.lht";

	/* do not do safe_fs here - it's just a test code that never ends up in pcb_rnd */
	doc = lht_dom_load(fn, &errmsg);
	if (doc == NULL) {
		fprintf(stderr, "Error loading %s: %s\n", fn, errmsg);
		exit(1);
	}

	lht_temp_exec(stdout, "", doc, "header", ins_, &err);

	lht_dom_uninit(doc);
	return 0;
}
#endif
