/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <stdio.h>

#ifdef PCB_HAVE_LIBXML2
#include <libxml/tree.h>
#include <libxml/parser.h>

static void amf_load_material(xmlNode *material)
{
	rnd_trace("amf material\n");
}

static int amf_load_double(double *dst, xmlNode *node)
{
	xmlNode *n;
	for(n = node->children; n != NULL; n = n->next) {
		if (n->type == XML_TEXT_NODE) {
			char *end;
			*dst = strtod((char *)n->content, &end);
			return (*end == '\0') ? 0 : -1;
		}
	}
	return -1;
}

static int amf_load_long(long *dst, xmlNode *node)
{
	xmlNode *n;
	for(n = node->children; n != NULL; n = n->next) {
		if (n->type == XML_TEXT_NODE) {
			char *end;
			*dst = strtol((char *)n->content, &end, 10);
			return (*end == '\0') ? 0 : -1;
		}
	}
	return -1;
}

static void amf_load_vertices(vtd0_t *dst, xmlNode *vertices)
{
	xmlNode *n, *m;
	for(n = vertices->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"coordinates") == 0) {
			double x = 0, y = 0, z = 0;
			for(m = n->children; m != NULL; m = m->next) {
				if (m->name[1] == '\0') {
					switch(m->name[0]) {
						case 'x': amf_load_double(&x, m); break;
						case 'y': amf_load_double(&y, m); break;
						case 'z': amf_load_double(&z, m); break;
					}
				}
			}
			vtd0_append(dst, x);
			vtd0_append(dst, y);
			vtd0_append(dst, z);
		}
	}
}

static void stl_triangle_normal(stl_facet_t *t)
{
	double x1 = t->vx[0], y1 = t->vy[0], z1 = t->vz[0];
	double x2 = t->vx[1], y2 = t->vy[1], z2 = t->vz[1];
	double x3 = t->vx[2], y3 = t->vy[2], z3 = t->vz[2];
	double len;

	t->n[0] = (y2-y1)*(z3-z1) - (y3-y1)*(z2-z1);
	t->n[1] = (z2-z1)*(x3-x1) - (x2-x1)*(z3-z1);
	t->n[2] = (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1);

	len = sqrt(t->n[0] * t->n[0] + t->n[1] * t->n[1] + t->n[2] * t->n[2]);
	if (len == 0) return;

	t->n[0] /= len;
	t->n[1] /= len;
	t->n[2] /= len;
}

static stl_facet_t *amf_load_volume(vtd0_t *verts, xmlNode *volume)
{
	xmlNode *n, *m;
	stl_facet_t *t, *head = NULL, *tail = NULL;

	for(n = volume->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"triangle") == 0) {
			long v[3] = {-1, -1, -1};
			int i, good;

			for(m = n->children; m != NULL; m = m->next) {
				if ((m->name[0] == 'v') && (m->name[2] == '\0')) {
					int idx = m->name[1] - '1';
					if ((idx >= 0) && (idx < 3))
						amf_load_long(&v[idx], m);
				}
			}

			for(i = 0, good = 1; i < 3; i++)
				if ((v[i] < 0) || (v[i]*3 >= verts->used))
					good = 0;

			if (!good)
				continue;

			t = malloc(sizeof(stl_facet_t));
			t->next = NULL;
			for(i = 0, good = 1; i < 3; i++) {
				double *crd = verts->array + v[i] * 3;
				t->vx[i] = crd[0];
				t->vy[i] = crd[1];
				t->vz[i] = crd[2];
			}
			stl_triangle_normal(t);

			if (tail != NULL) {
				tail->next = t;
				tail = t;
			}
			else
				head = tail = t;
		}
	}
	return head;
}

static stl_facet_t *amf_load_mesh(xmlNode *mesh)
{
	xmlNode *n, *m;
	vtd0_t verts = {0};
	stl_facet_t *t, *head = NULL, *tail = NULL;

	/* load vertices */
	for(n = mesh->children; n != NULL; n = n->next)
		if (xmlStrcmp(n->name, (xmlChar *)"vertices") == 0)
			for(m = n->children; m != NULL; m = m->next)
				if (xmlStrcmp(m->name, (xmlChar *)"vertex") == 0)
					amf_load_vertices(&verts, m);

	if (verts.used == 0)
		return NULL;

	/* load volumes */
	for(n = mesh->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"volume") == 0) {
			t = amf_load_volume(&verts, n);
			if (t == NULL) continue;
			if (tail != NULL) {
				tail->next = t;
				tail = t;
			}
			else
				head = tail = t;
		}
	}

	return head;
}

static stl_facet_t *amf_solid_fload(rnd_hidlib_t *hl, FILE *f, const char *fn)
{
	xmlDoc *doc;
	xmlNode *root, *n, *m;
	stl_facet_t *t, *head = NULL, *tail = NULL;

	doc = xmlReadFile(fn, NULL, 0);
	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "amf xml parsing error on file %s\n", fn);
		return NULL;
	}

	root = xmlDocGetRootElement(doc);
	if (xmlStrcmp(root->name, (xmlChar *)"amf") != 0) {
		rnd_message(RND_MSG_ERROR, "amf xml error on file %s: root is not <amf>\n", fn);
		xmlFreeDoc(doc);
		return NULL;
	}

	/* read all materials */
	for(n = root->children; n != NULL; n = n->next)
		if (xmlStrcmp(n->name, (xmlChar *)"material") == 0)
			amf_load_material(n);

	/* read all volumes */
	for(n = root->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"object") == 0) {
			for(m = n->children; m != NULL; m = m->next) {
				if (xmlStrcmp(m->name, (xmlChar *)"mesh") == 0) {
					t = amf_load_mesh(m);
					if (t == NULL) continue;
					if (tail != NULL) {
						tail->next = t;
						tail = t;
					}
					else
						head = tail = t;
				}
			}
		}
	}

	xmlFreeDoc(doc);
	return head;
}



#else

/* Fallback: still provide a dummy if libxml is not available */
static stl_facet_t *amf_solid_fload(rnd_hidlib_t *hl, FILE *f, const char *fn)
{
	return &stl_format_not_supported;
}

#endif
