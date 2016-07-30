/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "conf.h"
#include "misc.h"

#define LISTSEP " [[pcb-rnd]] "

/* Save and load pcb conf attributes, but do not save ::design:: (because they have flags) */
static const char *conf_attr_prefix = "PCB::conf::";
static const char *conf_attr_prefix_inhibit = "design::";
/* The optimizer will fix this */
#define conf_attr_prefix_len strlen(conf_attr_prefix)
#define conf_attr_prefix_inhibit_len strlen(conf_attr_prefix_inhibit)

static int path_ok(const char *path)
{
	if (strncmp(path, conf_attr_prefix, conf_attr_prefix_len) != 0)
		return 0;
	if (strncmp(path + conf_attr_prefix_len, conf_attr_prefix_inhibit, conf_attr_prefix_inhibit_len) == 0)
		return 0;
	return 1;
}

static void c2a(PCBType *pcb, lht_node_t *tree, char *path1)
{
	lht_dom_iterator_t it;
	lht_node_t *n;
	char apath[512], *path, *pe;
	int path1l = strlen(path1);

	memcpy(apath, conf_attr_prefix, conf_attr_prefix_len);
	path = apath + conf_attr_prefix_len;

	if (path1l != 0) {
		memcpy(path, path1, path1l);
		path[path1l] = '/';
		pe = path+path1l+1;
	}
	else
		pe = path;

	/* a depth-first-search and save config items from the tree */
	for(n = lht_dom_first(&it, tree); n != NULL; n = lht_dom_next(&it)) {
		strcpy(pe, n->name);
		if (n->type == LHT_HASH)
			c2a(pcb, n, path);
		if (strncmp(path, "design/",7) == 0) {
			continue;
		}
		if (n->type == LHT_TEXT) {
			conf_native_t *nv = conf_get_field(path);
			if ((nv != NULL) && (!nv->random_flags.io_pcb_no_attrib))
				AttributePutToList(&pcb->Attributes, apath, n->data.text.value, 1);
		}
		else if (n->type == LHT_LIST) {
			lht_node_t *i;
			conf_native_t *nv = conf_get_field(path);
			if ((nv != NULL) && (!nv->random_flags.io_pcb_no_attrib)) {
				gds_t conc;
				gds_init(&conc);
				for(i = n->data.list.first; i != NULL; i = i->next) {
					if (i != n->data.list.first)
						gds_append_str(&conc, LISTSEP);
					gds_append_str(&conc, i->data.text.value);
				}
				AttributePutToList(&pcb->Attributes, apath,  conc.array, 1);
				gds_uninit(&conc);
			}
		}
	}
}

void io_pcb_attrib_c2a(PCBType *pcb)
{
	lht_node_t *nmain = conf_lht_get_first(CFR_DESIGN);

	c2a(pcb, nmain, "");
}

void io_pcb_attrib_a2c(PCBType *pcb)
{
	int n;

	for (n = 0; n < pcb->Attributes.Number; n++) {
		if (path_ok(pcb->Attributes.List[n].name)) {
			conf_native_t *nv = conf_get_field(pcb->Attributes.List[n].name + conf_attr_prefix_len);
			if (nv == NULL)
				continue;
			if (nv->type == CFN_LIST) {
				char *tmp = strdup(pcb->Attributes.List[n].value);
				char *next, *curr;
				for(curr = tmp; curr != NULL; curr = next) {
					next = strstr(curr, LISTSEP);
					if (next != NULL) {
						*next = '\0';
						next += strlen(LISTSEP);
					}
					conf_set(CFR_DESIGN, pcb->Attributes.List[n].name + conf_attr_prefix_len, -1, curr, POL_APPEND);
				}
				free(tmp);
			}
			else /* assume plain string */
				conf_set(CFR_DESIGN, pcb->Attributes.List[n].name + conf_attr_prefix_len, -1, pcb->Attributes.List[n].value, POL_OVERWRITE);
		}
	}
}
