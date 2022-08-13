/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - stackup import/export
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
#include <genht/htsp.h>
#include <genht/hash.h>
#include "data.h"
#include "tlayer.h"
#include <librnd/core/safe_fs.h>
#include <librnd/core/error.h>
#include "parse.h"
#include <librnd/core/compat_misc.h>
#include "obj_line.h"
#include "obj_arc.h"
#include "obj_poly.h"
#include <librnd/core/vtc0.h>
#include "plug_io.h"
#include "obj_pstk_inlines.h"

#include "common_inlines.h"

#define LAYERNET(obj) if (nmap != NULL) tedax_finsert_layernet_tags(f, nmap, (pcb_any_obj_t *)obj)

static void tedax_layer_fsave_pstk_polys(pcb_board_t *pcb, pcb_data_t *data, pcb_layer_t *ly, FILE *f)
{
	PCB_PADSTACK_LOOP(data) {
		pcb_pstk_shape_t *shp = pcb_pstk_shape_at(pcb, padstack, ly);
		if ((shp != NULL) && (shp->shape == PCB_PSSH_POLY)) {
			int n;
			fprintf(f, "begin polyline v1 pstk_%p_%p\n", (void *)padstack, (void *)ly);
			for(n = 0; n < shp->data.poly.len; n++)
				rnd_fprintf(f, " v %.06mm %.06mm\n", shp->data.poly.x[n], shp->data.poly.y[n]);
			fprintf(f, "end polyline\n");
		}
	}
	PCB_END_LOOP;

	PCB_SUBC_LOOP(data) {
		tedax_layer_fsave_pstk_polys(pcb, subc->data, ly, f);
	}
	PCB_END_LOOP;
}

static void tedax_layer_fsave_pstk_shape(pcb_board_t *pcb, pcb_data_t *data, pcb_layer_t *ly, FILE *f, pcb_netmap_t *nmap)
{
	PCB_PADSTACK_LOOP(data) {
		pcb_pstk_shape_t *shp = pcb_pstk_shape_at(pcb, padstack, ly);
		if (shp == NULL)
			continue;
		switch(shp->shape) {
			case PCB_PSSH_HSHADOW: /* no shape drawn */ break;
			case PCB_PSSH_LINE:
				rnd_fprintf(f, " line");
				LAYERNET(padstack);
				rnd_fprintf(f, " %.06mm %.06mm %.06mm %.06mm %.06mm %.06mm\n",
					padstack->x + shp->data.line.x1, padstack->y + shp->data.line.y1,
					padstack->x + shp->data.line.x2, padstack->y + shp->data.line.y2,
					shp->data.line.thickness, PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, padstack) ? (rnd_coord_t)rnd_round(padstack->Clearance/2) : 0);
				break;
			case PCB_PSSH_CIRC:
				rnd_fprintf(f, " line");
				LAYERNET(padstack);
				rnd_fprintf(f, " %.06mm %.06mm %.06mm %.06mm %.06mm %.06mm\n",
					padstack->x + shp->data.circ.x, padstack->y + shp->data.circ.y,
					padstack->x + shp->data.circ.x, padstack->y + shp->data.circ.y,
					shp->data.circ.dia, PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, padstack) ? (rnd_coord_t)rnd_round(padstack->Clearance/2) : 0);
				break;
			case PCB_PSSH_POLY:
				rnd_fprintf(f, " poly");
				LAYERNET(padstack);
				rnd_fprintf(f, " pstk_%p_%p %.06mm %.06mm\n", (void *)padstack, (void *)ly, padstack->x, padstack->y);
				break;
		}
	}
	PCB_END_LOOP;

	PCB_SUBC_LOOP(data) {
		tedax_layer_fsave_pstk_shape(pcb, subc->data, ly, f, nmap);
	}
	PCB_END_LOOP;
}

int tedax_layer_fsave(pcb_board_t *pcb, rnd_layergrp_id_t gid, const char *layname, FILE *f, pcb_netmap_t *nmap)
{
	char lntmp[64];
	int lno;
	rnd_pline_t *pl;
	long plid;
	const char *blockid = ((nmap == NULL) ? "layer" : "layernet");
	pcb_layergrp_t *g = pcb_get_layergrp(pcb, gid);

	if (g == NULL)
		return -1;

	if (layname == NULL)
		layname = g->name;
	if (layname == NULL) {
		layname = lntmp;
		sprintf(lntmp, "anon_%ld", gid);
	}

	for(lno = 0; lno < g->len; lno++) {
		pcb_layer_t *ly = pcb_get_layer(pcb->Data, g->lid[lno]);
		if (ly == NULL)
			continue;

		PCB_POLY_LOOP(ly) {
			rnd_pline_t *pl;
			if (!polygon->NoHolesValid)
				pcb_poly_compute_no_holes(polygon);

			for(pl = polygon->NoHoles, plid = 0; pl != NULL; pl = pl->next, plid++) {
				rnd_vnode_t *v;
				long i, n;
				fprintf(f, "begin polyline v1 pllay_%ld_%ld_%ld\n", gid, polygon->ID, plid);
				n = pl->Count;
				for(v = pl->head, i = 0; i < n; v = v->next, i++)
					rnd_fprintf(f, " v %.06mm %.06mm\n", v->point[0], v->point[1]);
				fprintf(f, "end polyline\n");
			}
		} PCB_END_LOOP;

		if (nmap != NULL) { /* text objets have to be approximated with a rectangular polygon "keepout" */
			PCB_TEXT_LOOP(ly) {
				fprintf(f, "begin polyline v1 txlay_%ld\n", text->ID);
				rnd_fprintf(f, " v %.06mm %.06mm\n", text->bbox_naked.X1, text->bbox_naked.Y1);
				rnd_fprintf(f, " v %.06mm %.06mm\n", text->bbox_naked.X1, text->bbox_naked.Y2);
				rnd_fprintf(f, " v %.06mm %.06mm\n", text->bbox_naked.X2, text->bbox_naked.Y2);
				rnd_fprintf(f, " v %.06mm %.06mm\n", text->bbox_naked.X2, text->bbox_naked.Y1);
				fprintf(f, "end polyline\n");
			} PCB_END_LOOP;

			if (lno == 0)
				tedax_layer_fsave_pstk_polys(pcb, pcb->Data, ly, f);
		}
	}

	fprintf(f, "begin %s v1 %s\n", blockid, layname);
	for(lno = 0; lno < g->len; lno++) {
		pcb_layer_t *ly = pcb_get_layer(pcb->Data, g->lid[lno]);
		if (ly == NULL)
			continue;
		PCB_LINE_LOOP(ly) {
			rnd_fprintf(f, " line");
			LAYERNET(line);
			rnd_fprintf(f, " %.06mm %.06mm %.06mm %.06mm %.06mm %.06mm\n",
				line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y,
				line->Thickness, PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, line) ? rnd_round(line->Clearance/2) : 0);
		} PCB_END_LOOP;
		PCB_ARC_LOOP(ly) {
			rnd_coord_t sx, sy, ex, ey, clr;

			if (arc->Width != arc->Height)
				pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)arc, "arc", "Elliptical arc", "Saving as circular arc instead - geometry will differ");

			pcb_arc_get_end(arc, 0, &sx, &sy);
			pcb_arc_get_end(arc, 1, &ex, &ey);
			clr = PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, arc) ? rnd_round(arc->Clearance/2) : 0;
			rnd_fprintf(f, " arc");
			LAYERNET(arc);
			rnd_fprintf(f, " %.06mm %.06mm %.06mm %f %f %.06mm %.06mm ",
				arc->X, arc->Y, arc->Width, arc->StartAngle, arc->Delta, arc->Thickness, clr);
			rnd_fprintf(f, "%.06mm %.06mm %.06mm %.06mm\n", sx, sy, ex, ey);
		} PCB_END_LOOP;
		{
			pcb_gfx_t *gfx;
			for(gfx = gfxlist_first(&ly->Gfx); gfx != NULL; gfx = gfxlist_next(gfx))
				pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)gfx, "gfx", "gfx can not be exported", "please use the lihata board format");
		}

		if (nmap == NULL) {
			PCB_TEXT_LOOP(ly) {
				int scale;
				if (pcb_text_old_scale(text, &scale) != 0)
					pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)text, "text-scale", "file format does not support different x and y direction text scale - using average scale", "Use the scale field, set scale_x and scale_y to 0");
				if (text->mirror_x)
					pcb_io_incompat_save(NULL, (pcb_any_obj_t *)text, "text-mirror-x", "file format does not support different mirroring text in the x direction", "do not mirror, or mirror in the y direction (with the ONSOLDER flag)");
				rnd_fprintf(f, " text %.06mm %.06mm %.06mm %.06mm %d %f %.06mm ",
					text->bbox_naked.X1, text->bbox_naked.Y1, text->bbox_naked.X2, text->bbox_naked.Y2,
					scale, text->rot, PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text) ? 1 : 0);
				tedax_fprint_escape(f, text->TextString);
				fputc('\n', f);
			} PCB_END_LOOP;
		}
		else {
			PCB_TEXT_LOOP(ly) {
				rnd_fprintf(f, " poly");
				LAYERNET(text);
				rnd_fprintf(f, " txlay_%ld 0 0\n", text->ID);
			} PCB_END_LOOP;
		}

		PCB_POLY_LOOP(ly) {
			for(pl = polygon->NoHoles, plid = 0; pl != NULL; pl = pl->next, plid++) {
				rnd_fprintf(f, " poly");
				LAYERNET(polygon);
				rnd_fprintf(f, " pllay_%ld_%ld_%ld 0 0\n", gid, polygon->ID, plid);
			}
		} PCB_END_LOOP;
		if (lno == 0)
			tedax_layer_fsave_pstk_shape(pcb, pcb->Data, ly, f, nmap);
	}
	fprintf(f, "end %s\n", blockid);
	return 0;
}

int tedax_layer_save(pcb_board_t *pcb, rnd_layergrp_id_t gid, const char *layname, const char *fn)
{
	int res;
	FILE *f;

	f = rnd_fopen_askovr(&PCB->hidlib, fn, "w", NULL);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_layer_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_layer_fsave(pcb, gid, layname, f, NULL);
	fclose(f);
	return res;
}

int tedax_layers_fload(pcb_data_t *data, FILE *f, const char *blk_id, int silent)
{
	long start, n;
	int argc, res = 0;
	char line[520];
	char *argv[16], *end;
	htsp_t plines;
	htsp_entry_t *e;
	vtc0_t *coords;

	if (tedax_seek_hdr(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	start = ftell(f);

	htsp_init(&plines, strhash, strkeyeq);
	while((argc = tedax_seek_block(f, "polyline", "v1", NULL, 1, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) > 1) {
		char *pname;
		if (htsp_has(&plines, argv[3])) {
			rnd_message(RND_MSG_ERROR, "duplicate polyline: %s\n", argv[3]);
			res = -1;
			goto error;
		}
		pname = rnd_strdup(argv[3]);
		coords = malloc(sizeof(vtc0_t));
		vtc0_init(coords);

		while((argc = tedax_getline(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {

			if ((argc == 3) && (strcmp(argv[0], "v") == 0)) {
				rnd_bool s1, s2;
				vtc0_append(coords, rnd_get_value(argv[1], "mm", NULL, &s1));
				vtc0_append(coords, rnd_get_value(argv[2], "mm", NULL, &s2));
				if (!s1 || !s2) {
					rnd_message(RND_MSG_ERROR, "invalid coords in polyline %s: %s;%s\n", pname, argv[1], argv[2]);
					res = -1;
					free(pname);
					goto error;
				}
			}
			else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "polyline") == 0)) {
				break;
			}
			else {
				rnd_message(RND_MSG_ERROR, "invalid command in polyline %s: %s\n", pname, argv[0]);
				res = -1;
				free(pname);
				goto error;
			}
		}

		htsp_insert(&plines, pname, coords);
	}

	fseek(f, start, SEEK_SET);

	while((argc = tedax_seek_block(f, "layer", "v1", blk_id, silent, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) > 1) {
		pcb_layer_t *ly;
		if (data->LayerN >= PCB_MAX_LAYER) {
			rnd_message(RND_MSG_ERROR, "too many layers\n");
			res = -1;
			goto error;
		}
		ly = &data->Layer[data->LayerN++];
		free((char *)ly->name);
		ly->name = rnd_strdup(argv[3]);
		ly->is_bound = data->parent_type != PCB_PARENT_BOARD;
		if (!ly->is_bound) {
			/* real layer */
			memset(&ly->meta.real, 0, sizeof(ly->meta.real));
			ly->meta.real.grp = -1; /* group insertion is to be done by the caller */
			ly->meta.real.vis = 1;
		}
		else
			memset(&ly->meta.bound, 0, sizeof(ly->meta.bound));

		while((argc = tedax_getline(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
			if ((argc == 7) && (strcmp(argv[0], "line") == 0)) {
				rnd_coord_t x1, y1, x2, y2, th, cl;
				rnd_bool s1, s2, s3, s4;

				x1 = rnd_get_value(argv[1], "mm", NULL, &s1);
				y1 = rnd_get_value(argv[2], "mm", NULL, &s2);
				x2 = rnd_get_value(argv[3], "mm", NULL, &s3);
				y2 = rnd_get_value(argv[4], "mm", NULL, &s4);
				if (!s1 || !s2 || !s3 || !s4) {
					rnd_message(RND_MSG_ERROR, "invalid line coords in line: %s;%s %s;%s\n", argv[1], argv[2], argv[3], argv[4]);
					res = -1;
					goto error;
				}
				th = rnd_get_value(argv[5], "mm", NULL, &s1);
				cl = rnd_get_value(argv[6], "mm", NULL, &s2);
				if (!s1 || !s2) {
					rnd_message(RND_MSG_ERROR, "invalid thickness or clearance in line: %s;%s\n", argv[5], argv[6]);
					res = -1;
					goto error;
				}
				pcb_line_new_merge(ly, x1, y1, x2, y2, th, cl*2, pcb_flag_make(PCB_FLAG_CLEARLINE));
			}
			else if ((argc == 12) && (strcmp(argv[0], "arc") == 0)) {
				rnd_coord_t cx, cy, r, th, cl;
				double sa, da;
				rnd_bool s1, s2, s3;

				cx = rnd_get_value(argv[1], "mm", NULL, &s1);
				cy = rnd_get_value(argv[2], "mm", NULL, &s2);
				r  = rnd_get_value(argv[3], "mm", NULL, &s3);
				if (!s1 || !s2 || !s3) {
					rnd_message(RND_MSG_ERROR, "invalid arc coords or radius in line: %s;%s %s\n", argv[1], argv[2], argv[3]);
					res = -1;
					goto error;
				}
				sa = strtod(argv[4], &end);
				if ((*end != '\0') || (sa < 0) || (sa >= 360.0)) {
					rnd_message(RND_MSG_ERROR, "invalid arc start angle %s\n", argv[4]);
					res = -1;
					goto error;
				}
				da = strtod(argv[5], &end);
				if ((*end != '\0') || (da < -360.0) || (da > 360.0)) {
					rnd_message(RND_MSG_ERROR, "invalid arc delta angle %s\n", argv[5]);
					res = -1;
					goto error;
				}
				th = rnd_get_value(argv[6], "mm", NULL, &s1);
				cl = rnd_get_value(argv[7], "mm", NULL, &s2);
				if (!s1 || !s2) {
					rnd_message(RND_MSG_ERROR, "invalid thickness or clearance in arc: %s;%s\n", argv[6], argv[7]);
					res = -1;
					goto error;
				}
				pcb_arc_new(ly, cx, cy, r, r, sa, da, th, cl*2, pcb_flag_make(PCB_FLAG_CLEARLINE), 1);
			}
			else if ((argc == 9) && (strcmp(argv[0], "text") == 0)) {
				rnd_coord_t bx1, by1, bx2, by2, rw, rh, aw, ah;
				rnd_bool s1, s2, s3, s4;
				double rot, zx, zy, z;
				pcb_text_t *text;

				bx1 = rnd_get_value(argv[1], "mm", NULL, &s1);
				by1 = rnd_get_value(argv[2], "mm", NULL, &s2);
				bx2 = rnd_get_value(argv[3], "mm", NULL, &s3);
				by2 = rnd_get_value(argv[4], "mm", NULL, &s4);
				if (!s1 || !s2 || !s3 || !s4) {
					rnd_message(RND_MSG_ERROR, "invalid bbox coords in text %s;%s %s;%s \n", argv[1], argv[2], argv[3], argv[4]);
					res = -1;
					goto error;
				}
				rot = strtod(argv[6], &end);
				if (*end != '\0') {
					rnd_message(RND_MSG_ERROR, "invalid text rotation %s \n", argv[6]);
					res = -1;
					goto error;
				}
				if (rot < 0)
					rot += 360;
				text = pcb_text_new(ly, pcb_font(PCB, 0, 1), bx1, by1, rot, 100, 0, argv[8], pcb_flag_make(PCB_FLAG_CLEARLINE));
				rw = bx2-bx1; rh = by2-by1;
				pcb_text_pre(text);
				for(n = 0; n < 8; n++) {
					pcb_text_bbox(pcb_font(PCB, 0, 1), text);
					aw = text->bbox_naked.X2 - text->bbox_naked.X1; ah = text->bbox_naked.Y2 - text->bbox_naked.Y1;
					zx = (double)rw/(double)aw; zy = (double)rh/(double)ah;
					z = zx < zy ? zx : zy;
					if ((z > 0.999) && (z < 1.001))
						break;
					text->Scale = rnd_round(text->Scale*z);
				}
				pcb_text_bbox(pcb_font(PCB, 0, 1), text);
				aw = text->bbox_naked.X2 - text->bbox_naked.X1; ah = text->bbox_naked.Y2 - text->bbox_naked.Y1;
				text->X += (double)(rw-aw)/2.0;
				text->Y += (double)(rh-ah)/2.0;
				pcb_text_post(text);

			}
			else if ((argc == 4) && (strcmp(argv[0], "poly") == 0)) {
				rnd_bool s1, s2;
				rnd_coord_t ox, oy;
				pcb_poly_t *poly;

				ox = rnd_get_value(argv[2], "mm", NULL, &s1);
				oy = rnd_get_value(argv[3], "mm", NULL, &s2);
				if (!s1 || !s2) {
					rnd_message(RND_MSG_ERROR, "invalid coords in poly %s;%s\n", argv[2], argv[3]);
					res = -1;
					goto error;
				}
				coords = htsp_get(&plines, argv[1]);
				if (coords == NULL) {
					rnd_message(RND_MSG_ERROR, "invalid polyline referecnce %s\n", argv[1]);
					res = -1;
					goto error;
				}
				rnd_trace("POLY: %mm %mm %s\n", ox, oy, argv[1]);
				poly = pcb_poly_new(ly, 0, pcb_no_flags());
				for(n = 0; n < coords->used; n+=2)
					pcb_poly_point_new(poly, ox+coords->array[n], oy+coords->array[n+1]);
				pcb_add_poly_on_layer(ly, poly);
			}
			else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "layer") == 0))
				break;
			else {
				rnd_message(RND_MSG_ERROR, "invalid layer object %s\n", argv[0]);
				res = -1;
				goto error;
			}
		}
	}


	error:;
	for(e = htsp_first(&plines); e != NULL; e = htsp_next(&plines, e)) {
		free(e->key);
		vtc0_uninit(e->value);
		free(e->value);
	}
	htsp_uninit(&plines);
	return res;
}

int tedax_layers_load(pcb_data_t *data, const char *fn, const char *blk_id, int silent)
{
	int res;
	FILE *f;

	f = rnd_fopen(&PCB->hidlib, fn, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "tedax_layers_load(): can't open %s for reading\n", fn);
		return -1;
	}
	res = tedax_layers_fload(data, f, blk_id, silent);
	fclose(f);
	return res;
}

