/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

static int drc_query_lht_save_def(rnd_hidlib_t *hl, lht_node_t *ndefs, const char *def_id)
{
	lht_node_t *norig;
	if (pcb_drc_query_def_by_name(def_id, &norig, 0) != 0)
		return -1;
	lht_dom_list_append(ndefs, lht_dom_duptree(norig));
	return 0;
}

static int drc_query_lht_save_rule(rnd_hidlib_t *hl, const char *fn, const char *rule_id)
{
	htsi_t defs;
	htsi_entry_t *e;
	fgw_arg_t tmp;
	lht_doc_t *doc;
	lht_node_t *ndefs, *nrules, *norig;
	int res = 0;
	FILE *f;

	if (pcb_drc_query_get(hl, 1, rule_id, "query", &tmp) != 0)
		return -1;

	f = rnd_fopen(hl, fn, "w");
	if (f == NULL)
		return -1;

	doc = lht_dom_init();
	doc->root = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-drc-query-v1");
	ndefs = lht_dom_node_alloc(LHT_LIST, "definitions");
	nrules = lht_dom_node_alloc(LHT_LIST, "rules");
	lht_dom_list_append(doc->root, ndefs);
	lht_dom_list_append(doc->root, nrules);

	htsi_init(&defs, strhash, strkeyeq);
	pcb_qry_extract_defs(&defs, tmp.val.str);
	for(e = htsi_first(&defs); e != NULL; e = htsi_next(&defs, e)) {
		if (drc_query_lht_save_def(hl, ndefs, e->key) != 0)
			res = -1;
		free(e->key);
	}
	htsi_uninit(&defs);

	if (pcb_drc_query_rule_by_name(rule_id, &norig, 0) == 0) {
		lht_dom_list_append(nrules, lht_dom_duptree(norig));
		res = lht_dom_export(doc->root, f, "");
	}
	else
		res = -1;

	fclose(f);
	lht_dom_uninit(doc);

	return res;
}
