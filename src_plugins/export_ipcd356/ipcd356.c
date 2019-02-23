/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  ipc-d-356 export plugin
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

#include "board.h"
#include "data.h"
#include "safe_fs.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "netlist2.h"
#include "math_helper.h"
#include "layer.h"
#include "obj_arc.h"
#include "obj_line.h"
#include "obj_poly.h"
#include "obj_subc.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_cam.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "plugins.h"


#include "brave.h"
#include "netlist.h"

static const char *ipcd356_cookie = "ipcd356 exporter";

/*** low level export code ***/
typedef struct {
	pcb_board_t *pcb;
	FILE *f;
	int is_mil;
} write_ctx_t;

typedef struct {
	pcb_any_obj_t *o;
	const char *netname, *refdes, *term;
	int is_plated, access_top, access_bottom, rot, masked_top, masked_bottom;
	pcb_coord_t hole, width, height, cx, cy;
} test_feature_t;

static int getattr(pcb_any_obj_t *o, const char *key)
{
	char *val = pcb_attribute_get(&o->Attributes, key);
	if (val == NULL) return 0;
	if (pcb_strcasecmp(val, "yes") == 0) return 1;
	return 0;
}

static void fill_field(char *dst, int start, int end, const char *data, const char *name)
{
	int n;
	const char *d = data;

	if (data == NULL)
		d = "";

	for(n = start; n <= end; n++) {
		if (*d != '\0')
			dst[n] = *d++;
		else
			dst[n] = ' ';
	}
	if (*d != '\0')
		pcb_message(PCB_MSG_WARNING, "Data '%s' is too long for a(n) %s, truncated\n", data, name);
}

static void fill_field_coord(write_ctx_t *ctx, char *dst, int start, int end, pcb_coord_t crd, int sign, const char *name)
{
	int len = end-start+1;
	char tmp[32], fmt[16];
	if (sign) {
		dst[start] =  (crd >= 0) ? '+' : '-';
		start++;
		len--;
	}
	if (ctx->is_mil) {
		sprintf(fmt, "%%0%d.0ml", len);
		crd *= 10;
	}
	else {
		sprintf(fmt, "%%0%d.0mm", len);
		crd *= 1000;
	}
	pcb_snprintf(tmp, sizeof(tmp), fmt, crd);
	fill_field(dst, start, end, tmp, name);
}

static void ipcd356_write_head(write_ctx_t *ctx)
{
	char utc[64];

	pcb_print_utc(utc, sizeof(utc), 0);

	fprintf(ctx->f, "C  IPC-D-356 Netlist generated by pcb-rnd " PCB_VERSION "\n");
	fprintf(ctx->f, "C  \n");
	fprintf(ctx->f, "C  File created on %s\n", utc);
	fprintf(ctx->f, "C  \n");
	fprintf(ctx->f, "P  JOB   %s\n", (PCB->Name == NULL) ? PCB->Filename : PCB->Name);
	fprintf(ctx->f, "P  CODE  00\n");
	fprintf(ctx->f, "P  UNITS CUST %d\n", ctx->is_mil ? 0 : 1);
	fprintf(ctx->f, "P  DIM   N\n");
	fprintf(ctx->f, "P  VER   IPC-D-356\n");
	fprintf(ctx->f, "P  IMAGE PRIMARY\n");
	fprintf(ctx->f, "C  \n");
}

static void ipcd356_write_foot(write_ctx_t *ctx)
{
	fprintf(ctx->f, "999\n\n");
}

static void ipcd356_write_feature(write_ctx_t *ctx, test_feature_t *t)
{
	char line[128];
	int is_tooling, is_mid;

	is_tooling = getattr(t->o, "ipcd356::tooling");
	is_mid = getattr(t->o, "ipcd356::mid");

	line[0] = '3';
	if (is_tooling)
		line[1] = '4';
	else if (t->hole > 0)
		line[1] = '1';
	else
		line[1] = '2';
	line[2] = '7';

	fill_field(line, 3, 16, t->netname, "netname");
	fill_field(line, 17, 19, NULL, NULL);
	fill_field(line, 20, 25, t->refdes, "refdes");
	line[26] = '-';
	fill_field(line, 27, 30, t->term, "term ID");
	line[31] = is_mid ? 'M' : ' ';

	if (t->hole > 0) {
		line[32] = 'D';
		fill_field_coord(ctx, line, 33, 36, t->hole, 0, "hole");
	}
	else
		fill_field(line, 32, 36, NULL, NULL);

	if (t->hole > 0)
		line[37] = t->is_plated ? 'P' : 'U';
	else
		line[37] = ' ';

	if ((t->access_top) && (t->access_bottom))
		strcpy(line+38, "A00");
	else if (t->access_top)
		strcpy(line+38, "A01");
	else if (t->access_bottom)
		strcpy(line+38, "A02");
	else
		return; /* do not export something that's not accessible from top or bottom */

	line[41] = 'X';
	fill_field_coord(ctx, line, 42, 48, t->cx, 1, "X coord");
	line[49] = 'Y';
	fill_field_coord(ctx, line, 50, 56, PCB->MaxHeight - t->cy, 1, "Y coord");

	line[57] = 'X';
	fill_field_coord(ctx, line, 58, 61, t->width, 0, "width");
	line[62] = 'Y';
	fill_field_coord(ctx, line, 63, 66, t->height, 0, "height");
	line[67] = 'R';
	fill_field_coord(ctx, line, 68, 70, t->rot, 0, "rotation");

	line[71] = ' ';
	if ((t->masked_top) && (t->masked_bottom))
		strcpy(line+72, "S3");
	else if (t->masked_top)
		strcpy(line+72, "S1");
	else if (t->masked_bottom)
		strcpy(line+72, "S2");
	else
		strcpy(line+72, "S0");
	fill_field(line, 74, 79, NULL, NULL);

	line[80] = '\n';
	line[81] = '\0';
	fprintf(ctx->f, "%s", line);
}

/* light terminals */
static void ipcd356_pstk_shape(test_feature_t *t, pcb_pstk_shape_t *sh)
{
	int n;
	pcb_coord_t x1, y1, x2, y2;
	switch(sh->shape) {
		case PCB_PSSH_HSHADOW:
			break;
		case PCB_PSSH_CIRC:
			t->width = sh->data.circ.dia;
			t->height = 0;
			t->cx += sh->data.circ.x;
			t->cy += sh->data.circ.y;
			break;
		case PCB_PSSH_LINE:
			t->height = t->width = sh->data.line.thickness;
			t->cx += (sh->data.line.x1 + sh->data.line.x2)/2;
			t->cy += (sh->data.line.y1 + sh->data.line.y2)/2;
			break;
		case PCB_PSSH_POLY:
			t->height = t->width = sh->data.line.thickness;
			x1 = x2 = sh->data.poly.x[0];
			y1 = y2 = sh->data.poly.y[0];
			for(n = 1; n < sh->data.poly.len; n++) {
				if (sh->data.poly.x[n] < x1) x1 = sh->data.poly.x[n];
				if (sh->data.poly.x[n] > x2) x2 = sh->data.poly.x[n];
				if (sh->data.poly.y[n] < y1) y1 = sh->data.poly.y[n];
				if (sh->data.poly.y[n] > y2) y2 = sh->data.poly.y[n];
			}

			t->cx += (x1 + x2)/2;
			t->cy += (y1 + y2)/2;
			if (pcb_pline_is_rectangle(sh->data.poly.pa->contours)) {
				t->width = (x2 - x1);
				t->height = (y2 - y1);
			}
			else {
				t->width = (x2 - x1) / 4 + 1;
				t->height = (y2 - y1) / 4 + 1;
			}
			break;
	}
}

static void ipcd356_write_pstk(write_ctx_t *ctx, pcb_subc_t *subc, pcb_pstk_t *pstk)
{
	test_feature_t t;
	pcb_pstk_proto_t *proto;
	pcb_pstk_shape_t *st, *sb;
	pcb_pstk_tshape_t *tshp;

	if (pstk->term == NULL)
		return;

	proto = pcb_pstk_get_proto(pstk);
	if (proto == NULL)
		return;

	if (!(pcb_brave & PCB_BRAVE_OLD_NETLIST)) {
		pcb_net_term_t *term = pcb_net_find_by_obj(&ctx->pcb->netlist[PCB_NETLIST_EDITED], (pcb_any_obj_t *)pstk);
		t.netname = (term == NULL) ? "N/C" : term->parent.net->name;
	}
	else {
		pcb_lib_menu_t *mn = pcb_netlist_find_net4term(ctx->pcb, (pcb_any_obj_t *)pstk);
		t.netname = (mn == NULL) ? "N/C" : pcb_netlist_name(mn);
	}

	t.o = (pcb_any_obj_t *)pstk;
	t.refdes = subc->refdes;
	t.term = pstk->term;
	t.is_plated = proto->hplated;
	t.access_top = ((st = pcb_pstk_shape(pstk, PCB_LYT_TOP | PCB_LYT_COPPER, 0)) != NULL);
	t.access_bottom = ((sb = pcb_pstk_shape(pstk, PCB_LYT_BOTTOM | PCB_LYT_COPPER, 0)) != NULL);
	/* this assumes the mask shape will expose all copper */
	t.masked_top = (pcb_pstk_shape(pstk, PCB_LYT_TOP | PCB_LYT_MASK, 0) == NULL);
	t.masked_bottom = (pcb_pstk_shape(pstk, PCB_LYT_BOTTOM | PCB_LYT_MASK, 0) == NULL);
	t.hole = proto->hdia;
	t.cx = pstk->x;
	t.cy = pstk->y;

	/* this assumes bottom shape is not smaller than top shape */
	if (st != NULL) ipcd356_pstk_shape(&t, st);
	else if (sb != NULL) ipcd356_pstk_shape(&t, sb);

	tshp = pcb_pstk_get_tshape(pstk);
	t.rot = tshp->rot;

	ipcd356_write_feature(ctx, &t);
}

/* heavy terminals: lines, arcs, polygons */
static int ipcd356_heavy(write_ctx_t *ctx, test_feature_t *t, pcb_subc_t *subc, pcb_layer_t *layer, pcb_any_obj_t *o)
{
	pcb_layer_type_t flg;

	if (o->term == NULL)
		return -1;
	flg = pcb_layer_flags_(layer);
	if (!(flg & PCB_LYT_COPPER))
		return -1;

	memset(t, 0, sizeof(test_feature_t));

	if (!(pcb_brave & PCB_BRAVE_OLD_NETLIST)) {
		pcb_net_term_t *term = pcb_net_find_by_obj(&ctx->pcb->netlist[PCB_NETLIST_EDITED], (pcb_any_obj_t *)o);
		t->netname = (term == NULL) ? "N/C" : term->parent.net->name;
	}
	else {
		pcb_lib_menu_t *mn = pcb_netlist_find_net4term(ctx->pcb, (pcb_any_obj_t *)o);
		t->netname = (mn == NULL) ? "N/C" : pcb_netlist_name(mn);
	}

	t->o = o;
	t->refdes = subc->refdes;
	t->term = o->term;
	t->access_top = (flg & PCB_LYT_TOP);
	t->access_bottom = (flg & PCB_LYT_BOTTOM);
	return 0;
}

static void ipcd356_write_line(write_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *layer, pcb_line_t *line)
{
	test_feature_t t;

	if (ipcd356_heavy(ctx, &t, subc, layer, (pcb_any_obj_t *)line) != 0)
		return;

	t.cx = (line->Point1.X + line->Point2.X) / 2;
	t.cy = (line->Point1.Y + line->Point2.Y) / 2;
	t.width = t.height = line->Thickness;
	t.rot = atan2(line->Point2.Y - line->Point1.Y, line->Point2.X - line->Point1.X) * PCB_RAD_TO_DEG;
	ipcd356_write_feature(ctx, &t);
}

static void ipcd356_write_arc(write_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *layer, pcb_arc_t *arc)
{
	test_feature_t t;

	if (ipcd356_heavy(ctx, &t, subc, layer, (pcb_any_obj_t *)arc) != 0)
		return;

	pcb_arc_middle(arc, &t.cx, &t.cy);
	t.width = t.height = arc->Thickness;
	t.rot = arc->StartAngle + (arc->Delta / 2);
	ipcd356_write_feature(ctx, &t);
}

static void ipcd356_write_poly(write_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *layer, pcb_poly_t *poly)
{
	test_feature_t t;

	if (ipcd356_heavy(ctx, &t, subc, layer, (pcb_any_obj_t *)poly) != 0)
		return;

	t.cx = (poly->BoundingBox.X1 + poly->BoundingBox.X2) / 2;
	t.cy = (poly->BoundingBox.Y1 + poly->BoundingBox.Y2) / 2;
	t.width = (poly->BoundingBox.X2 - poly->BoundingBox.X1) / 8 + 1;
	t.height = (poly->BoundingBox.Y2 - poly->BoundingBox.Y1) / 8 + 1;
	t.rot = 0;
	ipcd356_write_feature(ctx, &t);
}

static void ipcd356_write(pcb_board_t *pcb, FILE *f)
{
	write_ctx_t ctx;

	ctx.pcb = pcb;
	ctx.f = f;
	ctx.is_mil = (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0);

	ipcd356_write_head(&ctx);
	PCB_SUBC_LOOP(pcb->Data); {
TODO("subc: subc-in-subc")
		PCB_PADSTACK_LOOP(subc->data); {
			ipcd356_write_pstk(&ctx, subc, padstack);
		} PCB_END_LOOP;
		PCB_LINE_ALL_LOOP(subc->data); {
			ipcd356_write_line(&ctx, subc, layer, line);
		} PCB_ENDALL_LOOP;
		PCB_ARC_ALL_LOOP(subc->data); {
			ipcd356_write_arc(&ctx, subc, layer, arc);
		} PCB_ENDALL_LOOP;
		PCB_POLY_ALL_LOOP(subc->data); {
			ipcd356_write_poly(&ctx, subc, layer, polygon);
		} PCB_ENDALL_LOOP;
	} PCB_END_LOOP;
	ipcd356_write_foot(&ctx);
}

/*** export hid administration and API/glu ***/

static pcb_hid_attribute_t ipcd356_options[] = {
/* %start-doc options "8 IPC-D-356 Netlist Export"
@ftable @code
@item --netlist-file <string>
Name of the IPC-D-356 Netlist output file.
@end ftable
%end-doc
*/
	{
	 "netlistfile",
	 "Name of the IPC-D-356 Netlist output file",
	 PCB_HATT_STRING,
	 0, 0, {0, 0, 0}, 0, 0},
#define HA_ipcd356_filename 0
};

#define NUM_OPTIONS (sizeof(ipcd356_options)/sizeof(ipcd356_options[0]))

static pcb_hid_attr_val_t ipcd356_values[NUM_OPTIONS];

static pcb_hid_attribute_t *ipcd356_get_export_options(int *n)
{
	if ((PCB != NULL) && (ipcd356_options[HA_ipcd356_filename].default_val.str_value == NULL))
		pcb_derive_default_filename(PCB->Filename, &ipcd356_options[HA_ipcd356_filename], ".net");

	if (n != NULL)
		*n = NUM_OPTIONS;

	return ipcd356_options;
}

static int ipcd356_parse_arguments(int *argc, char ***argv)
{
	return pcb_hid_parse_command_line(argc, argv);
}

static void ipcd356_do_export(pcb_hid_attr_val_t *options)
{
	int n;
	const char *fn;
	FILE *f;

	if (!options) {
		ipcd356_get_export_options(0);

		for (n = 0; n < NUM_OPTIONS; n++)
			ipcd356_values[n] = ipcd356_options[n].default_val;

		options = ipcd356_values;
	}

	fn = options[HA_ipcd356_filename].str_value;
	if (fn == NULL)
		fn = "pcb-rnd-out.net";

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't open %s for write\n", fn);
		return;
	}
	ipcd356_write(PCB, f);
	fclose(f);
}

static pcb_hid_t ipcd356_hid;

int pplg_check_ver_export_ipcd356(int ver_needed) { return 0; }

void pplg_uninit_export_ipcd356(void)
{
	pcb_hid_remove_attributes_by_cookie(ipcd356_cookie);
}

int pplg_init_export_ipcd356(void)
{
	PCB_API_CHK_VER;
	memset(&ipcd356_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&ipcd356_hid);

	ipcd356_hid.struct_size = sizeof(pcb_hid_t);
	ipcd356_hid.name = "IPC-D-356";
	ipcd356_hid.description = "Exports to IPC-D-356 netlist";
	ipcd356_hid.exporter = 1;

	ipcd356_hid.get_export_options = ipcd356_get_export_options;
	ipcd356_hid.do_export = ipcd356_do_export;
	ipcd356_hid.parse_arguments = ipcd356_parse_arguments;

	pcb_hid_register_hid(&ipcd356_hid);

	pcb_hid_register_attributes(ipcd356_options, sizeof(ipcd356_options) / sizeof(ipcd356_options[0]), ipcd356_cookie, 0);
	return 0;
}
