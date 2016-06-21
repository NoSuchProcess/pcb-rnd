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

static const char *conf_attr_prefix = "PCB::conf::";
/* The optimizer will fix this */
#define conf_attr_prefix_len strlen(conf_attr_prefix)

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

	/* a depth-first-search for symlinks or broken symlinks */
	for(n = lht_dom_first(&it, tree); n != NULL; n = lht_dom_next(&it)) {
		strcpy(pe, n->name);
		if (n->type == LHT_HASH)
			c2a(pcb, n, path);
		if (n->type == LHT_TEXT) {
			conf_native_t *nv = conf_get_field(path);
			if ((nv != NULL) && (!nv->random_flags.io_pcb_no_attrib)) {
				printf("C2A: '%s' %d\n", path, nv->random_flags.io_pcb_no_attrib);
				AttributePutToList(&pcb->Attributes, apath, n->data.text.value, 1);
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

	for (n = 0; n < pcb->Attributes.Number; n++)
		if (strncmp(pcb->Attributes.List[n].name, conf_attr_prefix, conf_attr_prefix_len) == 0)
			conf_set(CFR_DESIGN, pcb->Attributes.List[n].name + conf_attr_prefix_len, -1, pcb->Attributes.List[n].value, POL_OVERWRITE);
}
